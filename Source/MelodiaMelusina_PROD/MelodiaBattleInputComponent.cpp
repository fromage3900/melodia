// Native input bridge from mapped battle commands into the Melodia rhythm loop.

#include "MelodiaBattleInputComponent.h"

#include "Components/InputComponent.h"
#include "GameFramework/Actor.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

UMelodiaBattleInputComponent::UMelodiaBattleInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMelodiaBattleInputComponent::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoBindPlayerInput)
	{
		BindBattleInput();
	}
}

bool UMelodiaBattleInputComponent::BindBattleInput()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();
	if (!Owner || !World)
	{
		return false;
	}

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController)
	{
		return false;
	}

	Owner->EnableInput(PlayerController);
	if (!Owner->InputComponent)
	{
		return false;
	}

	Owner->InputComponent->BindAction(TEXT("Attack"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnBasicInputPressed);
	Owner->InputComponent->BindAction(TEXT("Skill"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnSkillInputPressed);
	Owner->InputComponent->BindAction(TEXT("Ultimate"), IE_Pressed, this, &UMelodiaBattleInputComponent::OnUltimateInputPressed);
	bInputBound = true;
	return true;
}

bool UMelodiaBattleInputComponent::HandleBasicInput()
{
	++BasicInputCount;
	LastInputCommandName = TEXT("Basic");
	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Basic);
}

bool UMelodiaBattleInputComponent::HandleSkillInput()
{
	++SkillInputCount;
	LastInputCommandName = TEXT("Skill");
	return ExecuteInputCommand(EMelodiaRhythmBattleCommand::Skill);
}

bool UMelodiaBattleInputComponent::HandleUltimateInput()
{
	++UltimateInputCount;
	LastInputCommandName = TEXT("Ultimate");
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
