// Native exploration hero base: locomotion pawn with Melodia gameplay components.

#include "MelodiaCharacterBase.h"

#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaExplorationInputComponent.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaRhythmHUDWidget.h"

AMelodiaCharacterBase::AMelodiaCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CosmeticsComponent = CreateDefaultSubobject<UMelodiaCosmeticsComponent>(TEXT("MelodiaCosmetics"));
	ExplorationInputComponent = CreateDefaultSubobject<UMelodiaExplorationInputComponent>(TEXT("MelodiaExplorationInput"));
	InventoryComponent = CreateDefaultSubobject<UMelodiaInventoryComponent>(TEXT("MelodiaInventory"));

	ConfigureExplorationMovement();

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;
}

void AMelodiaCharacterBase::ConfigureExplorationMovement()
{
	if (UCharacterMovementComponent* Movement = GetCharacterMovement())
	{
		Movement->bOrientRotationToMovement = true;
		Movement->RotationRate = FRotator(0.0f, 540.0f, 0.0f);
		Movement->MaxWalkSpeed = 450.0f;
		Movement->JumpZVelocity = 420.0f;
		Movement->AirControl = 0.25f;
		Movement->BrakingDecelerationWalking = 1400.0f;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCapsuleHalfHeight(88.0f);
		Capsule->SetCapsuleRadius(34.0f);
	}
}

void AMelodiaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	InitializeExplorationSystems();
}

void AMelodiaCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (bAutoBindExplorationInput && ExplorationInputComponent)
	{
		ExplorationInputComponent->BindExplorationInput();
	}
}

void AMelodiaCharacterBase::InitializeExplorationSystems()
{
	if (bSeedStarterInventory && InventoryComponent && InventoryComponent->Slots.Num() == 0)
	{
		InventoryComponent->SeedStarterKit();
	}

	if (bAutoBindExplorationInput && ExplorationInputComponent)
	{
		ExplorationInputComponent->BindExplorationInput();
	}

	if (bAutoApplyCosmetics)
	{
		ApplyMelusinaPresentation();
	}
}

void AMelodiaCharacterBase::ApplyMelusinaPresentation()
{
	if (CosmeticsComponent)
	{
		CosmeticsComponent->ApplyDefaultMelusinaPreset();
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->ApplyCuteCombatTheme();
			Widget->TriggerSparkleBurst();
		}
	}
}
