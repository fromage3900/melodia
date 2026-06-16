// Native glue for the rhythm-battle vertical slice.

#include "MelodiaBattleLoopLibrary.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaCombatStateComponent.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/UObjectIterator.h"

const FName UMelodiaBattleLoopLibrary::RhythmVictoryResolvedTag = TEXT("MelodiaRhythmVictoryResolved");
const FName UMelodiaBattleLoopLibrary::RhythmRewardConfirmedTag = TEXT("MelodiaRhythmRewardConfirmed");
const FName UMelodiaBattleLoopLibrary::RhythmExplorationReadyTag = TEXT("MelodiaRhythmExplorationReady");
const FName UMelodiaBattleLoopLibrary::RhythmUltimateReadyTag = TEXT("MelodiaRhythmUltimateReady");

namespace
{
void ForEachRhythmHUD(UWorld* World, TFunctionRef<void(UMelodiaRhythmHUDWidget&)> Callback)
{
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		UMelodiaRhythmHUDWidget* Widget = *It;
		if (Widget && Widget->GetWorld() == World)
		{
			Callback(*Widget);
		}
	}
}

void PublishLoopPhase(UWorld* World, const EMelodiaLoopPhase Phase)
{
	if (!World)
	{
		return;
	}

	if (AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World)))
	{
		GameMode->SetLoopPhase(Phase);
	}
}

UMelodiaCombatStateComponent* FindOrCreateCombatState(AActor* BattleController)
{
	if (!BattleController)
	{
		return nullptr;
	}

	if (UMelodiaCombatStateComponent* ExistingState = BattleController->FindComponentByClass<UMelodiaCombatStateComponent>())
	{
		return ExistingState;
	}

	UMelodiaCombatStateComponent* NewState = NewObject<UMelodiaCombatStateComponent>(BattleController, UMelodiaCombatStateComponent::StaticClass(), TEXT("MelodiaCombatState"));
	if (NewState)
	{
		NewState->RegisterComponent();
		BattleController->AddInstanceComponent(NewState);
	}

	return NewState;
}
}

bool UMelodiaBattleLoopLibrary::ApplyRhythmBattleHit(UObject* WorldContextObject, AActor* BattleController, const float Grade, const int32 ComboToWin)
{
	return ApplyRhythmBattleAction(WorldContextObject, BattleController, Grade, ComboToWin, false);
}

bool UMelodiaBattleLoopLibrary::ApplyRhythmSkillAction(UObject* WorldContextObject, AActor* BattleController, const float Grade, const int32 ComboToWin)
{
	return ApplyRhythmBattleAction(WorldContextObject, BattleController, Grade, ComboToWin, true);
}

bool UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(UObject* WorldContextObject, AActor* BattleController, const EMelodiaRhythmBattleCommand Command, const float Grade, const int32 ComboToWin)
{
	switch (Command)
	{
	case EMelodiaRhythmBattleCommand::Skill:
		return ApplyRhythmSkillAction(WorldContextObject, BattleController, Grade, ComboToWin);
	case EMelodiaRhythmBattleCommand::Ultimate:
		return TriggerRhythmUltimate(WorldContextObject, BattleController);
	case EMelodiaRhythmBattleCommand::Basic:
	default:
		return ApplyRhythmBattleHit(WorldContextObject, BattleController, Grade, ComboToWin);
	}
}

bool UMelodiaBattleLoopLibrary::ApplyRhythmBattleAction(UObject* WorldContextObject, AActor* BattleController, const float Grade, const int32 ComboToWin, const bool bSkillAction)
{
	if (!BattleController)
	{
		return false;
	}

	const int32 LegacyComboToWin = ComboToWin;
	(void)LegacyComboToWin;

	if (BattleController->Tags.Contains(RhythmVictoryResolvedTag))
	{
		ConfirmRhythmVictoryReward(BattleController);
		return false;
	}

	if (BattleController->Tags.Contains(RhythmExplorationReadyTag))
	{
		BattleController->Tags.Remove(RhythmExplorationReadyTag);
		SetIntPropertyValue(BattleController, TEXT("RhythmCombo"), FMath::Max(1, GetIntPropertyValue(BattleController, TEXT("RhythmCombo"), 1)));
		PublishLoopPhase(BattleController->GetWorld(), EMelodiaLoopPhase::Battle);
	}

	if (FFloatProperty* MultiplierProperty = CastField<FFloatProperty>(BattleController->GetClass()->FindPropertyByName(TEXT("CurrentRhythmMultiplier"))))
	{
		MultiplierProperty->SetPropertyValue_InContainer(BattleController, FMath::Max(0.0f, Grade));
	}

	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	const int32 SkillPointMax = CombatState ? FMath::Max(1, CombatState->SkillPointMax) : FMath::Max(1, GetIntPropertyValue(BattleController, TEXT("RhythmSkillPointMax"), 5));
	int32 NewSkillPoints = CombatState ? FMath::Clamp(CombatState->SkillPoints, 0, SkillPointMax) : FMath::Clamp(GetIntPropertyValue(BattleController, TEXT("RhythmSkillPoints"), 3), 0, SkillPointMax);
	int32 SkillPointDelta = 0;
	int32 BasicActionCount = CombatState ? CombatState->BasicActivationCount : GetIntPropertyValue(BattleController, TEXT("RhythmBasicActivationCount"), 0);
	int32 SkillActionCount = CombatState ? CombatState->SkillActivationCount : GetIntPropertyValue(BattleController, TEXT("RhythmSkillActivationCount"), 0);

	if (bSkillAction)
	{
		constexpr int32 SkillCost = 1;
		const bool bCanSpend = CombatState ? CombatState->SpendSkillPoints(SkillCost) : NewSkillPoints >= SkillCost;
		if (!bCanSpend)
		{
			if (UWorld* World = BattleController->GetWorld())
			{
				ForEachRhythmHUD(World, [NewSkillPoints, SkillPointMax](UMelodiaRhythmHUDWidget& Widget)
				{
					Widget.SetSkillPoints(NewSkillPoints, SkillPointMax);
					Widget.ShowActionPrompt(TEXT("Not enough skill points | Basic attack"));
					Widget.TriggerSparkleBurst();
				});
			}
			return false;
		}

		if (CombatState)
		{
			NewSkillPoints = CombatState->SkillPoints;
			SkillPointDelta = CombatState->LastSkillPointDelta;
			SkillActionCount = CombatState->SkillActivationCount;
		}
		else
		{
			--NewSkillPoints;
			SkillPointDelta = -SkillCost;
			++SkillActionCount;
		}
	}
	else
	{
		if (CombatState)
		{
			NewSkillPoints = CombatState->AddSkillPoints(1);
			SkillPointDelta = CombatState->LastSkillPointDelta;
			BasicActionCount = CombatState->BasicActivationCount;
		}
		else
		{
			const int32 PreviousSkillPoints = NewSkillPoints;
			NewSkillPoints = FMath::Clamp(NewSkillPoints + 1, 0, SkillPointMax);
			SkillPointDelta = NewSkillPoints - PreviousSkillPoints;
			++BasicActionCount;
		}
	}

	SetIntPropertyValue(BattleController, TEXT("RhythmSkillPoints"), NewSkillPoints);
	SetIntPropertyValue(BattleController, TEXT("RhythmSkillPointMax"), SkillPointMax);
	SetIntPropertyValue(BattleController, TEXT("RhythmLastSkillPointDelta"), SkillPointDelta);
	SetIntPropertyValue(BattleController, TEXT("RhythmBasicActivationCount"), BasicActionCount);
	SetIntPropertyValue(BattleController, TEXT("RhythmSkillActivationCount"), SkillActionCount);

	const float UltimateMax = CombatState ? FMath::Max(1.0f, CombatState->UltimateMax) : FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), 100.0f));
	const float UltimateGainPerGrade = bSkillAction ? 26.0f : 34.0f;
	const float UltimateGain = FMath::Clamp(FMath::Max(0.0f, Grade) * UltimateGainPerGrade, 0.0f, UltimateMax);
	const float NewUltimateGauge = CombatState ? CombatState->AddUltimateGauge(UltimateGain) : FMath::Clamp(GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f) + UltimateGain, 0.0f, UltimateMax);
	SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), NewUltimateGauge);
	SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), UltimateMax);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateReady"), NewUltimateGauge >= UltimateMax);
	if (NewUltimateGauge >= UltimateMax)
	{
		BattleController->Tags.AddUnique(RhythmUltimateReadyTag);
	}
	const bool bUltimateReady = NewUltimateGauge >= UltimateMax;

	const float MaxHP = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), 45.0f));
	float CurrentHP = GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), MaxHP);
	if (CurrentHP <= 0.0f)
	{
		CurrentHP = MaxHP;
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), CurrentHP);
	}

	const float BaseDamage = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmBaseDamage"), 10.0f));
	const float ActionScalar = bSkillAction ? 1.75f : 1.0f;
	const float RawDamage = FMath::Max(1.0f, BaseDamage * FMath::Max(0.0f, Grade) * ActionScalar);
	const bool bHadBreakFollowUp = CombatState && CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed;
	const float FollowUpBonusDamage = bHadBreakFollowUp ? FMath::Max(1.0f, RawDamage * 0.70f) : 0.0f;
	if (CombatState && bHadBreakFollowUp)
	{
		CombatState->ConsumeBreakFollowUp(FollowUpBonusDamage);
	}
	const bool bBreakFollowUpConsumed = CombatState && CombatState->bBreakFollowUpConsumed && FollowUpBonusDamage > 0.0f;
	const float Damage = RawDamage + FollowUpBonusDamage;
	const float ToughnessDamage = FMath::Max(1.0f, (bSkillAction ? 42.0f : 32.0f) * FMath::Max(0.0f, Grade));
	const bool bBreakTriggered = CombatState && CombatState->ApplyEnemyToughnessDamage(ToughnessDamage);
	if (CombatState && bBreakTriggered)
	{
		CombatState->ApplyEnemyTurnDelay(bSkillAction ? 2 : 1);
	}
	else if (CombatState && bBreakFollowUpConsumed)
	{
		CombatState->ConsumeEnemyTurnDelay();
	}
	const bool bEnemyBroken = CombatState && CombatState->bEnemyBroken;
	const bool bBreakFollowUpAvailable = CombatState && CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed;
	const int32 EnemyTurnDelayStacks = CombatState ? CombatState->EnemyTurnDelayStacks : 0;
	const int32 LastEnemyTurnDelay = CombatState ? CombatState->LastEnemyTurnDelay : 0;
	const float EnemyToughness = CombatState ? CombatState->EnemyToughness : FMath::Max(0.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), 100.0f) - ToughnessDamage);
	const float EnemyToughnessMax = CombatState ? FMath::Max(1.0f, CombatState->EnemyToughnessMax) : FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), 100.0f));
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), EnemyToughness);
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), EnemyToughnessMax);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastToughnessDamage"), ToughnessDamage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), FollowUpBonusDamage);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmEnemyBroken"), bEnemyBroken);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpAvailable"), bBreakFollowUpAvailable);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpConsumed"), bBreakFollowUpConsumed);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyBreakCount"), CombatState ? CombatState->EnemyBreakCount : (bBreakTriggered ? 1 : 0));
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpAvailableCount"), CombatState ? CombatState->BreakFollowUpAvailableCount : 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpConsumedCount"), CombatState ? CombatState->BreakFollowUpConsumedCount : 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayStacks"), EnemyTurnDelayStacks);
	SetIntPropertyValue(BattleController, TEXT("RhythmLastEnemyTurnDelay"), LastEnemyTurnDelay);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayApplyCount"), CombatState ? CombatState->EnemyTurnDelayApplyCount : 0);
	const float RemainingHP = FMath::Max(0.0f, CurrentHP - Damage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), Damage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), RemainingHP);

	if (UWorld* World = BattleController->GetWorld())
	{
		const FString ActionName = bSkillAction ? TEXT("Skill") : TEXT("Basic");
		const FString FollowUpText = bBreakFollowUpConsumed ? FString::Printf(TEXT(" | Follow %.0f"), FollowUpBonusDamage) : TEXT("");
		const FString DelayText = EnemyTurnDelayStacks > 0 ? FString::Printf(TEXT(" | Delay %d"), EnemyTurnDelayStacks) : TEXT("");
		const FString StatusText = FString::Printf(TEXT("%s %.0f%s | HP %.0f/%.0f | Break %.0f%%%s | SP %d/%d | Ult %.0f%%"), *ActionName, Damage, *FollowUpText, RemainingHP, MaxHP, (EnemyToughness / EnemyToughnessMax) * 100.0f, *DelayText, NewSkillPoints, SkillPointMax, (NewUltimateGauge / UltimateMax) * 100.0f);
		FString ActionPrompt = bSkillAction ? TEXT("Skill spent 1 SP | Rhythm follow-up") : TEXT("Basic +1 SP | Build ultimate");
		if (bEnemyBroken)
		{
			ActionPrompt = TEXT("Enemy broken | Press the advantage");
		}
		if (bBreakFollowUpAvailable)
		{
			ActionPrompt = TEXT("Break follow-up ready");
		}
		if (EnemyTurnDelayStacks > 0)
		{
			ActionPrompt = FString::Printf(TEXT("Enemy delayed | Tempo +%d"), EnemyTurnDelayStacks);
		}
		if (bBreakFollowUpConsumed)
		{
			ActionPrompt = TEXT("Follow-up duet landed");
		}
		if (bUltimateReady)
		{
			ActionPrompt = TEXT("Ultimate ready | Act now");
		}
		ForEachRhythmHUD(World, [StatusText, NewUltimateGauge, UltimateMax, bUltimateReady, RemainingHP, MaxHP, Damage, NewSkillPoints, SkillPointMax, ActionPrompt, EnemyToughness, EnemyToughnessMax, bEnemyBroken, bBreakFollowUpAvailable, bBreakFollowUpConsumed, FollowUpBonusDamage, EnemyTurnDelayStacks, LastEnemyTurnDelay](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetUltimateGauge(NewUltimateGauge, UltimateMax, bUltimateReady);
			Widget.SetSkillPoints(NewSkillPoints, SkillPointMax);
			Widget.SetEnemyVitals(RemainingHP, MaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, bEnemyBroken);
			Widget.SetBreakFollowUpWindow(bBreakFollowUpAvailable, bBreakFollowUpConsumed, FollowUpBonusDamage);
			Widget.SetEnemyTurnDelay(EnemyTurnDelayStacks, LastEnemyTurnDelay);
			Widget.ShowBattleStatus(StatusText);
			Widget.ShowActionPrompt(ActionPrompt);
			Widget.TriggerDamageFlash(Damage);
			if (bUltimateReady)
			{
				Widget.ShowUltimateReady();
			}
		});
	}

	PublishReactiveCommandState(
		BattleController,
		bSkillAction ? TEXT("Skill") : TEXT("Basic"),
		bEnemyBroken ? TEXT("Broken") : (bSkillAction ? TEXT("Break stance") : TEXT("Preparing strike")),
		bEnemyBroken ? 0.0f : (bSkillAction ? Damage * 0.45f : Damage * 0.32f),
		bUltimateReady || bBreakTriggered,
		false);

	if (RemainingHP > 0.0f)
	{
		UE_LOG(LogTemp, Log, TEXT("Melodia rhythm battle hit dealt %.1f damage, enemy HP %.1f/%.1f."), Damage, RemainingHP, MaxHP);
		return false;
	}

	ResolveRhythmVictory(WorldContextObject, BattleController, RemainingHP);
	return true;
}

bool UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(AActor* BattleController)
{
	return BattleController && BattleController->Tags.Contains(RhythmVictoryResolvedTag);
}

bool UMelodiaBattleLoopLibrary::HasRhythmRewardBeenConfirmed(AActor* BattleController)
{
	return BattleController && BattleController->Tags.Contains(RhythmRewardConfirmedTag);
}

bool UMelodiaBattleLoopLibrary::IsRhythmExplorationReady(AActor* BattleController)
{
	return BattleController && BattleController->Tags.Contains(RhythmExplorationReadyTag);
}

bool UMelodiaBattleLoopLibrary::IsRhythmUltimateReady(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	return BattleController && (BattleController->Tags.Contains(RhythmUltimateReadyTag) || (CombatState && CombatState->bUltimateReady) || GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f) >= GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), 100.0f));
}

float UMelodiaBattleLoopLibrary::GetRhythmUltimateGauge(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	if (CombatState)
	{
		return CombatState->UltimateGauge;
	}

	return GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f);
}

int32 UMelodiaBattleLoopLibrary::GetRhythmSkillPoints(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	if (CombatState)
	{
		return FMath::Clamp(CombatState->SkillPoints, 0, FMath::Max(1, CombatState->SkillPointMax));
	}

	return GetIntPropertyValue(BattleController, TEXT("RhythmSkillPoints"), 3);
}

int32 UMelodiaBattleLoopLibrary::GetRhythmSkillPointMax(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	if (CombatState)
	{
		return FMath::Max(1, CombatState->SkillPointMax);
	}

	return FMath::Max(1, GetIntPropertyValue(BattleController, TEXT("RhythmSkillPointMax"), 5));
}

bool UMelodiaBattleLoopLibrary::CanUseRhythmSkill(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	if (CombatState)
	{
		return CombatState->CanSpendSkillPoints(1);
	}

	return GetRhythmSkillPoints(BattleController) >= 1;
}

bool UMelodiaBattleLoopLibrary::TriggerRhythmUltimate(UObject* WorldContextObject, AActor* BattleController)
{
	if (!BattleController || !IsRhythmUltimateReady(BattleController) || BattleController->Tags.Contains(RhythmVictoryResolvedTag))
	{
		return false;
	}

	const float MaxHP = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), 45.0f));
	float CurrentHP = GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), MaxHP);
	if (CurrentHP <= 0.0f)
	{
		CurrentHP = MaxHP;
	}

	const float BaseDamage = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmBaseDamage"), 10.0f));
	const float UltimateDamage = FMath::Max(BaseDamage * 3.0f, MaxHP * 0.75f);
	const float RemainingHP = FMath::Max(0.0f, CurrentHP - UltimateDamage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), UltimateDamage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastUltimateDamage"), UltimateDamage);
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), RemainingHP);
	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	if (CombatState)
	{
		CombatState->RecordUltimateActivated(UltimateDamage);
		CombatState->ApplyEnemyToughnessDamage(CombatState->EnemyToughnessMax);
		CombatState->ApplyEnemyTurnDelay(2);
	}
	const float EnemyToughness = CombatState ? CombatState->EnemyToughness : 0.0f;
	const float EnemyToughnessMax = CombatState ? FMath::Max(1.0f, CombatState->EnemyToughnessMax) : 100.0f;
	const bool bEnemyBroken = CombatState && CombatState->bEnemyBroken;
	const bool bBreakFollowUpAvailable = CombatState && CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed;
	const int32 EnemyTurnDelayStacks = CombatState ? CombatState->EnemyTurnDelayStacks : 2;
	const int32 LastEnemyTurnDelay = CombatState ? CombatState->LastEnemyTurnDelay : 2;
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), EnemyToughness);
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), EnemyToughnessMax);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastToughnessDamage"), EnemyToughnessMax);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), 0.0f);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmEnemyBroken"), bEnemyBroken);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpAvailable"), bBreakFollowUpAvailable);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpConsumed"), false);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyBreakCount"), CombatState ? CombatState->EnemyBreakCount : 1);
	SetIntPropertyValue(BattleController, TEXT("RhythmTotalEnemyBreakCount"), CombatState ? CombatState->TotalEnemyBreakCount : 1);
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpAvailableCount"), CombatState ? CombatState->BreakFollowUpAvailableCount : 1);
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpConsumedCount"), CombatState ? CombatState->BreakFollowUpConsumedCount : 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayStacks"), EnemyTurnDelayStacks);
	SetIntPropertyValue(BattleController, TEXT("RhythmLastEnemyTurnDelay"), LastEnemyTurnDelay);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayApplyCount"), CombatState ? CombatState->EnemyTurnDelayApplyCount : 1);

	SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateReady"), false);
	SetIntPropertyValue(BattleController, TEXT("RhythmUltimateActivationCount"), CombatState ? CombatState->UltimateActivationCount : GetIntPropertyValue(BattleController, TEXT("RhythmUltimateActivationCount"), 0) + 1);
	BattleController->Tags.Remove(RhythmUltimateReadyTag);
	PublishReactiveCommandState(BattleController, TEXT("Ultimate"), TEXT("Interrupted"), UltimateDamage, false, true);

	if (UWorld* World = BattleController->GetWorld())
	{
		const int32 SkillPoints = GetRhythmSkillPoints(BattleController);
		const int32 SkillPointMax = GetRhythmSkillPointMax(BattleController);
		ForEachRhythmHUD(World, [UltimateDamage, RemainingHP, MaxHP, SkillPoints, SkillPointMax, EnemyToughness, EnemyToughnessMax, bEnemyBroken, bBreakFollowUpAvailable, EnemyTurnDelayStacks, LastEnemyTurnDelay](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetUltimateGauge(0.0f, 100.0f, false);
			Widget.SetSkillPoints(SkillPoints, SkillPointMax);
			Widget.SetEnemyVitals(RemainingHP, MaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, bEnemyBroken);
			Widget.ShowUltimateActivated(UltimateDamage);
			Widget.SetBreakFollowUpWindow(bBreakFollowUpAvailable, false, 0.0f);
			Widget.SetEnemyTurnDelay(EnemyTurnDelayStacks, LastEnemyTurnDelay);
			if (bBreakFollowUpAvailable)
			{
				Widget.ShowActionPrompt(TEXT("Break follow-up ready"));
			}
		});
	}

	if (RemainingHP <= 0.0f)
	{
		ResolveRhythmVictory(WorldContextObject, BattleController, RemainingHP);
	}

	return true;
}

void UMelodiaBattleLoopLibrary::ResetRhythmBattleEncounter(AActor* BattleController)
{
	if (!BattleController)
	{
		return;
	}

	const float MaxHP = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), 45.0f));
	SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), MaxHP);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), 0.0f);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastUltimateDamage"), 0.0f);
	SetFloatPropertyValue(BattleController, TEXT("CurrentRhythmMultiplier"), 1.0f);
	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	if (CombatState)
	{
		CombatState->ResetUltimateGauge();
		CombatState->ResetActionEconomy();
		SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), CombatState->UltimateMax);
		SetIntPropertyValue(BattleController, TEXT("RhythmSkillPointMax"), CombatState->SkillPointMax);
		SetIntPropertyValue(BattleController, TEXT("RhythmSkillPoints"), CombatState->SkillPoints);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), CombatState->EnemyToughness);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), CombatState->EnemyToughnessMax);
	}

	SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f);
	SetIntPropertyValue(BattleController, TEXT("RhythmCombo"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmUltimateActivationCount"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmLastSkillPointDelta"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmBasicActivationCount"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmSkillActivationCount"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyBreakCount"), 0);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastToughnessDamage"), 0.0f);
	SetFloatPropertyValue(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), 0.0f);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmEnemyBroken"), false);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpAvailable"), false);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpConsumed"), false);
	SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateReady"), false);
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpAvailableCount"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpConsumedCount"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayStacks"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmLastEnemyTurnDelay"), 0);
	SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayApplyCount"), 0);
	SetBoolPropertyValue(BattleController, TEXT("isBattleOver"), false);
	SetBoolPropertyValue(BattleController, TEXT("isPlayerVictory"), false);
	BattleController->Tags.Remove(RhythmVictoryResolvedTag);
	BattleController->Tags.Remove(RhythmExplorationReadyTag);
	BattleController->Tags.Remove(RhythmUltimateReadyTag);
	PublishReactiveCommandState(BattleController, TEXT("Battle Start"), TEXT("Waiting"), 0.0f, false, false);

	if (UWorld* World = BattleController->GetWorld())
	{
		const int32 SkillPoints = GetRhythmSkillPoints(BattleController);
		const int32 SkillPointMax = GetRhythmSkillPointMax(BattleController);
		const float EnemyToughness = CombatState ? CombatState->EnemyToughness : GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), 100.0f);
		const float EnemyToughnessMax = CombatState ? CombatState->EnemyToughnessMax : GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), 100.0f);
		ForEachRhythmHUD(World, [MaxHP, SkillPoints, SkillPointMax, EnemyToughness, EnemyToughnessMax](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.ClearBattleStatus();
			Widget.SetEnemyVitals(MaxHP, MaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, false);
			Widget.SetBreakFollowUpWindow(false, false, 0.0f);
			Widget.SetEnemyTurnDelay(0, 0);
			Widget.SetSkillPoints(SkillPoints, SkillPointMax);
			Widget.ShowActionPrompt(TEXT("Battle start | Choose action"));
		});
	}
}

void UMelodiaBattleLoopLibrary::ConfirmRhythmVictoryReward(AActor* BattleController)
{
	if (!BattleController)
	{
		return;
	}

	ResetRhythmBattleEncounter(BattleController);
	BattleController->Tags.AddUnique(RhythmRewardConfirmedTag);
	BattleController->Tags.AddUnique(RhythmExplorationReadyTag);
	PublishLoopPhase(BattleController->GetWorld(), EMelodiaLoopPhase::ExplorationReady);

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.ShowBattleStatus(TEXT("Reward claimed | Exploration ready"));
		});
	}
}

float UMelodiaBattleLoopLibrary::GetFloatPropertyValue(AActor* Actor, const FName PropertyName, const float FallbackValue)
{
	if (!Actor)
	{
		return FallbackValue;
	}

	const FFloatProperty* FloatProperty = CastField<FFloatProperty>(Actor->GetClass()->FindPropertyByName(PropertyName));
	return FloatProperty ? FloatProperty->GetPropertyValue_InContainer(Actor) : FallbackValue;
}

void UMelodiaBattleLoopLibrary::SetFloatPropertyValue(AActor* Actor, const FName PropertyName, const float Value)
{
	if (Actor)
	{
		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
		{
			FloatProperty->SetPropertyValue_InContainer(Actor, Value);
		}
	}
}

int32 UMelodiaBattleLoopLibrary::GetIntPropertyValue(AActor* Actor, const FName PropertyName, const int32 FallbackValue)
{
	if (!Actor)
	{
		return FallbackValue;
	}

	const FIntProperty* IntProperty = CastField<FIntProperty>(Actor->GetClass()->FindPropertyByName(PropertyName));
	return IntProperty ? IntProperty->GetPropertyValue_InContainer(Actor) : FallbackValue;
}

void UMelodiaBattleLoopLibrary::SetIntPropertyValue(AActor* Actor, const FName PropertyName, const int32 Value)
{
	if (Actor)
	{
		if (FIntProperty* IntProperty = CastField<FIntProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
		{
			IntProperty->SetPropertyValue_InContainer(Actor, Value);
		}
	}
}

void UMelodiaBattleLoopLibrary::SetBoolPropertyValue(AActor* Actor, const FName PropertyName, const bool bValue)
{
	if (Actor)
	{
		if (FBoolProperty* BoolProperty = CastField<FBoolProperty>(Actor->GetClass()->FindPropertyByName(PropertyName)))
		{
			BoolProperty->SetPropertyValue_InContainer(Actor, bValue);
		}
	}
}

void UMelodiaBattleLoopLibrary::PublishReactiveCommandState(AActor* BattleController, const FString& CommandName, const FString& IntentName, const float IntentPower, const bool bUltimateWindow, const bool bUltimateInterrupt)
{
	if (!BattleController)
	{
		return;
	}

	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	int32 ActiveTurnIndex = 0;
	if (CombatState)
	{
		CombatState->RecordCommandState(CommandName, IntentName, IntentPower, bUltimateWindow, bUltimateInterrupt);
		ActiveTurnIndex = CombatState->ActiveTurnOrderIndex;
		SetIntPropertyValue(BattleController, TEXT("RhythmCommandSequence"), CombatState->CommandSequence);
		SetIntPropertyValue(BattleController, TEXT("RhythmActiveTurnOrderIndex"), CombatState->ActiveTurnOrderIndex);
		SetIntPropertyValue(BattleController, TEXT("RhythmUltimateInterruptCount"), CombatState->UltimateInterruptCount);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyIntentPower"), CombatState->EnemyIntentPower);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateInterruptWindow"), CombatState->bUltimateInterruptWindow);
	}

	if (UWorld* World = BattleController->GetWorld())
	{
		TArray<FString> TurnOrder;
		TurnOrder.Add(TEXT("Melusina"));
		TurnOrder.Add(bUltimateInterrupt ? TEXT("Melusina") : TEXT("Enemy"));
		TurnOrder.Add(TEXT("Melusina"));
		TurnOrder.Add(TEXT("Enemy"));
		ForEachRhythmHUD(World, [TurnOrder, ActiveTurnIndex, CommandName, IntentName, IntentPower, bUltimateWindow, bUltimateInterrupt](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetTurnOrderPreview(TurnOrder, ActiveTurnIndex);
			Widget.SetReactiveBattleState(CommandName, IntentName, IntentPower, bUltimateWindow, bUltimateInterrupt);
		});
	}
}

void UMelodiaBattleLoopLibrary::ResolveRhythmVictory(UObject* WorldContextObject, AActor* BattleController, const float RemainingHP)
{
	if (!BattleController || BattleController->Tags.Contains(RhythmVictoryResolvedTag))
	{
		return;
	}

	BattleController->Tags.Add(RhythmVictoryResolvedTag);
	PublishLoopPhase(BattleController->GetWorld(), EMelodiaLoopPhase::VictoryReward);

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.ShowVictoryReward(TEXT("Victory! Reward +100G"));
		});
	}

	if (FBoolProperty* BattleOverProperty = CastField<FBoolProperty>(BattleController->GetClass()->FindPropertyByName(TEXT("isBattleOver"))))
	{
		BattleOverProperty->SetPropertyValue_InContainer(BattleController, true);
	}

	if (FBoolProperty* PlayerVictoryProperty = CastField<FBoolProperty>(BattleController->GetClass()->FindPropertyByName(TEXT("isPlayerVictory"))))
	{
		PlayerVictoryProperty->SetPropertyValue_InContainer(BattleController, true);
	}

	UWorld* World = nullptr;
	if (WorldContextObject)
	{
		World = WorldContextObject->GetWorld();
	}
	if (!World)
	{
		World = BattleController->GetWorld();
	}

	if (World)
	{
		for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
		{
			It->NotifyBattleWon();
			UE_LOG(LogTemp, Log, TEXT("Melodia rhythm battle loop resolved victory with enemy HP %.1f."), RemainingHP);
			return;
		}

		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->FindFunction(TEXT("NotifyBattleWon")))
			{
				Actor->ProcessEvent(Actor->FindFunction(TEXT("NotifyBattleWon")), nullptr);
				UE_LOG(LogTemp, Log, TEXT("Melodia rhythm battle loop resolved victory with enemy HP %.1f."), RemainingHP);
				return;
			}
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("Melodia rhythm battle loop resolved victory, but no quest manager was available."));
}
