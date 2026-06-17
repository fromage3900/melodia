// Authoritative battle encounter orchestrator.

#include "MelodiaBattleSession.h"

#include "Engine/World.h"
#include "MelodiaBattleInputComponent.h"
#include "MelodiaBattleLoopLibrary.h"
#include "MelodiaContentRegistrySubsystem.h"
#include "MelodiaJRPGBridgeLibrary.h"
#include "MelodiaJRPGPresenter.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaPartySubsystem.h"
#include "MelodiaRhythmExecutionComponent.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "MelodiaSongSkillLibrary.h"

void UMelodiaBattleSession::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	BattlePhase = EMelodiaBattlePhase::None;
	HUDMode = EMelodiaHUDMode::Exploration;
}

UMelodiaBattleSession* UMelodiaBattleSession::Get(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	const UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	UGameInstance* GI = World->GetGameInstance();
	return GI ? GI->GetSubsystem<UMelodiaBattleSession>() : nullptr;
}

bool UMelodiaBattleSession::IsEncounterActive() const
{
	return BattlePhase != EMelodiaBattlePhase::None
		&& BattlePhase != EMelodiaBattlePhase::Victory
		&& BattlePhase != EMelodiaBattlePhase::Defeat
		&& BattlePhase != EMelodiaBattlePhase::Fled;
}

AActor* UMelodiaBattleSession::ResolveBattleController() const
{
	return ActiveBattleController;
}

UMelodiaRhythmExecutionComponent* UMelodiaBattleSession::ResolveExecutionComponent() const
{
	AActor* Controller = ResolveBattleController();
	return Controller ? Controller->FindComponentByClass<UMelodiaRhythmExecutionComponent>() : nullptr;
}

void UMelodiaBattleSession::SetBattlePhase(const EMelodiaBattlePhase NewPhase)
{
	if (BattlePhase == NewPhase)
	{
		return;
	}

	const EMelodiaBattlePhase PreviousPhase = BattlePhase;
	BattlePhase = NewPhase;
	OnBattlePhaseChanged.Broadcast(NewPhase, PreviousPhase);
	SyncHUDMode();
	SyncGameLoopPhase();
}

void UMelodiaBattleSession::SyncHUDMode()
{
	switch (BattlePhase)
	{
	case EMelodiaBattlePhase::AwaitingPlayerCommand:
	case EMelodiaBattlePhase::EnemyTurn:
		HUDMode = EMelodiaHUDMode::BattleCompact;
		break;
	case EMelodiaBattlePhase::RhythmExecution:
		HUDMode = EMelodiaHUDMode::BattleHighway;
		break;
	case EMelodiaBattlePhase::Victory:
		HUDMode = EMelodiaHUDMode::Victory;
		break;
	case EMelodiaBattlePhase::Defeat:
		HUDMode = EMelodiaHUDMode::Defeat;
		break;
	case EMelodiaBattlePhase::None:
	case EMelodiaBattlePhase::Fled:
	default:
		HUDMode = EMelodiaHUDMode::Exploration;
		break;
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetHUDMode(HUDMode);
		}
	}
}

void UMelodiaBattleSession::SyncGameLoopPhase()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(World->GetAuthGameMode());
	if (!GameMode)
	{
		return;
	}

	switch (BattlePhase)
	{
	case EMelodiaBattlePhase::AwaitingPlayerCommand:
	case EMelodiaBattlePhase::RhythmExecution:
	case EMelodiaBattlePhase::EnemyTurn:
		GameMode->SetLoopPhase(EMelodiaLoopPhase::Battle);
		break;
	case EMelodiaBattlePhase::Victory:
		GameMode->SetLoopPhase(EMelodiaLoopPhase::VictoryReward);
		break;
	case EMelodiaBattlePhase::None:
	case EMelodiaBattlePhase::Defeat:
	case EMelodiaBattlePhase::Fled:
	default:
		GameMode->SetLoopPhase(EMelodiaLoopPhase::ExplorationReady);
		break;
	}
}

bool UMelodiaBattleSession::BeginEncounter(const FMelodiaEncounterDefinition& Encounter, const bool bSuppressPhoenixBattleUI)
{
	if (!Encounter.BattleController)
	{
		return false;
	}

	if (IsEncounterActive())
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle session: encounter already active."));
		return false;
	}

	ActiveEncounter = Encounter;
	ActiveBattleController = Encounter.BattleController;
	ActiveBattleController->SetActorHiddenInGame(false);
	ActiveBattleController->SetActorEnableCollision(true);

	if (!UMelodiaJRPGPresenter::InitializeEncounter(this, Encounter, bSuppressPhoenixBattleUI))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle session: presenter init failed."));
	}

	UMelodiaBattleLoopLibrary::ResetRhythmBattleEncounter(ActiveBattleController);

	UMelodiaJRPGBridgeLibrary::SyncPartyUnitsFromSubsystem(this, ActiveBattleController);

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMelodiaPartySubsystem* Party = GI->GetSubsystem<UMelodiaPartySubsystem>())
		{
			Party->SyncFromProgression();
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->bDrawExplorationHUD = false;
			Widget->ShowBattleStatus(TEXT("Battle started"));
			Widget->ShowActionPrompt(TEXT("Pick a command — Phoenix UI or 1/2/4"));
		}
	}

	SetBattlePhase(EMelodiaBattlePhase::AwaitingPlayerCommand);
	UE_LOG(LogTemp, Log, TEXT("Melodia battle session began encounter on %s."), *ActiveBattleController->GetName());
	return true;
}

bool UMelodiaBattleSession::SubmitLoopCommand(const EMelodiaRhythmBattleCommand Command, const float Grade)
{
	AActor* Controller = ResolveBattleController();
	if (!Controller || !IsEncounterActive())
	{
		return false;
	}

	if (UMelodiaRhythmExecutionComponent* Execution = ResolveExecutionComponent())
	{
		if (Execution->IsExecutionActive())
		{
			return false;
		}
	}

	const bool bSucceeded = UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(this, Controller, Command, Grade, 3);
	if (bSucceeded && Command != EMelodiaRhythmBattleCommand::Ultimate)
	{
		SetBattlePhase(EMelodiaBattlePhase::AwaitingPlayerCommand);
	}

	if (UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(Controller))
	{
		SetBattlePhase(EMelodiaBattlePhase::Victory);
	}
	else if (UMelodiaBattleLoopLibrary::IsPartyDefeated(Controller))
	{
		EndEncounter(EMelodiaEncounterResult::Defeat);
	}

	return bSucceeded;
}

bool UMelodiaBattleSession::SubmitBasicCommand()
{
	if (BattlePhase == EMelodiaBattlePhase::Victory)
	{
		return ConfirmVictoryReward();
	}

	return SubmitLoopCommand(EMelodiaRhythmBattleCommand::Basic, 1.0f);
}

bool UMelodiaBattleSession::SubmitSkillCommand(const FName SkillId)
{
	if (BattlePhase == EMelodiaBattlePhase::Victory)
	{
		return ConfirmVictoryReward();
	}

	if (!CanSubmitSkillCommand(SkillId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle session: cannot submit skill %s."), *SkillId.ToString());
		return false;
	}

	AActor* Controller = ResolveBattleController();
	if (!Controller)
	{
		return false;
	}

	if (UGameInstance* GI = GetGameInstance())
	{
		if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			Progression->State.ActiveSkillId = SkillId;
		}
	}

	UMelodiaRhythmExecutionComponent* Execution = ResolveExecutionComponent();
	if (!Execution)
	{
		Execution = NewObject<UMelodiaRhythmExecutionComponent>(Controller, UMelodiaRhythmExecutionComponent::StaticClass(), TEXT("MelodiaRhythmExecution"));
		if (Execution)
		{
			Execution->RegisterComponent();
			Controller->AddInstanceComponent(Execution);
		}
	}

	if (!Execution || !Execution->BeginSkillExecutionById(SkillId))
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle session: skill execution failed for %s."), *SkillId.ToString());
		return false;
	}

	NotifyRhythmExecutionStarted();

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			FMelodiaSongSkillRecipe Skill;
			if (UMelodiaSongSkillLibrary::ResolveSongSkill(this, SkillId, Skill))
			{
				Widget->ShowActionPrompt(FString::Printf(TEXT("Skill: %s — hit notes on beat"), *Skill.DisplayName.ToString()));
			}
		}
	}

	return true;
}

int32 UMelodiaBattleSession::GetPlayerMechanicLevel() const
{
	if (const UGameInstance* GI = GetGameInstance())
	{
		if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			return Progression->GetMechanicLevel();
		}
	}
	return 1;
}

TArray<FName> UMelodiaBattleSession::GetUnlockedSkillIds() const
{
	return UMelodiaSongSkillLibrary::ResolveUnlockedSkillIds(this, GetPlayerMechanicLevel());
}

TArray<FMelodiaSongSkillRecipe> UMelodiaBattleSession::GetUnlockedSkills() const
{
	TArray<FMelodiaSongSkillRecipe> Skills;
	for (const FName SkillId : GetUnlockedSkillIds())
	{
		FMelodiaSongSkillRecipe Recipe;
		if (UMelodiaSongSkillLibrary::ResolveSongSkill(this, SkillId, Recipe))
		{
			Skills.Add(Recipe);
		}
	}
	return Skills;
}

bool UMelodiaBattleSession::IsSkillUnlocked(const FName SkillId) const
{
	return GetUnlockedSkillIds().Contains(SkillId);
}

bool UMelodiaBattleSession::CanSubmitSkillCommand(const FName SkillId) const
{
	if (SkillId.IsNone() || !IsEncounterActive())
	{
		return false;
	}

	if (!IsSkillUnlocked(SkillId))
	{
		return false;
	}

	AActor* Controller = ResolveBattleController();
	return Controller && UMelodiaBattleLoopLibrary::CanUseRhythmSkill(Controller);
}

bool UMelodiaBattleSession::SubmitUltimateCommand()
{
	if (BattlePhase == EMelodiaBattlePhase::Victory)
	{
		return ConfirmVictoryReward();
	}

	if (UMelodiaRhythmExecutionComponent* Execution = ResolveExecutionComponent())
	{
		if (Execution->IsExecutionActive())
		{
			Execution->CancelExecution();
		}
	}

	const bool bTriggered = UMelodiaBattleLoopLibrary::TriggerRhythmUltimate(this, ResolveBattleController());
	if (bTriggered && UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(ResolveBattleController()))
	{
		SetBattlePhase(EMelodiaBattlePhase::Victory);
	}
	return bTriggered;
}

bool UMelodiaBattleSession::SubmitFleeCommand()
{
	if (BattlePhase == EMelodiaBattlePhase::Victory)
	{
		return ConfirmVictoryReward();
	}

	AActor* Controller = ResolveBattleController();
	if (!Controller)
	{
		return false;
	}

	if (UMelodiaRhythmExecutionComponent* Execution = ResolveExecutionComponent())
	{
		if (Execution->IsExecutionActive())
		{
			Execution->CancelExecution();
		}
	}

	UMelodiaJRPGPresenter::TryFleePresentation(Controller);
	const bool bFled = UMelodiaBattleLoopLibrary::TryFleeRhythmBattle(this, Controller);
	if (bFled)
	{
		EndEncounter(EMelodiaEncounterResult::Fled);
	}
	return bFled;
}

bool UMelodiaBattleSession::ConfirmVictoryReward()
{
	AActor* Controller = ResolveBattleController();
	if (!Controller)
	{
		return false;
	}

	if (!UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(Controller))
	{
		return false;
	}

	UMelodiaBattleLoopLibrary::ConfirmRhythmVictoryReward(Controller);
	EndEncounter(EMelodiaEncounterResult::Victory);
	return true;
}

void UMelodiaBattleSession::NotifyRhythmExecutionStarted()
{
	SetBattlePhase(EMelodiaBattlePhase::RhythmExecution);
}

void UMelodiaBattleSession::NotifyRhythmExecutionFinished()
{
	AActor* Controller = ResolveBattleController();
	if (Controller && UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(Controller))
	{
		SetBattlePhase(EMelodiaBattlePhase::Victory);
	}
	else
	{
		SetBattlePhase(EMelodiaBattlePhase::AwaitingPlayerCommand);
	}
}

void UMelodiaBattleSession::EndEncounter(const EMelodiaEncounterResult Result)
{
	(void)Result;

	if (ActiveBattleController)
	{
		UMelodiaJRPGPresenter::TeardownPresentation(ActiveBattleController);
	}

	ActiveBattleController = nullptr;
	ActiveEncounter = FMelodiaEncounterDefinition();
	SetBattlePhase(EMelodiaBattlePhase::None);
}
