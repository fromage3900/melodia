// Exploration UI input: inventory toggle and quest refresh.

#include "MelodiaExplorationInputComponent.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaInteractable.h"
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

	APlayerController* PlayerController = UGameplayStatics::GetPlayerController(World, 0);
	if (!PlayerController || !PlayerController->InputComponent)
	{
		return false;
	}

	PlayerController->InputComponent->BindAction(TEXT("Inventory"), IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInventoryPressed);
	PlayerController->InputComponent->BindAction(TEXT("Interact"), IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	PlayerController->InputComponent->BindKey(EKeys::E, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	PlayerController->InputComponent->BindKey(EKeys::I, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInventoryPressed);
	PlayerController->InputComponent->BindKey(EKeys::Gamepad_FaceButton_Bottom, IE_Pressed, this, &UMelodiaExplorationInputComponent::OnInteractPressed);
	bInputBound = true;
	return true;
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
