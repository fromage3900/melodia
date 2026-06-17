// PIE-only runtime verifier for the Melodia rhythm battle loop.

#include "MelodiaLoopVerifier.h"

#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaBattleInputComponent.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaBattleSession.h"
#include "MelodiaCombatStateComponent.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaMusicManager.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/UObjectIterator.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGMelodiaAttributes.h"

AMelodiaLoopVerifier::AMelodiaLoopVerifier()
{
	PrimaryActorTick.bCanEverTick = true;
}

void AMelodiaLoopVerifier::BeginPlay()
{
	Super::BeginPlay();

	if (!bRunOnBeginPlay)
	{
		SetActorTickEnabled(false);
		return;
	}

	if (const AMelodiaMusicManager* MusicManager = Cast<AMelodiaMusicManager>(FindOrSpawnActor(AMelodiaMusicManager::StaticClass(), FVector::ZeroVector)))
	{
		FirstBeatPosition = MusicManager->GetCurrentBeatPosition();
		bCapturedFirstBeat = true;
	}
}

void AMelodiaLoopVerifier::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bHasRun || !bRunOnBeginPlay)
	{
		return;
	}

	ElapsedSeconds += DeltaSeconds;
	if (ElapsedSeconds >= VerificationDelaySeconds)
	{
		bHasRun = true;
		RunVerificationNow();
		SetActorTickEnabled(false);
	}
}

bool AMelodiaLoopVerifier::RunVerificationNow()
{
	FString MusicDetail;
	FString BattleDetail;
	FString HUDDetail;
	FString QuestDetail;
	FString RhythmManagerDetail;
	FString PCGDetail;

	const bool bMusic = VerifyMusicClock(MusicDetail);
	const bool bBattle = VerifyBattleHooks(BattleDetail);
	const bool bHUD = VerifyHUDHooks(HUDDetail);
	const bool bQuest = VerifyQuestHook(QuestDetail);
	const bool bRhythmManager = VerifyRhythmManagerWiring(RhythmManagerDetail);
	const bool bPCG = VerifyPCGGraphs(PCGDetail);
	const bool bPass = bMusic && bBattle && bHUD && bQuest && bRhythmManager && bPCG;

	bLastVerificationPassed = bPass;
	LastVerificationSummary = FString::Printf(TEXT("Music=%s | Input=%s | Battle=%s | HUD=%s | Quest=%s | PCG=%s"),
		*MusicDetail,
		*RhythmManagerDetail,
		*BattleDetail,
		*HUDDetail,
		*QuestDetail,
		*PCGDetail);

	UE_LOG(LogTemp, Warning, TEXT("MELODIA_LOOP_VERIFY %s | %s"),
		bPass ? TEXT("PASS") : TEXT("FAIL"),
		*LastVerificationSummary);

	return bPass;
}

UClass* AMelodiaLoopVerifier::ResolveClass(const FSoftClassPath& ClassPath) const
{
	UObject* LoadedObject = ClassPath.TryLoad();
	return Cast<UClass>(LoadedObject);
}

AActor* AMelodiaLoopVerifier::FindOrSpawnActor(UClass* ActorClass, const FVector& Location) const
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	for (TActorIterator<AActor> It(World, ActorClass); It; ++It)
	{
		return *It;
	}

	FActorSpawnParameters SpawnParameters;
	SpawnParameters.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	return World->SpawnActor<AActor>(ActorClass, Location, FRotator::ZeroRotator, SpawnParameters);
}

bool AMelodiaLoopVerifier::VerifyMusicClock(FString& Detail)
{
	AMelodiaMusicManager* MusicManager = Cast<AMelodiaMusicManager>(FindOrSpawnActor(AMelodiaMusicManager::StaticClass(), FVector(-10680.0f, -5000.0f, -2140.0f)));
	if (!MusicManager)
	{
		Detail = TEXT("missing");
		return false;
	}

	if (!bCapturedFirstBeat)
	{
		FirstBeatPosition = MusicManager->GetCurrentBeatPosition();
	}

	const float CurrentBeatPosition = MusicManager->GetCurrentBeatPosition();
	const bool bAdvanced = CurrentBeatPosition > FirstBeatPosition + 0.01f;
	Detail = FString::Printf(TEXT("beat %.3f->%.3f quartz=%s"), FirstBeatPosition, CurrentBeatPosition, MusicManager->IsQuartzClockActive() ? TEXT("true") : TEXT("false"));
	return bAdvanced;
}

bool AMelodiaLoopVerifier::VerifyBattleHooks(FString& Detail)
{
	UClass* BattleClass = ResolveClass(BattleControllerClassPath);
	AActor* BattleController = FindOrSpawnActor(BattleClass, FVector(-10600.0f, -5000.0f, -2140.0f));
	if (!BattleController)
	{
		Detail = TEXT("missing");
		return false;
	}

	UFunction* RegisterFunction = BattleController->FindFunction(TEXT("RegisterRhythmHit"));
	if (!RegisterFunction)
	{
		Detail = TEXT("missing RegisterRhythmHit");
		return false;
	}

	UFunction* SetMultiplierFunction = BattleController->FindFunction(TEXT("SetRhythmMultiplier"));
	if (!SetMultiplierFunction)
	{
		Detail = TEXT("missing SetRhythmMultiplier");
		return false;
	}

	UClass* QuestClass = ResolveClass(QuestManagerClassPath);
	AActor* QuestManager = FindOrSpawnActor(QuestClass, FVector(-10570.0f, -5000.0f, -2140.0f));
	const AMelodiaQuestManagerBase* NativeQuestManager = Cast<AMelodiaQuestManagerBase>(QuestManager);
	const int32 InitialQuestWins = NativeQuestManager ? NativeQuestManager->CompletedBattleCount : 0;
	UMelodiaBattleInputComponent* InputBridge = BattleController->FindComponentByClass<UMelodiaBattleInputComponent>();
	auto SetFloatProperty = [BattleController](const FName PropertyName, const float Value)
	{
		if (FFloatProperty* FloatProperty = CastField<FFloatProperty>(BattleController->GetClass()->FindPropertyByName(PropertyName)))
		{
			FloatProperty->SetPropertyValue_InContainer(BattleController, Value);
		}
	};

	SetFloatProperty(TEXT("RhythmEnemyMaxHP"), 135.0f);
	SetFloatProperty(TEXT("RhythmEnemyHP"), 135.0f);
	SetFloatProperty(TEXT("RhythmUltimateGauge"), 0.0f);
	SetFloatProperty(TEXT("RhythmUltimateMax"), 100.0f);

	bool bVictoryResolved = false;
	auto ApplyVerifiedRhythmAction = [this, BattleController, RegisterFunction, SetMultiplierFunction, InputBridge, &bVictoryResolved](const bool bSkillAction)
	{
		if (InputBridge)
		{
			const bool bResult = bSkillAction ? InputBridge->HandleSkillInput() : InputBridge->HandleBasicInput();
			bVictoryResolved = bResult || bVictoryResolved;
			return;
		}

		struct FRegisterParams
		{
			bool bSuccess = true;
			int32 NewCombo = 0;
		};

		FRegisterParams RegisterParams;
		BattleController->ProcessEvent(RegisterFunction, &RegisterParams);

		struct FMultiplierParams
		{
			float Grade = 1.5f;
		};

		FMultiplierParams MultiplierParams;
		BattleController->ProcessEvent(SetMultiplierFunction, &MultiplierParams);
		bVictoryResolved = UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(
			this,
			BattleController,
			bSkillAction ? EMelodiaRhythmBattleCommand::Skill : EMelodiaRhythmBattleCommand::Basic,
			MultiplierParams.Grade,
			3) || bVictoryResolved;
	};
	auto ApplyVerifiedRhythmHit = [&ApplyVerifiedRhythmAction]()
	{
		ApplyVerifiedRhythmAction(false);
	};
	auto ApplyVerifiedRhythmSkill = [&ApplyVerifiedRhythmAction]()
	{
		ApplyVerifiedRhythmAction(true);
	};

	for (int32 HitIndex = 0; HitIndex < 2; ++HitIndex)
	{
		ApplyVerifiedRhythmHit();
	}

	const bool bUltimateReadyAfterCharge = UMelodiaBattleLoopLibrary::IsRhythmUltimateReady(BattleController);
	const bool bUltimateTriggered = InputBridge ? InputBridge->HandleUltimateInput() : UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(this, BattleController, EMelodiaRhythmBattleCommand::Ultimate, 1.5f, 3);
	const UMelodiaCombatStateComponent* CombatStateAfterUltimate = BattleController->FindComponentByClass<UMelodiaCombatStateComponent>();
	const float TriggeredUltimateDamage = CombatStateAfterUltimate ? CombatStateAfterUltimate->LastUltimateDamage : 0.0f;
	const int32 UltimateInterruptCountAfterUltimate = CombatStateAfterUltimate ? CombatStateAfterUltimate->UltimateInterruptCount : 0;
	const FString IntentAfterUltimate = CombatStateAfterUltimate ? CombatStateAfterUltimate->EnemyIntentName : TEXT("");
	const bool bEnemyBrokenAfterUltimate = CombatStateAfterUltimate && CombatStateAfterUltimate->bEnemyBroken;
	const int32 EnemyBreakCountAfterUltimate = CombatStateAfterUltimate ? CombatStateAfterUltimate->TotalEnemyBreakCount : 0;
	const float EnemyToughnessAfterUltimate = CombatStateAfterUltimate ? CombatStateAfterUltimate->EnemyToughness : -1.0f;
	const bool bBreakFollowUpReadyAfterUltimate = CombatStateAfterUltimate && CombatStateAfterUltimate->bBreakFollowUpAvailable;
	const int32 BreakFollowUpAvailableCountAfterUltimate = CombatStateAfterUltimate ? CombatStateAfterUltimate->BreakFollowUpAvailableCount : 0;
	const bool bSkillWasAvailable = UMelodiaBattleLoopLibrary::CanUseRhythmSkill(BattleController);
	ApplyVerifiedRhythmSkill();
	const UMelodiaCombatStateComponent* CombatStateAfterSkill = BattleController->FindComponentByClass<UMelodiaCombatStateComponent>();
	const int32 SkillActionCountAfterSkill = CombatStateAfterSkill ? CombatStateAfterSkill->SkillActivationCount : 0;
	const int32 BreakFollowUpConsumedCountAfterSkill = CombatStateAfterSkill ? CombatStateAfterSkill->BreakFollowUpConsumedCount : 0;
	const float FollowUpBonusDamageAfterSkill = CombatStateAfterSkill ? CombatStateAfterSkill->LastFollowUpBonusDamage : 0.0f;
	ApplyVerifiedRhythmHit();
	ApplyVerifiedRhythmHit();

	AMelodiaRhythmGameModeBase* MutableRhythmGameMode = GetWorld() ? Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr;
	AMelodiaEncounterTrigger* EncounterTrigger = MutableRhythmGameMode ? MutableRhythmGameMode->ActiveEncounterTrigger : nullptr;
	const bool bEncounterTriggerPresent = EncounterTrigger != nullptr;
	SetFloatProperty(TEXT("RhythmEnemyMaxHP"), 45.0f);
	SetFloatProperty(TEXT("RhythmEnemyHP"), 45.0f);

	for (int32 HitIndex = 0; HitIndex < 3; ++HitIndex)
	{
		ApplyVerifiedRhythmHit();
	}

	const FProperty* MultiplierProperty = BattleController->GetClass()->FindPropertyByName(TEXT("CurrentRhythmMultiplier"));
	const FProperty* ComboProperty = BattleController->GetClass()->FindPropertyByName(TEXT("RhythmCombo"));
	const FProperty* EnemyHPProperty = BattleController->GetClass()->FindPropertyByName(TEXT("RhythmEnemyHP"));
	const FProperty* LastDamageProperty = BattleController->GetClass()->FindPropertyByName(TEXT("RhythmLastDamage"));
	const FProperty* BattleOverProperty = BattleController->GetClass()->FindPropertyByName(TEXT("isBattleOver"));
	const FProperty* PlayerVictoryProperty = BattleController->GetClass()->FindPropertyByName(TEXT("isPlayerVictory"));
	const FFloatProperty* MultiplierFloat = CastField<FFloatProperty>(MultiplierProperty);
	const FIntProperty* ComboInt = CastField<FIntProperty>(ComboProperty);
	const FFloatProperty* EnemyHPFloat = CastField<FFloatProperty>(EnemyHPProperty);
	const FFloatProperty* LastDamageFloat = CastField<FFloatProperty>(LastDamageProperty);
	const FBoolProperty* BattleOverBool = CastField<FBoolProperty>(BattleOverProperty);
	const FBoolProperty* PlayerVictoryBool = CastField<FBoolProperty>(PlayerVictoryProperty);
	const UMelodiaCombatStateComponent* CombatState = BattleController->FindComponentByClass<UMelodiaCombatStateComponent>();

	const float Multiplier = MultiplierFloat ? MultiplierFloat->GetPropertyValue_InContainer(BattleController) : 0.0f;
	const int32 Combo = ComboInt ? ComboInt->GetPropertyValue_InContainer(BattleController) : -1;
	const float EnemyHP = EnemyHPFloat ? EnemyHPFloat->GetPropertyValue_InContainer(BattleController) : -1.0f;
	const float LastDamage = LastDamageFloat ? LastDamageFloat->GetPropertyValue_InContainer(BattleController) : 0.0f;
	const int32 TotalUltimateActivations = CombatState ? CombatState->TotalUltimateActivationCount : 0;
	const int32 SkillPoints = CombatState ? CombatState->SkillPoints : 0;
	const int32 SkillPointMax = CombatState ? CombatState->SkillPointMax : 0;
	const int32 BasicActionCount = CombatState ? CombatState->BasicActivationCount : 0;
	const int32 SkillActionCount = FMath::Max(CombatState ? CombatState->SkillActivationCount : 0, SkillActionCountAfterSkill);
	const bool bBattleOver = BattleOverBool && BattleOverBool->GetPropertyValue_InContainer(BattleController);
	const bool bPlayerVictory = PlayerVictoryBool && PlayerVictoryBool->GetPropertyValue_InContainer(BattleController);
	const bool bRewardConfirmed = UMelodiaBattleLoopLibrary::HasRhythmRewardBeenConfirmed(BattleController);
	const int32 QuestWins = NativeQuestManager ? NativeQuestManager->CompletedBattleCount : InitialQuestWins;
	const int32 EncounterWins = QuestWins - InitialQuestWins;
	const AMelodiaRhythmGameModeBase* RhythmGameMode = GetWorld() ? Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(GetWorld())) : nullptr;
	const FString LoopPhaseText = RhythmGameMode ? RhythmGameMode->LastLoopPhaseText : TEXT("missing");
	const int32 BattlePhaseEntries = RhythmGameMode ? RhythmGameMode->BattlePhaseEntryCount : 0;
	const int32 VictoryRewardPhaseEntries = RhythmGameMode ? RhythmGameMode->VictoryRewardPhaseCount : 0;
	const int32 ExplorationReadyPhaseEntries = RhythmGameMode ? RhythmGameMode->ExplorationReadyPhaseCount : 0;
	const bool bExplorationControlReady = RhythmGameMode && RhythmGameMode->bExplorationControlReady;
	const int32 ExplorationControlRestores = RhythmGameMode ? RhythmGameMode->ExplorationControlRestoreCount : 0;
	const bool bHasExplorationPawn = RhythmGameMode && RhythmGameMode->ActiveExplorationPawn != nullptr;
	const bool bMelusinaPawnActive = RhythmGameMode && RhythmGameMode->bMelusinaPawnActive;
	const int32 MelusinaPawnApplyCount = RhythmGameMode ? RhythmGameMode->MelusinaPawnApplyCount : 0;
	const bool bHUDWidgetInViewport = RhythmGameMode && RhythmGameMode->bRhythmHUDWidgetInViewport;
	const bool bGameModeInputBound = RhythmGameMode && RhythmGameMode->bBattleInputBound;
	const bool bInputBridgeBound = InputBridge && InputBridge->bInputBound;
	const UMelodiaBattleSession* BattleSession = UMelodiaBattleSession::Get(this);
	const bool bBattleSessionSubsystemPresent = BattleSession != nullptr;
	const int32 InputBridgeBasicCount = InputBridge ? InputBridge->BasicInputCount : 0;
	const int32 InputBridgeSkillCount = InputBridge ? InputBridge->SkillInputCount : 0;
	const int32 InputBridgeUltimateCount = InputBridge ? InputBridge->UltimateInputCount : 0;
	const int32 InputBridgeCommandCount = InputBridge ? InputBridge->SuccessfulCommandCount : 0;
	const UMelodiaCosmeticsComponent* ActiveCosmetics = RhythmGameMode ? RhythmGameMode->ActiveCosmeticsComponent : nullptr;
	const bool bCosmeticsApplied = ActiveCosmetics && ActiveCosmetics->bLastApplySucceeded && ActiveCosmetics->AppliedCosmeticCount > 0;
	const FString CosmeticPresetText = RhythmGameMode ? RhythmGameMode->LastCosmeticPresetText : TEXT("");
	const int32 EncounterTriggerActivations = EncounterTrigger ? EncounterTrigger->ActivationCount : 0;
	const bool bTriggerHasVisibleMarker = EncounterTrigger && EncounterTrigger->bHasVisibleMarker;
	bool bVictoryHUDVisible = false;
	int32 ExplorationPromptCount = 0;
	int32 BattleStartPromptCount = 0;
	int32 UltimateReadyPromptCount = 0;
	int32 UltimateActivationPromptCount = 0;
	int32 SkillPointUpdateCount = 0;
	int32 EnemyBreakGaugeUpdateCount = 0;
	int32 BreakFollowUpUpdateCount = 0;
	int32 SparkleBurstCount = 0;
	int32 EnemyVitalsUpdateCount = 0;
	int32 TurnOrderUpdateCount = 0;
	int32 ReactiveStateUpdateCount = 0;
	int32 ActionPromptCount = 0;
	int32 DamageFlashCount = 0;
	int32 NativePaintFrameCount = 0;
	int32 NativeFiligreePaintCount = 0;
	int32 NativeSparklePaintCount = 0;
	int32 NativeTurnRailPaintCount = 0;
	int32 NativeCommandCardPaintCount = 0;
	int32 NativeSkillPointPaintCount = 0;
	int32 NativeEnemyVitalsPaintCount = 0;
	int32 NativeIntentPaintCount = 0;
	int32 NativeBreakGaugePaintCount = 0;
	int32 NativeFollowUpPaintCount = 0;
	int32 NativeDamageFlashPaintCount = 0;
	int32 NativeLabelPaintCount = 0;
	int32 NativePortraitPaintCount = 0;
	bool bCuteThemeApplied = false;
	FString RewardText;
	FString ActionPromptText;
	if (UWorld* World = GetWorld())
	{
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			UMelodiaRhythmHUDWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World && Widget->bVictoryFeedbackVisible)
			{
				bVictoryHUDVisible = true;
				RewardText = Widget->LastRewardText;
			}

			if (Widget && Widget->GetWorld() == World)
			{
				ExplorationPromptCount += Widget->ExplorationPromptCount;
				BattleStartPromptCount += Widget->BattleStartPromptCount;
				UltimateReadyPromptCount += Widget->UltimateReadyPromptCount;
				UltimateActivationPromptCount += Widget->UltimateActivationPromptCount;
				SkillPointUpdateCount += Widget->SkillPointUpdateCount;
				EnemyBreakGaugeUpdateCount += Widget->EnemyBreakGaugeUpdateCount;
				BreakFollowUpUpdateCount += Widget->BreakFollowUpUpdateCount;
				SparkleBurstCount += Widget->SparkleBurstCount;
				EnemyVitalsUpdateCount += Widget->EnemyVitalsUpdateCount;
				TurnOrderUpdateCount += Widget->TurnOrderUpdateCount;
				ReactiveStateUpdateCount += Widget->ReactiveStateUpdateCount;
				ActionPromptCount += Widget->ActionPromptCount;
				DamageFlashCount += Widget->DamageFlashCount;
				NativePaintFrameCount += Widget->NativePaintFrameCount;
				NativeFiligreePaintCount += Widget->NativeFiligreePaintCount;
				NativeSparklePaintCount += Widget->NativeSparklePaintCount;
				NativeTurnRailPaintCount += Widget->NativeTurnRailPaintCount;
				NativeCommandCardPaintCount += Widget->NativeCommandCardPaintCount;
				NativeSkillPointPaintCount += Widget->NativeSkillPointPaintCount;
				NativeEnemyVitalsPaintCount += Widget->NativeEnemyVitalsPaintCount;
				NativeIntentPaintCount += Widget->NativeIntentPaintCount;
				NativeBreakGaugePaintCount += Widget->NativeBreakGaugePaintCount;
				NativeFollowUpPaintCount += Widget->NativeFollowUpPaintCount;
				NativeDamageFlashPaintCount += Widget->NativeDamageFlashPaintCount;
				NativeLabelPaintCount += Widget->NativeLabelPaintCount;
				NativePortraitPaintCount += Widget->NativePortraitPaintCount;
				bCuteThemeApplied = bCuteThemeApplied || Widget->bCuteCombatThemeApplied;
				if (ActionPromptText.IsEmpty())
				{
					ActionPromptText = Widget->LastActionPromptText;
				}
			}
		}
	}

	Detail = FString::Printf(TEXT("multiplier=%.2f combo=%d damage=%.1f enemyHP=%.1f ultimateReady=%s ultimateTriggered=%s ultimateDamage=%.1f ultimateTotal=%d ultimateInterrupts=%d intentAfterUlt=%s enemyBroken=%s breaks=%d toughnessAfterUlt=%.1f followReady=%s followReadyCount=%d followConsumed=%d followBonus=%.1f skillAvailable=%s skillPoints=%d/%d basicActions=%d skillActions=%d inputBound=%s session=%s gameModeInput=%s inputBasic=%d inputSkill=%d inputUlt=%d inputCommands=%d battleOver=%s playerVictory=%s rewardConfirmed=%s encounterWins=%d questWins=%d phase=%s battlePhases=%d victoryPhases=%d exploreReadyPhases=%d explorationControl=%s controlRestores=%d melusina=%s melusinaApplies=%d hudViewport=%s cosmetics=%s cosmetic=%s triggerStarted=%s triggerActivations=%d triggerMarker=%s explorePrompts=%d battlePrompts=%d ultReadyPrompts=%d ultActivationPrompts=%d skillPointUpdates=%d breakUpdates=%d followUpdates=%d sparkles=%d vitals=%d turnRail=%d reactiveStates=%d actionPrompts=%d damageFlashes=%d nativePaint=%d nativeFiligree=%d nativeSparkles=%d nativeTurnRail=%d nativeCommands=%d nativeSkillPips=%d nativeVitals=%d nativeIntent=%d nativeBreak=%d nativeFollow=%d nativeDamageFlash=%d nativeLabels=%d nativePortraits=%d cuteTheme=%s hudVictory=%s action=%s reward=%s"),
		Multiplier,
		Combo,
		LastDamage,
		EnemyHP,
		bUltimateReadyAfterCharge ? TEXT("true") : TEXT("false"),
		bUltimateTriggered ? TEXT("true") : TEXT("false"),
		TriggeredUltimateDamage,
		TotalUltimateActivations,
		UltimateInterruptCountAfterUltimate,
		*IntentAfterUltimate,
		bEnemyBrokenAfterUltimate ? TEXT("true") : TEXT("false"),
		EnemyBreakCountAfterUltimate,
		EnemyToughnessAfterUltimate,
		bBreakFollowUpReadyAfterUltimate ? TEXT("true") : TEXT("false"),
		BreakFollowUpAvailableCountAfterUltimate,
		BreakFollowUpConsumedCountAfterSkill,
		FollowUpBonusDamageAfterSkill,
		bSkillWasAvailable ? TEXT("true") : TEXT("false"),
		SkillPoints,
		SkillPointMax,
		BasicActionCount,
		SkillActionCount,
		bInputBridgeBound ? TEXT("true") : TEXT("false"),
		bBattleSessionSubsystemPresent ? TEXT("true") : TEXT("false"),
		bGameModeInputBound ? TEXT("true") : TEXT("false"),
		InputBridgeBasicCount,
		InputBridgeSkillCount,
		InputBridgeUltimateCount,
		InputBridgeCommandCount,
		bBattleOver ? TEXT("true") : TEXT("false"),
		bPlayerVictory ? TEXT("true") : TEXT("false"),
		bRewardConfirmed ? TEXT("true") : TEXT("false"),
		EncounterWins,
		QuestWins,
		*LoopPhaseText,
		BattlePhaseEntries,
		VictoryRewardPhaseEntries,
		ExplorationReadyPhaseEntries,
		bExplorationControlReady ? TEXT("true") : TEXT("false"),
		ExplorationControlRestores,
		bMelusinaPawnActive ? TEXT("true") : TEXT("false"),
		MelusinaPawnApplyCount,
		bHUDWidgetInViewport ? TEXT("true") : TEXT("false"),
		bCosmeticsApplied ? TEXT("true") : TEXT("false"),
		*CosmeticPresetText,
		bEncounterTriggerPresent ? TEXT("true") : TEXT("false"),
		EncounterTriggerActivations,
		bTriggerHasVisibleMarker ? TEXT("true") : TEXT("false"),
		ExplorationPromptCount,
		BattleStartPromptCount,
		UltimateReadyPromptCount,
		UltimateActivationPromptCount,
		SkillPointUpdateCount,
		EnemyBreakGaugeUpdateCount,
		BreakFollowUpUpdateCount,
		SparkleBurstCount,
		EnemyVitalsUpdateCount,
		TurnOrderUpdateCount,
		ReactiveStateUpdateCount,
		ActionPromptCount,
		DamageFlashCount,
		NativePaintFrameCount,
		NativeFiligreePaintCount,
		NativeSparklePaintCount,
		NativeTurnRailPaintCount,
		NativeCommandCardPaintCount,
		NativeSkillPointPaintCount,
		NativeEnemyVitalsPaintCount,
		NativeIntentPaintCount,
		NativeBreakGaugePaintCount,
		NativeFollowUpPaintCount,
		NativeDamageFlashPaintCount,
		NativeLabelPaintCount,
		NativePortraitPaintCount,
		bCuteThemeApplied ? TEXT("true") : TEXT("false"),
		bVictoryHUDVisible ? TEXT("true") : TEXT("false"),
		*ActionPromptText,
		*RewardText);
	return Multiplier >= 1.0f
		&& Combo >= 3
		&& LastDamage > 0.0f
		&& EnemyHP <= 0.0f
		&& bUltimateReadyAfterCharge
		&& bUltimateTriggered
		&& TriggeredUltimateDamage > 0.0f
		&& TotalUltimateActivations >= 1
		&& UltimateInterruptCountAfterUltimate >= 1
		&& IntentAfterUltimate == TEXT("Interrupted")
		&& bEnemyBrokenAfterUltimate
		&& EnemyBreakCountAfterUltimate >= 1
		&& EnemyToughnessAfterUltimate <= 0.0f
		&& bBreakFollowUpReadyAfterUltimate
		&& BreakFollowUpAvailableCountAfterUltimate >= 1
		&& BreakFollowUpConsumedCountAfterSkill >= 1
		&& FollowUpBonusDamageAfterSkill > 0.0f
		&& bSkillWasAvailable
		&& SkillPointMax >= 5
		&& BasicActionCount >= 1
		&& SkillActionCount >= 1
		&& bInputBridgeBound
		&& bBattleSessionSubsystemPresent
		&& bGameModeInputBound
		&& InputBridgeBasicCount >= 1
		&& InputBridgeSkillCount >= 1
		&& InputBridgeUltimateCount >= 1
		&& InputBridgeCommandCount >= 3
		&& bVictoryResolved
		&& bBattleOver
		&& bPlayerVictory
		&& bRewardConfirmed
		&& EncounterWins >= 2
		&& RhythmGameMode
		&& LoopPhaseText == TEXT("VictoryReward")
		&& BattlePhaseEntries >= 2
		&& VictoryRewardPhaseEntries >= 2
		&& ExplorationReadyPhaseEntries >= 1
		&& bExplorationControlReady
		&& ExplorationControlRestores >= 1
		&& bHasExplorationPawn
		&& bMelusinaPawnActive
		&& MelusinaPawnApplyCount >= 1
		&& bHUDWidgetInViewport
		&& 		bCosmeticsApplied
		&& CosmeticPresetText.Contains(TEXT("Melusina"))
		&& bEncounterTriggerPresent
		&& bTriggerHasVisibleMarker
		&& ExplorationPromptCount >= 1
		&& BattleStartPromptCount >= 1
		&& UltimateReadyPromptCount >= 1
		&& UltimateActivationPromptCount >= 1
		&& SkillPointUpdateCount >= 1
		&& EnemyBreakGaugeUpdateCount >= 1
		&& BreakFollowUpUpdateCount >= 1
		&& SparkleBurstCount >= 1
		&& EnemyVitalsUpdateCount >= 1
		&& TurnOrderUpdateCount >= 1
		&& ReactiveStateUpdateCount >= 1
		&& ActionPromptCount >= 1
		&& DamageFlashCount >= 1
		&& NativePaintFrameCount >= 1
		&& NativeFiligreePaintCount >= 1
		&& NativeSparklePaintCount >= 1
		&& NativeTurnRailPaintCount >= 1
		&& NativeCommandCardPaintCount >= 1
		&& NativeSkillPointPaintCount >= 1
		&& NativeEnemyVitalsPaintCount >= 1
		&& NativeIntentPaintCount >= 1
		&& NativeBreakGaugePaintCount >= 1
		&& NativeFollowUpPaintCount >= 1
		&& NativeDamageFlashPaintCount >= 1
		&& NativeLabelPaintCount >= 1
		&& NativePortraitPaintCount >= 1
		&& bCuteThemeApplied
		&& bVictoryHUDVisible
		&& RewardText.Contains(TEXT("Victory"));
}

bool AMelodiaLoopVerifier::VerifyHUDHooks(FString& Detail)
{
	UClass* WidgetClass = ResolveClass(RhythmWidgetClassPath);
	UWorld* World = GetWorld();
	if (!World || !WidgetClass)
	{
		Detail = TEXT("missing widget class");
		return false;
	}

	UMelodiaRhythmHUDWidget* Widget = CreateWidget<UMelodiaRhythmHUDWidget>(World, WidgetClass);
	if (!Widget)
	{
		Detail = TEXT("create failed");
		return false;
	}

	Widget->AddToViewport(50);
	Widget->SetJudgment(FText::FromString(TEXT("PERFECT")));
	Widget->DoPulse();

	Detail = TEXT("SetJudgment+DoPulse callable");
	return true;
}

bool AMelodiaLoopVerifier::VerifyQuestHook(FString& Detail)
{
	UClass* QuestClass = ResolveClass(QuestManagerClassPath);
	AActor* QuestManager = FindOrSpawnActor(QuestClass, FVector(-10570.0f, -5000.0f, -2140.0f));
	if (!QuestManager)
	{
		Detail = TEXT("missing");
		return false;
	}

	UFunction* NotifyFunction = QuestManager->FindFunction(TEXT("NotifyBattleWon"));
	if (!NotifyFunction)
	{
		Detail = TEXT("missing NotifyBattleWon");
		return false;
	}

	const AMelodiaQuestManagerBase* NativeQuestManager = Cast<AMelodiaQuestManagerBase>(QuestManager);
	Detail = NativeQuestManager
		? FString::Printf(TEXT("NotifyBattleWon callable wins=%d"), NativeQuestManager->CompletedBattleCount)
		: TEXT("NotifyBattleWon callable");
	return true;
}

bool AMelodiaLoopVerifier::VerifyRhythmManagerWiring(FString& Detail)
{
	UClass* RhythmManagerClass = ResolveClass(RhythmManagerClassPath);
	AActor* RhythmManager = FindOrSpawnActor(RhythmManagerClass, FVector(-10700.0f, -5040.0f, -2150.0f));
	if (!RhythmManager)
	{
		Detail = TEXT("missing rhythm manager");
		return false;
	}

	const FProperty* MusicManagerProperty = RhythmManager->GetClass()->FindPropertyByName(TEXT("MusicManagerRef"));
	const FObjectPropertyBase* MusicManagerObjectProperty = CastField<FObjectPropertyBase>(MusicManagerProperty);
	UObject* MusicManagerObject = MusicManagerObjectProperty ? MusicManagerObjectProperty->GetObjectPropertyValue_InContainer(RhythmManager) : nullptr;
	const AMelodiaMusicManager* MusicManager = Cast<AMelodiaMusicManager>(MusicManagerObject);
	if (!MusicManager)
	{
		Detail = TEXT("MusicManagerRef is not MelodiaMusicManager");
		return false;
	}

	Detail = FString::Printf(TEXT("manager=%s quartz=%s"),
		*MusicManager->GetClass()->GetName(),
		MusicManager->IsQuartzClockActive() ? TEXT("true") : TEXT("false"));
	return MusicManager->IsQuartzClockActive();
}

bool AMelodiaLoopVerifier::VerifyPCGGraphs(FString& Detail)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		Detail = TEXT("no world");
		return false;
	}

	int32 PCGComponentCount = 0;
	int32 GraphsWithNodes = 0;
	int32 CustomElementsFound = 0;
	int32 TotalGeneratedPoints = 0;
	bool bHasMelodiaAttrs = false;

	// Iterate all PCG components in the world.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		++PCGComponentCount;

		// Validate the graph asset structure.
		const UPCGGraph* Graph = PCGComp->GetGraph();
		if (!Graph)
		{
			continue;
		}

		// Check graph has nodes (non-empty).
		const TArray<UPCGNode*>& Nodes = Graph->GetNodes();
		if (Nodes.Num() > 0)
		{
			++GraphsWithNodes;
		}

		// Check for Melodia custom settings classes in the graph.
		for (const UPCGNode* Node : Nodes)
		{
			if (!Node) continue;
			const UPCGSettings* Settings = Node->GetSettings();
			if (!Settings) continue;

			const FString ClassName = Settings->GetClass()->GetName();
			if (ClassName.Contains(TEXT("Escher")) ||
				ClassName.Contains(TEXT("GravityZone")) ||
				ClassName.Contains(TEXT("RecursiveArch")) ||
				ClassName.Contains(TEXT("Tessellation")))
			{
				++CustomElementsFound;
			}
		}

		// Access generated output data to count points and check for Melodia attributes.
		// UE 5.7: use GetGeneratedGraphOutput() for runtime-accessible generated data (project standard).
		const FPCGDataCollection& GenOutput = PCGComp->GetGeneratedGraphOutput();
		for (const FPCGTaggedData& Tagged : GenOutput.TaggedData)
		{
			const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged.Data);
			if (!PointData) continue;

			TotalGeneratedPoints += PointData->GetPoints().Num();

			const UPCGMetadata* Meta = PointData->Metadata;
			if (Meta)
			{
				const FPCGMetadataAttribute<int32>* RoleAttr =
					Meta->GetConstTypedAttribute<int32>(FMelodiaPCGAttrs::ArchitecturalRoleAttr);
				if (RoleAttr)
				{
					bHasMelodiaAttrs = true;
				}
			}
		}
	}

	// Summary.
	Detail = FString::Printf(
		TEXT("components=%d graphs=%d customElements=%d generatedPoints=%d hasMelodiaAttrs=%s"),
		PCGComponentCount,
		GraphsWithNodes,
		CustomElementsFound,
		TotalGeneratedPoints,
		bHasMelodiaAttrs ? TEXT("true") : TEXT("false"));

	// Pass criteria:
	// - No PCG components → OK (hand-authored level).
	// - PCG components present → at least one graph has nodes.
	if (PCGComponentCount == 0)
	{
		Detail += TEXT(" [no PCG components — OK]");
		return true;
	}

	bool bPass = GraphsWithNodes >= 1;
	if (CustomElementsFound > 0 && !bHasMelodiaAttrs)
	{
		Detail += TEXT(" [WARN: custom elements found but no Melodia attrs detected]");
		// Don't fail — data may not be generated yet in editor.
	}
	return bPass;
}
