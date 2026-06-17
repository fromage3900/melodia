// Exploration UI input: inventory toggle and quest refresh.

#include "MelodiaExplorationInputComponent.h"

#include "Components/InputComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaCharacterBase.h"
#include "MelodiaInteractable.h"
#include "MelodiaPickableFlower.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/UObjectIterator.h"

UMelodiaExplorationInputComponent::UMelodiaExplorationInputComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UMelodiaExplorationInputComponent::BeginPlay()
{
	Super::BeginPlay();
	BindExplorationInput();
}

bool UMelodiaExplorationInputComponent::BindExplorationInput()
{
	UWorld* World = GetWorld();
	if (!World || bInputBound)
	{
		return bInputBound;
	}

	UInputComponent* InputComponent = ResolveInputComponent();
	if (!InputComponent)
	{
		return false;
	}

	InputComponent->BindAction(TEXT("Inventory"), IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInventoryPressed);
	InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	InputComponent->BindAction(TEXT("Pick"), IE_Pressed, this, &UMelodiaExplorationInputComponent::OnPickPressed);
	InputComponent->BindKey(EKeys::E, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	InputComponent->BindKey(EKeys::F, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnPickPressed);
	InputComponent->BindKey(EKeys::I, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInventoryPressed);
	InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	InputComponent->BindKey(EKeys::Gamepad_DPad_Up, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnPickPressed);
	bInputBound = true;

	if (!GetOwner()->IsA<AMelodiaCharacterBase>())
	{
		BindLocomotionInput(InputComponent);
	}

	return true;
}

UInputComponent* UMelodiaExplorationInputComponent::ResolveInputComponent() const
{
	if (const APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		if (OwnerPawn->InputComponent)
		{
			return OwnerPawn->InputComponent;
		}
	}

	if (UWorld* World = GetWorld())
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0))
		{
			return PlayerController->InputComponent;
		}
	}

	return nullptr;
}

bool UMelodiaExplorationInputComponent::BindLocomotionInput(UInputComponent* InputComponent)
{
	if (!InputComponent || bLocomotionBound || !GetOwner())
	{
		return bLocomotionBound;
	}

	InputComponent->BindAxis(TEXT("MoveForward"), this, &UMelodiaExplorationInputComponent::MoveForward);
	InputComponent->BindAxis(TEXT("MoveRight"), this, &UMelodiaExplorationInputComponent::MoveRight);
	InputComponent->BindAxis(TEXT("Turn"), this, &UMelodiaExplorationInputComponent::Turn);
	InputComponent->BindAxis(TEXT("LookUp"), this, &UMelodiaExplorationInputComponent::LookUp);
	InputComponent->BindAxis(TEXT("TurnRate"), this, &UMelodiaExplorationInputComponent::Turn);
	InputComponent->BindAxis(TEXT("LookUpRate"), this, &UMelodiaExplorationInputComponent::LookUp);

	if (ACharacter* Character = Cast<ACharacter>(GetOwner()))
	{
		InputComponent->BindAction(TEXT("Jump"), IE_Pressed, Character, &ACharacter::Jump);
		InputComponent->BindAction(TEXT("Jump"), IE_Released, Character, &ACharacter::StopJumping);
		InputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, Character, &ACharacter::Jump);
		InputComponent->BindKey(EKeys::SpaceBar, IE_Released, Character, &ACharacter::StopJumping);
		InputComponent->BindKey(EKeys::LeftShift, IE_Pressed, Character, &ACharacter::Jump);
		InputComponent->BindKey(EKeys::LeftShift, IE_Released, Character, &ACharacter::StopJumping);
	}

	bLocomotionBound = true;
	return true;
}

void UMelodiaExplorationInputComponent::MoveForward(const float Value)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || Value == 0.0f)
	{
		return;
	}

	const FRotator ControlRotation = OwnerPawn->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	OwnerPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void UMelodiaExplorationInputComponent::MoveRight(const float Value)
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn || Value == 0.0f)
	{
		return;
	}

	const FRotator ControlRotation = OwnerPawn->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	OwnerPawn->AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void UMelodiaExplorationInputComponent::Turn(const float Value)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->AddControllerYawInput(Value);
	}
}

void UMelodiaExplorationInputComponent::LookUp(const float Value)
{
	if (APawn* OwnerPawn = Cast<APawn>(GetOwner()))
	{
		OwnerPawn->AddControllerPitchInput(Value);
	}
}

void UMelodiaExplorationInputComponent::ToggleInventoryPanel()
{
	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetInventoryPanelOpen(!Widget->bInventoryPanelOpen);
		}

		for (TActorIterator<AMelodiaQuestManagerBase> QuestIt(World); QuestIt; ++QuestIt)
		{
			QuestIt->SyncHUD(World);
			break;
		}
	}
}

void UMelodiaExplorationInputComponent::OnInventoryPressed()
{
	ToggleInventoryPanel();
}

bool UMelodiaExplorationInputComponent::ActivateNearestInteraction()
{
	UWorld* World = GetWorld();
	APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!World || !PlayerPawn)
	{
		return false;
	}

	// Find the nearest actor implementing IMelodiaInteractable.
	AActor* BestActor = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Candidate = *It;
		if (!Candidate || !Candidate->GetClass()->ImplementsInterface(UMelodiaInteractable::StaticClass()))
		{
			continue;
		}

		if (!IMelodiaInteractable::Execute_CanInteract(Candidate, PlayerPawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Candidate->GetActorLocation());
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestActor = Candidate;
		}
	}

	if (BestActor)
	{
		return IMelodiaInteractable::Execute_ActivateInteraction(BestActor, PlayerPawn);
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->ShowActionPrompt(TEXT("No interaction nearby"));
	}
	return false;
}

void UMelodiaExplorationInputComponent::OnInteractPressed()
{
	ActivateNearestInteraction();
}

bool UMelodiaExplorationInputComponent::ActivateNearestPickable()
{
	UWorld* World = GetWorld();
	APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!World || !PlayerPawn)
	{
		return false;
	}

	AMelodiaPickableFlower* BestFlower = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		AMelodiaPickableFlower* Flower = *It;
		if (!Flower || !Flower->CanPick(PlayerPawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Flower->GetActorLocation());
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestFlower = Flower;
		}
	}

	if (BestFlower && BestFlower->TryPick(PlayerPawn))
	{
		return true;
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->ShowActionPrompt(TEXT("No flower in range (F)"));
	}
	return false;
}

FString UMelodiaExplorationInputComponent::GetNearestPickPrompt() const
{
	UWorld* World = GetWorld();
	const APawn* PlayerPawn = World ? UGameplayStatics::GetPlayerPawn(World, 0) : nullptr;
	if (!World || !PlayerPawn)
	{
		return FString();
	}

	AMelodiaPickableFlower* BestFlower = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		AMelodiaPickableFlower* Flower = *It;
		if (!Flower || !Flower->CanPick(PlayerPawn))
		{
			continue;
		}

		const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Flower->GetActorLocation());
		if (DistSq < BestDistanceSq)
		{
			BestDistanceSq = DistSq;
			BestFlower = Flower;
		}
	}

	return BestFlower ? BestFlower->GetPickPromptText() : FString();
}

void UMelodiaExplorationInputComponent::OnPickPressed()
{
	ActivateNearestPickable();
}
