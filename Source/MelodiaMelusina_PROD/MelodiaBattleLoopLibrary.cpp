// Native glue for the rhythm-battle vertical slice.

#include "MelodiaBattleLoopLibrary.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleSession.h"
#include "MelodiaCombatStateComponent.h"
#include "MelodiaCombatSyncLibrary.h"
#include "MelodiaCompanionActor.h"
#include "MelodiaCoreRulesLibrary.h"
#include "MelodiaJRPGBridgeLibrary.h"
#include "MelodiaKeySystemLibrary.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaPartySubsystem.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "MelodiaRhythmExecutionComponent.h"
#include "MelodiaSongSkillLibrary.h"
#include "UObject/UObjectIterator.h"

const FName UMelodiaBattleLoopLibrary::RhythmVictoryResolvedTag = TEXT("MelodiaRhythmVictoryResolved");
const FName UMelodiaBattleLoopLibrary::RhythmRewardConfirmedTag = TEXT("MelodiaRhythmRewardConfirmed");
const FName UMelodiaBattleLoopLibrary::RhythmExplorationReadyTag = TEXT("MelodiaRhythmExplorationReady");
const FName UMelodiaBattleLoopLibrary::RhythmUltimateReadyTag = TEXT("MelodiaRhythmUltimateReady");

namespace
{
void ForEachRhythmHUD(UWorld* World, TFunctionRef<void(UMelodiaRhythmHUDWidget&)> Callback)
{
	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Callback(*Widget);
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

void CleanupBattlePresentation(AActor* BattleController)
{
	if (!BattleController)
	{
		return;
	}

	if (UMelodiaRhythmExecutionComponent* Execution = BattleController->FindComponentByClass<UMelodiaRhythmExecutionComponent>())
	{
		if (Execution->IsExecutionActive())
		{
			Execution->CancelExecution();
		}
	}

	UMelodiaJRPGBridgeLibrary::TeardownPhoenixBattleUI(BattleController, EMelodiaPhoenixTeardownScope::Full);
	BattleController->SetActorHiddenInGame(true);
	BattleController->SetActorEnableCollision(false);
}

void NotifyQuestManagers(UWorld* World, TFunctionRef<void(AMelodiaQuestManagerBase&)> Callback)
{
	if (!World)
	{
		return;
	}

	for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
	{
		if (AMelodiaQuestManagerBase* QuestManager = *It)
		{
			Callback(*QuestManager);
		}
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
	if (BattleController)
	{
		FName SkillId = NAME_None;
		if (const UWorld* World = BattleController->GetWorld())
		{
			if (const UGameInstance* GI = World->GetGameInstance())
			{
				if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
				{
					SkillId = Progression->State.ActiveSkillId;
				}
			}
		}

		if (SkillId.IsNone())
		{
			SkillId = UMelodiaSongSkillLibrary::ResolveSkillIdForMechanicLevel(WorldContextObject, 1);
		}

		FMelodiaSongSkillRecipe Recipe;
		if (!SkillId.IsNone() && UMelodiaSongSkillLibrary::ResolveSongSkill(WorldContextObject, SkillId, Recipe))
		{
			return ExecuteInstantSkillRecipe(WorldContextObject, BattleController, Recipe, Grade);
		}
	}

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

bool UMelodiaBattleLoopLibrary::ExecuteInstantSkillRecipe(UObject* WorldContextObject, AActor* BattleController, const FMelodiaSongSkillRecipe& Recipe, const float Grade)
{
	if (!BattleController || Recipe.SkillId.IsNone())
	{
		return false;
	}

	const int32 SkillCost = Recipe.SPCostOverride > 0 ? Recipe.SPCostOverride : 1;
	const float SkillGrade = FMath::Clamp(Grade, 0.35f, 1.5f) * FMath::Max(0.5f, Recipe.PowerScalar);

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [&Recipe](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.ShowActionPrompt(FString::Printf(TEXT("Skill: %s"), *Recipe.DisplayName.ToString()));
		});
	}

	return ApplyRhythmBattleAction(WorldContextObject, BattleController, SkillGrade, 3, true, Recipe.Element, SkillCost, Recipe.PowerScalar);
}

bool UMelodiaBattleLoopLibrary::ApplyRhythmBattleAction(UObject* WorldContextObject, AActor* BattleController, const float Grade, const int32 ComboToWin, const bool bSkillAction, const EMelodiaSpellElement AttackElement, const int32 SkillCost, const float SkillScalar)
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
		const int32 EffectiveSkillCost = FMath::Max(1, SkillCost);
		const bool bCanSpend = CombatState ? CombatState->SpendSkillPoints(EffectiveSkillCost) : NewSkillPoints >= EffectiveSkillCost;
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
			NewSkillPoints -= EffectiveSkillCost;
			SkillPointDelta = -EffectiveSkillCost;
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

	const float UltimateMax = CombatState ? FMath::Max(1.0f, CombatState->UltimateMax) : FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), 100.0f));
	const float UltimateGainPerGrade = bSkillAction ? 26.0f : 34.0f;
	const float UltimateGain = FMath::Clamp(FMath::Max(0.0f, Grade) * UltimateGainPerGrade, 0.0f, UltimateMax);
	const float NewUltimateGauge = CombatState ? CombatState->AddUltimateGauge(UltimateGain) : FMath::Clamp(GetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f) + UltimateGain, 0.0f, UltimateMax);

	if (CombatState)
	{
		if (NewUltimateGauge >= UltimateMax)
		{
			BattleController->Tags.AddUnique(RhythmUltimateReadyTag);
		}
	}
	else
	{
		SetIntPropertyValue(BattleController, TEXT("RhythmSkillPoints"), NewSkillPoints);
		SetIntPropertyValue(BattleController, TEXT("RhythmSkillPointMax"), SkillPointMax);
		SetIntPropertyValue(BattleController, TEXT("RhythmLastSkillPointDelta"), SkillPointDelta);
		SetIntPropertyValue(BattleController, TEXT("RhythmBasicActivationCount"), BasicActionCount);
		SetIntPropertyValue(BattleController, TEXT("RhythmSkillActivationCount"), SkillActionCount);

		SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), NewUltimateGauge);
		SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateMax"), UltimateMax);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateReady"), NewUltimateGauge >= UltimateMax);
		if (NewUltimateGauge >= UltimateMax)
		{
			BattleController->Tags.AddUnique(RhythmUltimateReadyTag);
		}
	}
	const bool bUltimateReady = NewUltimateGauge >= UltimateMax;

	// Bridge: when real JRPG template units exist, treat their live HP as authoritative so both models stay in sync.
	const FMelodiaJRPGVitals JRPGVitals = UMelodiaJRPGBridgeLibrary::ReadJRPGVitals(BattleController);
	float MaxHP = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), 45.0f));
	float CurrentHP = GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), MaxHP);
	if (JRPGVitals.bHasEnemies && JRPGVitals.EnemyMaxHP > 0.0f)
	{
		MaxHP = FMath::Max(1.0f, JRPGVitals.EnemyMaxHP);
		CurrentHP = JRPGVitals.EnemyHP;
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), MaxHP);
	}
	if (CurrentHP <= 0.0f)
	{
		CurrentHP = MaxHP;
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), CurrentHP);
	}

	const float BaseDamage = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmBaseDamage"), 10.0f));
	const float ActionScalar = bSkillAction ? 1.75f : 1.0f;
	const EMelodiaSpellElement DefenseElement = CombatState ? CombatState->EnemyElement : EMelodiaSpellElement::Tide;
	const bool bHasMatchingKey = CombatState
		&& bSkillAction
		&& CombatState->EquippedKeyElement == AttackElement;
	const float ElementMultiplier = bSkillAction
		? UMelodiaCoreRulesLibrary::CalculateElementalDamageMultiplier(AttackElement, DefenseElement, bHasMatchingKey)
		: 1.0f;
	const float PowerBoost = bSkillAction ? FMath::Max(0.35f, SkillScalar) : 1.0f;
	const float RawDamage = FMath::Max(1.0f, BaseDamage * FMath::Max(0.0f, Grade) * ActionScalar * ElementMultiplier * PowerBoost);
	const bool bHadBreakFollowUp = CombatState && CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed;
	const float FollowUpBonusDamage = bHadBreakFollowUp ? FMath::Max(1.0f, RawDamage * 0.70f) : 0.0f;
	if (CombatState && bHadBreakFollowUp)
	{
		CombatState->ConsumeBreakFollowUp(FollowUpBonusDamage);
	}
	const bool bBreakFollowUpConsumed = CombatState && CombatState->bBreakFollowUpConsumed && FollowUpBonusDamage > 0.0f;
	const float Damage = RawDamage + FollowUpBonusDamage;
	const float ToughnessDamage = FMath::Max(1.0f, (bSkillAction ? 42.0f : 32.0f) * FMath::Max(0.0f, Grade));
	if (bSkillAction && ElementMultiplier > 1.0f)
	{
		NotifyQuestManagers(BattleController->GetWorld(), [](AMelodiaQuestManagerBase& QuestManager)
		{
			QuestManager.NotifyWeaknessHit();
		});
	}
	const bool bBreakTriggered = CombatState && CombatState->ApplyEnemyToughnessDamage(ToughnessDamage);
	if (CombatState && bBreakTriggered)
	{
		CombatState->ApplyEnemyTurnDelay(bSkillAction ? 2 : 1);
		NotifyQuestManagers(BattleController->GetWorld(), [](AMelodiaQuestManagerBase& QuestManager)
		{
			QuestManager.NotifyEnemyBroken();
		});
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
	float RemainingHP = FMath::Max(0.0f, CurrentHP - Damage);
	if (JRPGVitals.bHasEnemies)
	{
		const float SyncedEnemyHP = UMelodiaJRPGBridgeLibrary::DamageActiveEnemy(BattleController, Damage);
		if (SyncedEnemyHP >= 0.0f)
		{
			RemainingHP = SyncedEnemyHP;
		}
	}

	if (CombatState)
	{
		FMelodiaCombatLegacyOverrides Overrides;
		Overrides.bOverrideLastDamage = true;
		Overrides.LastDamage = Damage;
		Overrides.bOverrideEnemyHP = true;
		Overrides.EnemyHP = RemainingHP;
		Overrides.bOverrideEnemyMaxHP = true;
		Overrides.EnemyMaxHP = MaxHP;
		Overrides.bOverrideLastToughnessDamage = true;
		Overrides.LastToughnessDamage = ToughnessDamage;
		Overrides.bOverrideLastFollowUpBonusDamage = true;
		Overrides.LastFollowUpBonusDamage = FollowUpBonusDamage;
		UMelodiaCombatSyncLibrary::MirrorCombatStateToLegacyProperties(BattleController, CombatState, Overrides);
	}
	else
	{
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughness"), EnemyToughness);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyToughnessMax"), EnemyToughnessMax);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastToughnessDamage"), ToughnessDamage);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastFollowUpBonusDamage"), FollowUpBonusDamage);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmEnemyBroken"), bEnemyBroken);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpAvailable"), bBreakFollowUpAvailable);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmBreakFollowUpConsumed"), bBreakFollowUpConsumed);
		SetIntPropertyValue(BattleController, TEXT("RhythmEnemyBreakCount"), bBreakTriggered ? 1 : 0);
		SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpAvailableCount"), 0);
		SetIntPropertyValue(BattleController, TEXT("RhythmBreakFollowUpConsumedCount"), 0);
		SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayStacks"), EnemyTurnDelayStacks);
		SetIntPropertyValue(BattleController, TEXT("RhythmLastEnemyTurnDelay"), LastEnemyTurnDelay);
		SetIntPropertyValue(BattleController, TEXT("RhythmEnemyTurnDelayApplyCount"), 0);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), Damage);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), RemainingHP);
	}

	if (UWorld* World = BattleController->GetWorld())
	{
		const FString ActionName = bSkillAction ? TEXT("Skill") : TEXT("Basic");
		const FString ElementText = bSkillAction
			? FString::Printf(TEXT(" | %s×%.2f"), *UMelodiaCoreRulesLibrary::GetElementDisplayName(AttackElement).ToString(), ElementMultiplier)
			: TEXT("");
		const FString FollowUpText = bBreakFollowUpConsumed ? FString::Printf(TEXT(" | Follow %.0f"), FollowUpBonusDamage) : TEXT("");
		const FString DelayText = EnemyTurnDelayStacks > 0 ? FString::Printf(TEXT(" | Delay %d"), EnemyTurnDelayStacks) : TEXT("");
		const FString StatusText = FString::Printf(TEXT("%s %.0f%s%s | HP %.0f/%.0f | Break %.0f%%%s | SP %d/%d | Ult %.0f%%"), *ActionName, Damage, *ElementText, *FollowUpText, RemainingHP, MaxHP, (EnemyToughness / EnemyToughnessMax) * 100.0f, *DelayText, NewSkillPoints, SkillPointMax, (NewUltimateGauge / UltimateMax) * 100.0f);
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
		// Floating combat text colour-grades by action so hits read at a glance.
		const FLinearColor DamageTextTint = bBreakFollowUpConsumed
			? FLinearColor(0.98f, 0.82f, 0.34f, 1.0f)
			: (bSkillAction ? FLinearColor(0.62f, 0.92f, 1.0f, 1.0f) : FLinearColor(1.0f, 0.74f, 0.86f, 1.0f));
		ForEachRhythmHUD(World, [StatusText, NewUltimateGauge, UltimateMax, bUltimateReady, RemainingHP, MaxHP, Damage, NewSkillPoints, SkillPointMax, ActionPrompt, EnemyToughness, EnemyToughnessMax, bEnemyBroken, bBreakFollowUpAvailable, bBreakFollowUpConsumed, FollowUpBonusDamage, EnemyTurnDelayStacks, LastEnemyTurnDelay, DamageTextTint](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetBattlePhaseBanner(TEXT("Player Turn"));
			Widget.SetUltimateGauge(NewUltimateGauge, UltimateMax, bUltimateReady);
			Widget.SetSkillPoints(NewSkillPoints, SkillPointMax);
			Widget.SetEnemyVitals(RemainingHP, MaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, bEnemyBroken);
			Widget.SetBreakFollowUpWindow(bBreakFollowUpAvailable, bBreakFollowUpConsumed, FollowUpBonusDamage);
			Widget.SetEnemyTurnDelay(EnemyTurnDelayStacks, LastEnemyTurnDelay);
			Widget.ShowBattleStatus(StatusText);
			Widget.ShowActionPrompt(ActionPrompt);
			Widget.TriggerDamageFlash(Damage);
			Widget.PushFloatingCombatText(FString::Printf(TEXT("-%.0f"), Damage), true, DamageTextTint);
			if (bBreakFollowUpConsumed && FollowUpBonusDamage > 0.0f)
			{
				Widget.PushFloatingCombatText(FString::Printf(TEXT("FOLLOW +%.0f"), FollowUpBonusDamage), true, FLinearColor(0.98f, 0.88f, 0.42f, 1.0f));
			}
			if (bEnemyBroken)
			{
				Widget.PushFloatingCombatText(TEXT("BREAK!"), true, FLinearColor(1.0f, 0.84f, 0.36f, 1.0f));
			}
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
		ExecuteEnemyTurn(WorldContextObject, BattleController);
		return true;
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

bool UMelodiaBattleLoopLibrary::ApplyRhythmExecutionResult(UObject* WorldContextObject, AActor* BattleController, const TArray<FMelodiaHighwayNote>& NoteResults, const bool bSkillAction, const float SkillScalar, const int32 SkillCost)
{
	if (!BattleController || NoteResults.Num() == 0)
	{
		return false;
	}

	float MultiplierSum = 0.0f;
	int32 ComboIndex = 0;
	for (const FMelodiaHighwayNote& Note : NoteResults)
	{
		MultiplierSum += UMelodiaCoreRulesLibrary::GetRhythmCombatMultiplier(Note.Grade, ComboIndex);
		if (Note.bCountsAsHit)
		{
			++ComboIndex;
		}
	}

	const float AverageGrade = MultiplierSum / static_cast<float>(NoteResults.Num());
	const float EffectiveGrade = AverageGrade * (bSkillAction ? SkillScalar : 1.0f);

	EMelodiaSpellElement AttackElement = EMelodiaSpellElement::Forte;
	if (const UMelodiaRhythmExecutionComponent* Execution = Cast<UMelodiaRhythmExecutionComponent>(WorldContextObject))
	{
		AttackElement = Execution->ActiveSpell.SpellElement;
	}
	else if (BattleController)
	{
		if (const UMelodiaRhythmExecutionComponent* BattleExecution = BattleController->FindComponentByClass<UMelodiaRhythmExecutionComponent>())
		{
			AttackElement = BattleExecution->ActiveSpell.SpellElement;
		}
	}

	return ApplyRhythmBattleAction(WorldContextObject, BattleController, EffectiveGrade, ComboIndex, bSkillAction, AttackElement);
}

bool UMelodiaBattleLoopLibrary::TryFleeRhythmBattle(UObject* WorldContextObject, AActor* BattleController)
{
	if (!BattleController)
	{
		return false;
	}

	if (UMelodiaRhythmExecutionComponent* Execution = BattleController->FindComponentByClass<UMelodiaRhythmExecutionComponent>())
	{
		if (Execution->IsExecutionActive())
		{
			Execution->CancelExecution();
		}
	}

	const bool bPhoenixFled = UMelodiaJRPGBridgeLibrary::TryFleeBattle(BattleController);
	CleanupBattlePresentation(BattleController);
	PublishLoopPhase(BattleController->GetWorld(), EMelodiaLoopPhase::ExplorationReady);

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [bPhoenixFled](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.bDrawExplorationHUD = true;
			Widget.SetNoteHighwayActive(false, TArray<FMelodiaHighwayNote>(), 0.0f, 2.5f);
			Widget.SetBattlePhaseBanner(TEXT("Fled"));
			Widget.ShowBattleStatus(bPhoenixFled ? TEXT("Fled battle") : TEXT("Retreated to explore"));
			Widget.ShowActionPrompt(TEXT("Explore: walk to the song gate"));
		});
	}

	return true;
}

bool UMelodiaBattleLoopLibrary::ExecuteEnemyTurn(UObject* WorldContextObject, AActor* BattleController)
{
	if (!BattleController || BattleController->Tags.Contains(RhythmVictoryResolvedTag))
	{
		return false;
	}

	const float EnemyHP = GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), 0.0f);
	if (EnemyHP <= 0.0f)
	{
		return false;
	}

	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	if (!CombatState)
	{
		return false;
	}

	if (CombatState->EnemyTurnDelayStacks > 0)
	{
		CombatState->ConsumeEnemyTurnDelay();
		PublishReactiveCommandState(BattleController, TEXT("Enemy Turn"), TEXT("Delayed"), 0.0f, false, false);
		if (UWorld* World = BattleController->GetWorld())
		{
			ForEachRhythmHUD(World, [CombatState](UMelodiaRhythmHUDWidget& Widget)
			{
				Widget.SetBattlePhaseBanner(TEXT("Enemy Turn"));
				Widget.ShowActionPrompt(FString::Printf(TEXT("Enemy delayed | %d stacks left"), CombatState->EnemyTurnDelayStacks));
				Widget.PushFloatingCombatText(TEXT("DELAYED"), true, FLinearColor(0.74f, 0.90f, 1.0f, 1.0f));
			});
		}
		return true;
	}

	const float IntentPower = FMath::Max(4.0f, CombatState->EnemyIntentPower > 0.0f ? CombatState->EnemyIntentPower : 8.0f);
	const float PartyDamage = CombatState->ApplyPartyDamage(IntentPower);
	++CombatState->EnemyTurnCount;

	// Bridge: mirror the enemy attack onto the real JRPG party unit(s) and adopt their aggregate HP as authoritative.
	const FMelodiaJRPGVitals EnemyTurnVitals = UMelodiaJRPGBridgeLibrary::ReadJRPGVitals(BattleController);
	if (EnemyTurnVitals.bHasParty)
	{
		const float SyncedPartyHP = UMelodiaJRPGBridgeLibrary::DamageParty(BattleController, PartyDamage);
		if (SyncedPartyHP >= 0.0f)
		{
			CombatState->PartyMaxHP = FMath::Max(1.0f, EnemyTurnVitals.PartyMaxHP);
			CombatState->PartyHP = FMath::Clamp(SyncedPartyHP, 0.0f, CombatState->PartyMaxHP);
		}
	}

	PublishReactiveCommandState(BattleController, TEXT("Enemy Turn"), TEXT("Slime Strike"), IntentPower, false, false);

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [CombatState, PartyDamage](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetBattlePhaseBanner(TEXT("Enemy Turn"));
			Widget.SetPartyVitals(CombatState->PartyHP, CombatState->PartyMaxHP);
			Widget.TriggerDamageFlash(PartyDamage);
			Widget.PushFloatingCombatText(FString::Printf(TEXT("-%.0f"), PartyDamage), false, FLinearColor(1.0f, 0.44f, 0.52f, 1.0f));
			Widget.ShowActionPrompt(FString::Printf(TEXT("Slime hit you for %.0f"), PartyDamage));
			Widget.SetJudgmentString(FString::Printf(TEXT("Enemy turn -%.0f HP"), PartyDamage));
		});
	}

	if (IsPartyDefeated(BattleController))
	{
		if (UWorld* DefeatWorld = BattleController->GetWorld())
		{
			ForEachRhythmHUD(DefeatWorld, [](UMelodiaRhythmHUDWidget& Widget)
			{
				Widget.SetBattlePhaseBanner(TEXT("Defeat"));
				Widget.ShowBattleStatus(TEXT("Party defeated"));
				Widget.ShowActionPrompt(TEXT("Returning to explore..."));
			});

			if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(DefeatWorld))
			{
				Session->EndEncounter(EMelodiaEncounterResult::Defeat);
				return true;
			}
		}

		UMelodiaJRPGBridgeLibrary::RestorePartyVitals(BattleController);
		return true;
	}

	if (CombatState)
	{
		CombatState->RecordCommandState(TEXT("Waiting"), TEXT("Slime Strike"), 8.0f, false, false);
	}

	if (UWorld* World = BattleController->GetWorld())
	{
		ForEachRhythmHUD(World, [](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetBattlePhaseBanner(TEXT("Player Turn"));
			Widget.ShowActionPrompt(TEXT("Your turn | 1=Attack | 2=Skill | R=Ultimate | Tab=cycle skill"));
		});
	}

	return true;
}

bool UMelodiaBattleLoopLibrary::IsPartyDefeated(AActor* BattleController)
{
	const UMelodiaCombatStateComponent* CombatState = BattleController ? BattleController->FindComponentByClass<UMelodiaCombatStateComponent>() : nullptr;
	if (CombatState)
	{
		return CombatState->PartyHP <= 0.0f;
	}

	return GetFloatPropertyValue(BattleController, TEXT("RhythmPartyHP"), 100.0f) <= 0.0f;
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
	float RemainingHP = FMath::Max(0.0f, CurrentHP - UltimateDamage);
	// Bridge: mirror ultimate damage onto the real JRPG enemy unit(s).
	if (UMelodiaJRPGBridgeLibrary::HasJRPGUnits(BattleController))
	{
		const float SyncedEnemyHP = UMelodiaJRPGBridgeLibrary::DamageActiveEnemy(BattleController, UltimateDamage);
		if (SyncedEnemyHP >= 0.0f)
		{
			RemainingHP = SyncedEnemyHP;
		}
	}

	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	if (CombatState)
	{
		CombatState->RecordUltimateActivated(UltimateDamage);
		const bool bUltBreak = CombatState->ApplyEnemyToughnessDamage(CombatState->EnemyToughnessMax);
		CombatState->ApplyEnemyTurnDelay(2);
		if (bUltBreak)
		{
			NotifyQuestManagers(BattleController->GetWorld(), [](AMelodiaQuestManagerBase& QuestManager)
			{
				QuestManager.NotifyEnemyBroken();
			});
		}

		FMelodiaCombatLegacyOverrides Overrides;
		Overrides.bOverrideLastDamage = true;
		Overrides.LastDamage = UltimateDamage;
		Overrides.bOverrideLastUltimateDamage = true;
		Overrides.LastUltimateDamage = UltimateDamage;
		Overrides.bOverrideEnemyHP = true;
		Overrides.EnemyHP = RemainingHP;
		Overrides.bOverrideEnemyMaxHP = true;
		Overrides.EnemyMaxHP = MaxHP;
		Overrides.bOverrideLastToughnessDamage = true;
		Overrides.LastToughnessDamage = CombatState->EnemyToughnessMax;
		Overrides.bOverrideLastFollowUpBonusDamage = true;
		Overrides.LastFollowUpBonusDamage = 0.0f;
		UMelodiaCombatSyncLibrary::MirrorCombatStateToLegacyProperties(BattleController, CombatState, Overrides);
	}
	else
	{
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), UltimateDamage);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastUltimateDamage"), UltimateDamage);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), RemainingHP);
		SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f);
		SetBoolPropertyValue(BattleController, TEXT("bRhythmUltimateReady"), false);
		SetIntPropertyValue(BattleController, TEXT("RhythmUltimateActivationCount"), GetIntPropertyValue(BattleController, TEXT("RhythmUltimateActivationCount"), 0) + 1);
	}

	BattleController->Tags.Remove(RhythmUltimateReadyTag);
	PublishReactiveCommandState(BattleController, TEXT("Ultimate"), TEXT("Interrupted"), UltimateDamage, false, true);

	const float EnemyToughness = CombatState ? CombatState->EnemyToughness : 0.0f;
	const float EnemyToughnessMax = CombatState ? FMath::Max(1.0f, CombatState->EnemyToughnessMax) : 100.0f;
	const bool bEnemyBroken = CombatState && CombatState->bEnemyBroken;
	const bool bBreakFollowUpAvailable = CombatState && CombatState->bBreakFollowUpAvailable && !CombatState->bBreakFollowUpConsumed;
	const int32 EnemyTurnDelayStacks = CombatState ? CombatState->EnemyTurnDelayStacks : 2;
	const int32 LastEnemyTurnDelay = CombatState ? CombatState->LastEnemyTurnDelay : 2;

	if (UWorld* World = BattleController->GetWorld())
	{
		const int32 SkillPoints = GetRhythmSkillPoints(BattleController);
		const int32 SkillPointMax = GetRhythmSkillPointMax(BattleController);
		ForEachRhythmHUD(World, [UltimateDamage, RemainingHP, MaxHP, SkillPoints, SkillPointMax, EnemyToughness, EnemyToughnessMax, bEnemyBroken, bBreakFollowUpAvailable, EnemyTurnDelayStacks, LastEnemyTurnDelay](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.SetBattlePhaseBanner(TEXT("Ultimate"));
			Widget.SetUltimateGauge(0.0f, 100.0f, false);
			Widget.SetSkillPoints(SkillPoints, SkillPointMax);
			Widget.SetEnemyVitals(RemainingHP, MaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, bEnemyBroken);
			Widget.ShowUltimateActivated(UltimateDamage);
			Widget.PushFloatingCombatText(FString::Printf(TEXT("-%.0f"), UltimateDamage), true, FLinearColor(1.0f, 0.86f, 0.34f, 1.0f));
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
	else
	{
		ExecuteEnemyTurn(WorldContextObject, BattleController);
	}

	return true;
}

void UMelodiaBattleLoopLibrary::ResetRhythmBattleEncounter(AActor* BattleController, const int32 OverrideEncounterLevel)
{
	if (!BattleController)
	{
		return;
	}

	const float MaxHP = FMath::Max(1.0f, GetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), 45.0f));
	SetFloatPropertyValue(BattleController, TEXT("CurrentRhythmMultiplier"), 1.0f);
	UMelodiaCombatStateComponent* CombatState = FindOrCreateCombatState(BattleController);
	int32 EncounterLevel = 1;
	if (OverrideEncounterLevel > 0)
	{
		EncounterLevel = FMath::Clamp(OverrideEncounterLevel, 1, 30);
	}
	else if (UGameInstance* LevelGI = BattleController->GetGameInstance())
	{
		if (const UMelodiaMechanicProgressionSubsystem* Progression = LevelGI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			EncounterLevel = FMath::Clamp(Progression->GetMechanicLevel(), 1, 30);
		}
	}
	if (CombatState)
	{
		CombatState->ResetUltimateGauge();
		CombatState->ResetActionEconomy();
		CombatState->PartyHP = CombatState->PartyMaxHP;
		CombatState->LastPartyDamageTaken = 0.0f;
		CombatState->EnemyTurnCount = 0;

		UGameInstance* GI = BattleController->GetGameInstance();
		if (GI)
		{
			if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				CombatState->EquippedKeyElement = Progression->State.EquippedKeyElement;
				CombatState->bCompanionActive = Progression->State.bCompanionUnlocked;
			}
		}
		const float LevelScale = 1.0f + static_cast<float>(EncounterLevel - 1) * 0.075f;
		CombatState->EnemyToughnessMax = 80.0f + EncounterLevel * 5.0f;
		CombatState->EnemyToughness = CombatState->EnemyToughnessMax;
		CombatState->EnemyIntentPower = 4.5f + EncounterLevel * 1.25f;
		CombatState->EnemyElement = UMelodiaKeySystemLibrary::GetEnemyElementForEncounterLevel(EncounterLevel);

		if (UMelodiaPartySubsystem* Party = GI ? GI->GetSubsystem<UMelodiaPartySubsystem>() : nullptr)
		{
			Party->SyncFromProgression();
			Party->ApplyBattleStartBuffs(CombatState);
		}
		else
		{
			for (TActorIterator<AMelodiaCompanionActor> It(BattleController->GetWorld()); It; ++It)
			{
				if (It->bCompanionUnlocked)
				{
					It->ApplyBattleStartBuff(CombatState);
					CombatState->bCompanionActive = true;
					break;
				}
			}
		}
	}

	// Bridge: seed the native rhythm model from the live JRPG template units so HUD/HP match from the first beat.
	const FMelodiaJRPGVitals SeedVitals = UMelodiaJRPGBridgeLibrary::ReadJRPGVitals(BattleController);
	const float LevelScale = 1.0f + static_cast<float>(EncounterLevel - 1) * 0.075f;
	const float ScaledMaxHP = MaxHP * LevelScale;
	float DisplayMaxHP = ScaledMaxHP;
	FMelodiaCombatLegacyOverrides ResetOverrides;
	ResetOverrides.bOverrideLastDamage = true;
	ResetOverrides.bOverrideLastUltimateDamage = true;
	ResetOverrides.bOverrideLastToughnessDamage = true;
	ResetOverrides.bOverrideLastFollowUpBonusDamage = true;
	ResetOverrides.bOverrideLastPartyDamage = true;
	if (SeedVitals.bHasEnemies && SeedVitals.EnemyMaxHP > 0.0f)
	{
		const float ScaledEnemyMax = SeedVitals.EnemyMaxHP * LevelScale;
		ResetOverrides.bOverrideEnemyMaxHP = true;
		ResetOverrides.EnemyMaxHP = ScaledEnemyMax;
		ResetOverrides.bOverrideEnemyHP = true;
		ResetOverrides.EnemyHP = ScaledEnemyMax;
		DisplayMaxHP = ScaledEnemyMax;
		UMelodiaJRPGBridgeLibrary::SetActiveEnemyHP(BattleController, ScaledEnemyMax);
	}
	else
	{
		ResetOverrides.bOverrideEnemyMaxHP = true;
		ResetOverrides.EnemyMaxHP = ScaledMaxHP;
		ResetOverrides.bOverrideEnemyHP = true;
		ResetOverrides.EnemyHP = ScaledMaxHP;
	}
	if (SeedVitals.bHasParty && SeedVitals.PartyMaxHP > 0.0f && CombatState)
	{
		CombatState->PartyMaxHP = FMath::Max(1.0f, SeedVitals.PartyMaxHP);
		CombatState->PartyHP = FMath::Clamp(SeedVitals.PartyHP > 0.0f ? SeedVitals.PartyHP : SeedVitals.PartyMaxHP, 1.0f, CombatState->PartyMaxHP);
	}

	if (CombatState)
	{
		UMelodiaCombatSyncLibrary::MirrorCombatStateToLegacyProperties(BattleController, CombatState, ResetOverrides);
	}
	else
	{
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyHP"), ResetOverrides.EnemyHP);
		SetFloatPropertyValue(BattleController, TEXT("RhythmEnemyMaxHP"), ResetOverrides.EnemyMaxHP);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastDamage"), 0.0f);
		SetFloatPropertyValue(BattleController, TEXT("RhythmLastUltimateDamage"), 0.0f);
		SetFloatPropertyValue(BattleController, TEXT("RhythmUltimateGauge"), 0.0f);
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
	}

	SetIntPropertyValue(BattleController, TEXT("RhythmCombo"), 0);
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
		ForEachRhythmHUD(World, [DisplayMaxHP, SkillPoints, SkillPointMax, EnemyToughness, EnemyToughnessMax, CombatState, EncounterLevel](UMelodiaRhythmHUDWidget& Widget)
		{
			Widget.bDrawExplorationHUD = false;
			Widget.ClearBattleStatus();
			Widget.SetEnemyVitals(DisplayMaxHP, DisplayMaxHP);
			Widget.SetEnemyBreakGauge(EnemyToughness, EnemyToughnessMax, false);
			Widget.SetBreakFollowUpWindow(false, false, 0.0f);
			Widget.SetEnemyTurnDelay(0, 0);
			Widget.SetSkillPoints(SkillPoints, SkillPointMax);
			if (CombatState)
			{
				Widget.SetPartyVitals(CombatState->PartyHP, CombatState->PartyMaxHP);
			}
			Widget.SetNoteHighwayActive(false, TArray<FMelodiaHighwayNote>(), 0.0f, 2.5f);
			Widget.SetBattlePhaseBanner(TEXT("Player Turn"));
			const FString EnemyElementName = CombatState
				? UMelodiaCoreRulesLibrary::GetElementDisplayName(CombatState->EnemyElement).ToString()
				: TEXT("?");
			Widget.ShowActionPrompt(FString::Printf(
				TEXT("Weakness: %s | 1=Attack | 2=Skill | R=Ultimate | Tab=cycle skill"),
				*EnemyElementName));
			Widget.ShowBattleStatus(FString::Printf(TEXT("Battle Lv%d — break the toughness bar for bonus damage"), EncounterLevel));
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
	CleanupBattlePresentation(BattleController);
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
		UMelodiaCombatSyncLibrary::MirrorCombatStateToLegacyProperties(BattleController, CombatState, FMelodiaCombatLegacyOverrides());
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
			Widget.SetBattlePhaseBanner(TEXT("Victory"));
			Widget.ShowVictoryReward(TEXT("Victory! Returning to explore..."));
			Widget.ShowActionPrompt(TEXT("Victory — Space to continue now, or wait"));
			Widget.PushFloatingCombatText(TEXT("VICTORY"), true, FLinearColor(1.0f, 0.86f, 0.34f, 1.0f));
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
		const UMelodiaCombatStateComponent* CombatState = BattleController->FindComponentByClass<UMelodiaCombatStateComponent>();
		const bool bWonWithUltimate = CombatState && CombatState->bUltimateUsedThisBattle;

		for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
		{
			It->NotifyBattleWon();
			if (bWonWithUltimate)
			{
				It->NotifyUltimateVictory();
			}
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
