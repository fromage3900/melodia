// Native input bridge from mapped battle commands into the Melodia rhythm loop.

#include "MelodiaBattleInputComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaRhythmExecutionComponent.h"
#include "MelodiaRhythmGameModeBase.h"

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
	InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::One, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::Two, IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	InputComponent->BindKey(EKeys::R, IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Left, IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	InputComponent->BindKey(EKeys::Gamepad_RightTrigger, IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	bInputBound = true;

	UE_LOG(LogTemp, Log, TEXT("Melodia battle input bound Attack/Skill/Ultimate on player controller."));
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
	if (!BattleController || !UMelodiaBattleLoopLibrary::HasRhythmVictoryResolved(BattleController))
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

	if (Execution && Execution->BeginBasicExecution())
	{
		return true;
	}

	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Basic);
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

	if (Execution && Execution->BeginSkillExecution())
	{
		return true;
	}

	return false;
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

	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Ultimate);
}

bool UMelodiaBattleInputComponent::ExecuteInputCommand(const EMelodiaRhythmBattleCommand Command)
{
	AActor* BattleController = GetOwner();
	if (!BattleController)
	{
		return false;
	}

	if (Command != EMelodiaRhythmBattleCommand::Ultimate)
	{
		PrimeRhythmBlueprintHooks(BattleController);
	}

	const bool bSucceeded = UMelodiaBattleLoopLibrary::ExecuteRhythmBattleCommand(this, BattleController, Command, InputCommandGrade, ComboToWin);
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
	HandleSkillInput();
}

void UMelodiaBattleInputComponent::OnUltimateInputPressed()
{
	HandleUltimateInput();
}

void UMelodiaBattleInputComponent::PrimeRhythmBlueprintHooks(AActor* BattleController) const
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
		MultiplierParams.Grade = InputCommandGrade;
		BattleController->ProcessEvent(SetMultiplierFunction, &MultiplierParams);
	}
}
