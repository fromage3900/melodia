// Native input bridge from mapped battle commands into the Melodia rhythm loop.

#include "MelodiaBattleInputComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "MelodiaBattleSession.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaMusicManager.h"
#include "MelodiaSongSkillLibrary.h"
#include "MelodiaRhythmExecutionComponent.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaRhythmHUDWidget.h"

UMelodiaBattleInputComponent::UMelodiaBattleInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMelodiaBattleInputComponent::BeginPlay()
{
	Super::BeginPlay();

	EnsureExecutionComponent();

	if (bAutoBindPlayerInput)
	{
		BindBattleInput();
	}
}

bool UMelodiaBattleInputComponent::BindBattleInput()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle input: no player controller yet."));
		return false;
	}

	UInputComponent* InputComponent = PlayerController->InputComponent;
	if (!InputComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("Melodia battle input: player controller has no InputComponent."));
		return false;
	}

	if (bInputBound)
	{
		return true;
	}

	// Legacy action names (DefaultInput.ini) plus direct key binds for Enhanced Input projects.
	InputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindAction(TEXT("Skill"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	InputComponent->BindAction(TEXT("Ultimate"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	InputComponent->BindAction(TEXT("Flee"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnFleeInputPressed);
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	InputComponent->BindKey(EKeys::Tab, IE_Pressed, this, &UMelodiaBattleInputComponent::OnCycleSkillInputPressed);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	InputComponent->BindKey(EKeys::Four, IE_Pressed, this, &UMelodiaBattleInputComponent::OnFleeInputPressed);
	InputComponent->BindKey(EKeys::Escape, IE_Pressed, this, &UMelodiaBattleInputComponent::OnFleeInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Right, IE_Pressed, this, &UMelodiaBattleInputComponent::OnFleeInputPressed);
	bInputBound = true;

	UE_LOG(LogTemp, Log, TEXT("Melodia battle input bound Attack/Skill/Ultimate/Flee on player controller."));
	return true;
}

UMelodiaRhythmExecutionComponent* UMelodiaBattleInputComponent::EnsureExecutionComponent()
{
	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return nullptr;
	}

	if (UMelodiaRhythmExecutionComponent* Existing = Owner->FindComponentByClass<UMelodiaRhythmExecutionComponent>())
	{
		return Existing;
	}

	UMelodiaRhythmExecutionComponent* NewComponent = NewObject<UMelodiaRhythmExecutionComponent>(Owner, UMelodiaRhythmExecutionComponent::StaticClass(), TEXT("MelodiaRhythmExecution"));
	if (NewComponent)
	{
		NewComponent->RegisterComponent();
		Owner->AddInstanceComponent(NewComponent);
	}
	return NewComponent;
}

bool UMelodiaBattleInputComponent::IsBattleInputAllowed() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(World)))
		{
			return GameMode->CurrentLoopPhase == EMelodiaLoopPhase::Battle
				|| GameMode->CurrentLoopPhase == EMelodiaLoopPhase::VictoryReward;
		}
	}

	return false;
}

bool UMelodiaBattleInputComponent::TryConfirmVictoryReward()
{
	AActor* BattleController = GetOwner();
	if (!BattleController)
	{
		return false;
	}

	if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
	{
		if (UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(BattleController))
		{
			return Session->ConfirmVictoryReward();
		}
	}

	if (!UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(BattleController))
	{
		return false;
	}

	UMelodiaBattleLoopLibrary::ConfirmRhythmVictoryReward(BattleController);
	return true;
}

bool UMelodiaBattleInputComponent::HandleBasicInput()
{
	if (!IsBattleInputAllowed())
	{
		return false;
	}

	if (TryConfirmVictoryReward())
	{
		return true;
	}

	++BasicInputCount;
	LastInputCommandName = TEXT("Basic");

	UMelodiaRhythmExecutionComponent* Execution = EnsureExecutionComponent();
	if (Execution && Execution->IsExecutionActive())
	{
		return Execution->TryHitCurrentNote();
	}

	if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
	{
		if (Session->IsEncounterActive() || Session->BattlePhase == EMelodiaBattlePhase::Victory)
		{
			return Session->SubmitBasicCommand();
		}
	}

	// Classic JRPG basic attack — rhythm highway only opens after choosing a skill.
	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Basic, 1.0f);
}

bool UMelodiaBattleInputComponent::HandleSkillInput()
{
	if (!IsBattleInputAllowed())
	{
		return false;
	}

	if (TryConfirmVictoryReward())
	{
		return true;
	}

	++SkillInputCount;
	LastInputCommandName = TEXT("Skill");

	UMelodiaRhythmExecutionComponent* Execution = EnsureExecutionComponent();
	if (Execution && Execution->IsExecutionActive())
	{
		return Execution->TryHitCurrentNote();
	}

	if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
	{
		if (Session->IsEncounterActive())
		{
			FName SkillId = NAME_None;
			if (const UGameInstance* GI = GetWorld()->GetGameInstance())
			{
				if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
				{
					SkillId = Progression->State.ActiveSkillId;
				}
			}
			if (SkillId.IsNone())
			{
				SkillId = UMelodiaSongSkillLibrary::ResolveSkillIdForMechanicLevel(this, 1);
			}
			return Session->SubmitSkillCommand(SkillId);
		}
	}

	if (Execution && Execution->BeginSkillExecution())
	{
		ShowActiveSkillPrompt();
		return true;
	}

	return false;
}

bool UMelodiaBattleInputComponent::HandleCycleSkillInput()
{
	if (!IsBattleInputAllowed())
	{
		return false;
	}

	UMelodiaRhythmExecutionComponent* Execution = EnsureExecutionComponent();
	if (Execution && Execution->IsExecutionActive())
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				Progression->CycleActiveSkill();
				ShowActiveSkillPrompt();
				return true;
			}
		}
	}
	return false;
}

void UMelodiaBattleInputComponent::ShowActiveSkillPrompt() const
{
	if (const UWorld* World = GetWorld())
	{
		if (const UGameInstance* GI = World->GetGameInstance())
		{
			if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				const FString SkillName = Progression->GetActiveSkillDisplayName().ToString();
				if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
				{
					Widget->ShowActionPrompt(FString::Printf(
						TEXT("Active skill: %s | 2=Rhythm highway | Tab=cycle"),
						*SkillName));
				}
			}
		}
	}
}

bool UMelodiaBattleInputComponent::HandleUltimateInput()
{
	if (!IsBattleInputAllowed())
	{
		return false;
	}

	if (TryConfirmVictoryReward())
	{
		return true;
	}

	++UltimateInputCount;
	LastInputCommandName = TEXT("Ultimate");

	UMelodiaRhythmExecutionComponent* Execution = EnsureExecutionComponent();
	if (Execution && Execution->IsExecutionActive())
	{
		Execution->CancelExecution();
	}

	if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
	{
		if (Session->IsEncounterActive() || Session->BattlePhase == EMelodiaBattlePhase::Victory)
		{
			return Session->SubmitUltimateCommand();
		}
	}

	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Ultimate);
}

bool UMelodiaBattleInputComponent::HandleFleeInput()
{
	if (!IsBattleInputAllowed())
	{
		return false;
	}

	if (TryConfirmVictoryReward())
	{
		return true;
	}

	LastInputCommandName = TEXT("Flee");
	if (UMelodiaBattleSession* Session = UMelodiaBattleSession::Get(this))
	{
		if (Session->IsEncounterActive() || Session->BattlePhase == EMelodiaBattlePhase::Victory)
		{
			return Session->SubmitFleeCommand();
		}
	}
	return UMelodiaBattleLoopLibrary::TryFleeRhythmBattle(this, GetOwner());
}

FMelodiaRhythmGradeResult UMelodiaBattleInputComponent::GradeCurrentBeatTap() const
{
	if (const UWorld* World = GetWorld())
	{
		for (TActorIterator<AMelodiaMusicManager> It(World); It; ++It)
		{
			const float Phase = It->GetBeatPhase();
			const float PhaseError = FMath::Min(Phase, 1.0f - Phase);
			const float TimingErrorMs = PhaseError * It->GetBeatLength() * 1000.0f;
			return UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(TimingErrorMs, RhythmWindows);
		}
	}

	return UMelodiaCoreRulesLibrary::GradeInputFromTimingErrorMs(200.0f, RhythmWindows);
}

void UMelodiaBattleInputComponent::ShowTapFeedback(const FMelodiaRhythmGradeResult& GradeResult) const
{
	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetJudgment(GradeResult.DisplayText);
			Widget->DoPulse();
			if (GradeResult.bCountsAsHit)
			{
				Widget->TriggerSparkleBurst();
			}
		}
	}
}

bool UMelodiaBattleInputComponent::ExecuteInputCommand(const EMelodiaRhythmBattleCommand Command, const float GradeOverride)
{
	AActor* BattleController = GetOwner();
	if (!BattleController)
	{
		return false;
	}

	const float EffectiveGrade = GradeOverride >= 0.0f ? GradeOverride : InputCommandGrade;

	if (Command != EMelodiaRhythmBattleCommand::Ultimate)
	{
		PrimeRhythmBlueprintHooks(BattleController, EffectiveGrade);
	}

	const bool bSucceeded = UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(this, BattleController, Command, EffectiveGrade, ComboToWin);
	if (bSucceeded || Command != EMelodiaRhythmBattleCommand::Ultimate)
	{
		++SuccessfulCommandCount;
	}
	return bSucceeded;
}

void UMelodiaBattleInputComponent::OnBasicInputPressed()
{
	HandleBasicInput();
}

void UMelodiaBattleInputComponent::OnSkillInputPressed()
{
	if (APlayerController* PC = GetWorld() ? UGameplayStatics::GetPlayerController(GetWorld(), 0) : nullptr)
	{
		if (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))
		{
			HandleCycleSkillInput();
			return;
		}
	}
	HandleSkillInput();
}

void UMelodiaBattleInputComponent::OnUltimateInputPressed()
{
	HandleUltimateInput();
}

void UMelodiaBattleInputComponent::OnFleeInputPressed()
{
	HandleFleeInput();
}

void UMelodiaBattleInputComponent::OnCycleSkillInputPressed()
{
	HandleCycleSkillInput();
}

void UMelodiaBattleInputComponent::PrimeRhythmBlueprintHooks(AActor* BattleController, const float Grade) const
{
	if (!BattleController)
	{
		return;
	}

	if (UFunction* RegisterFunction = BattleController->FindFunction(TEXT("RegisterRhythmHit")))
	{
		struct FRegisterParams
		{
			bool bSuccess = true;
			int32 NewCombo = 0;
		};

		FRegisterParams RegisterParams;
		BattleController->ProcessEvent(RegisterFunction, &RegisterParams);
	}

	if (UFunction* SetMultiplierFunction = BattleController->FindFunction(TEXT("SetRhythmMultiplier")))
	{
		struct FMultiplierParams
		{
			float Grade = 1.5f;
		};

		FMultiplierParams MultiplierParams;
		MultiplierParams.Grade = Grade;
		BattleController->ProcessEvent(SetMultiplierFunction, &MultiplierParams);
	}
}
