// Exploration UI input: inventory toggle and quest refresh.

#include "MelodiaExplorationInputComponent.h"

#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "InputCoreTypes.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaPortal.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRestPoint.h"
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
		for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
		{
			UMelodiaRhythmHUDWidget* Widget = *It;
			if (Widget && Widget->GetWorld() == World)
			{
				Widget->SetInventoryPanelOpen(!Widget->bInventoryPanelOpen);
			}
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

	AActor* BestActor = nullptr;
	float BestDistanceSq = TNumericLimits<float>::Max();

	for (TActorIterator<AMelodiaRestPoint> It(World); It; ++It)
	{
		AMelodiaRestPoint* RestPoint = *It;
		if (RestPoint && RestPoint->IsPawnInRange(PlayerPawn))
		{
			const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), RestPoint->GetActorLocation());
			if (DistSq < BestDistanceSq)
			{
				BestDistanceSq = DistSq;
				BestActor = RestPoint;
			}
		}
	}

	for (TActorIterator<AMelodiaPortal> It(World); It; ++It)
	{
		AMelodiaPortal* Portal = *It;
		if (Portal && Portal->IsPawnInRange(PlayerPawn))
		{
			const float DistSq = FVector::DistSquared(PlayerPawn->GetActorLocation(), Portal->GetActorLocation());
			if (DistSq < BestDistanceSq)
			{
				BestDistanceSq = DistSq;
				BestActor = Portal;
			}
		}
	}

	if (AMelodiaRestPoint* RestPoint = Cast<AMelodiaRestPoint>(BestActor))
	{
		return RestPoint->ActivateRest(PlayerPawn);
	}
	if (AMelodiaPortal* Portal = Cast<AMelodiaPortal>(BestActor))
	{
		return Portal->ActivatePortal(PlayerPawn);
	}

	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		UMelodiaRhythmHUDWidget* Widget = *It;
		if (Widget && Widget->GetWorld() == World)
		{
			Widget->ShowActionPrompt(TEXT("No interaction nearby"));
		}
	}
	return false;
}

void UMelodiaExplorationInputComponent::OnInteractPressed()
{
	ActivateNearestInteraction();
}
