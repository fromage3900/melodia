// Blueprintable save/rest point: Melusina's Bed advances the day and saves.

#include "MelodiaRestPoint.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaSaveGame.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

AMelodiaRestPoint::AMelodiaRestPoint()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	RootComponent = InteractionSphere;
	InteractionSphere->InitSphereRadius(220.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BedMarker"));
	VisualMesh->SetupAttachment(InteractionSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -50.0f));
	VisualMesh->SetRelativeScale3D(FVector(1.8f, 0.85f, 0.25f));

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
	if (CubeMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(CubeMesh.Object);
	}
}

void AMelodiaRestPoint::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s ready at %s"), *DisplayName, *GetActorLocation().ToString());
}

bool AMelodiaRestPoint::IsPawnInRange(const APawn* Pawn) const
{
	if (!Pawn || !InteractionSphere)
	{
		return false;
	}

	const float Radius = InteractionSphere->GetScaledSphereRadius();
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

bool AMelodiaRestPoint::ActivateRest(APawn* RestingPawn)
{
	if (!RestingPawn || !IsPawnInRange(RestingPawn))
	{
		return false;
	}

	UMelodiaSaveGame* SaveData = Cast<UMelodiaSaveGame>(UGameplayStatics::LoadGameFromSlot(SaveSlotName, UserIndex));
	if (!SaveData)
	{
		SaveData = Cast<UMelodiaSaveGame>(UGameplayStatics::CreateSaveGameObject(UMelodiaSaveGame::StaticClass()));
	}
	if (!SaveData)
	{
		bLastSaveSucceeded = false;
		return false;
	}

	++RestActivationCount;
	SaveData->DayIndex = FMath::Max(1, SaveData->DayIndex + 1);
	SaveData->RestCount = FMath::Max(SaveData->RestCount + 1, RestActivationCount);
	SaveData->LastRestLocation = RestingPawn->GetActorLocation();
	SaveData->LastRestRotation = RestingPawn->GetActorRotation();
	SaveData->LastMapName = FName(*UGameplayStatics::GetCurrentLevelName(this, true));
	SaveData->bHasRestedAtMelusinasBed = true;
	SaveData->LastSaveReason = TEXT("Rested at Melusina's Bed");
	LastSavedDay = SaveData->DayIndex;

	bLastSaveSucceeded = UGameplayStatics::SaveGameToSlot(SaveData, SaveSlotName, UserIndex);
	PublishRestFeedback(GetWorld(), bLastSaveSucceeded
		? FString::Printf(TEXT("Rested at Melusina's Bed | Day %d saved"), LastSavedDay)
		: TEXT("Rest failed | Could not save"));

	return bLastSaveSucceeded;
}

void AMelodiaRestPoint::PublishRestFeedback(UWorld* World, const FString& Text) const
{
	if (!World)
	{
		return;
	}

	if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
	{
		Widget->ShowBattleStatus(Text);
		Widget->ShowActionPrompt(Text);
		Widget->PushFloatingCombatText(TEXT("SAVE"), false, FLinearColor(0.72f, 0.92f, 1.0f, 1.0f));
	}
}

// ─────────────────────────────────────────────────────────────────────────────
// IMelodiaInteractable interface implementation
// ─────────────────────────────────────────────────────────────────────────────

FString AMelodiaRestPoint::GetDisplayName_Implementation() const
{
	return DisplayName;
}

FString AMelodiaRestPoint::GetInteractionPrompt_Implementation() const
{
	return InteractionPrompt;
}

bool AMelodiaRestPoint::ActivateInteraction_Implementation(APawn* InstigatorPawn)
{
	return ActivateRest(InstigatorPawn);
}

bool AMelodiaRestPoint::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return IsPawnInRange(InstigatorPawn);
}

USphereComponent* AMelodiaRestPoint::GetInteractionSphere_Implementation() const
{
	return InteractionSphere;
}
