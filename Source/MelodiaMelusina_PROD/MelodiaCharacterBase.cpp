// Native exploration hero base: locomotion pawn with Melodia gameplay components.

#include "MelodiaCharacterBase.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "InputCoreTypes.h"
#include "MelodiaCosmeticsComponent.h"
#include "MelodiaExplorationInputComponent.h"
#include "MelodiaGlideComponent.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaRhythmHUDWidget.h"

AMelodiaCharacterBase::AMelodiaCharacterBase()
{
	PrimaryActorTick.bCanEverTick = false;

	CosmeticsComponent = CreateDefaultSubobject<UMelodiaCosmeticsComponent>(TEXT("MelodiaCosmetics"));
	ExplorationInputComponent = CreateDefaultSubobject<UMelodiaExplorationInputComponent>(TEXT("MelodiaExplorationInput"));
	InventoryComponent = CreateDefaultSubobject<UMelodiaInventoryComponent>(TEXT("MelodiaInventory"));
	GlideComponent = CreateDefaultSubobject<UMelodiaGlideComponent>(TEXT("MelodiaGlide"));

	ConfigureExplorationMovement();
	ConfigureExplorationCamera();

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
		Movement->JumpZVelocity = 520.0f;
		Movement->AirControl = 0.35f;
		Movement->BrakingDecelerationWalking = 1400.0f;
	}

	if (UCapsuleComponent* Capsule = GetCapsuleComponent())
	{
		Capsule->SetCapsuleHalfHeight(88.0f);
		Capsule->SetCapsuleRadius(34.0f);
	}

	if (USkeletalMeshComponent* MeshComponent = GetMesh())
	{
		MeshComponent->SetRelativeLocation(FVector(0.0f, 0.0f, -88.0f));
		MeshComponent->SetRelativeRotation(FRotator(0.0f, -90.0f, 0.0f));
	}
}

void AMelodiaCharacterBase::ConfigureExplorationCamera()
{
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 520.0f;
	CameraBoom->SetRelativeRotation(FRotator(-42.0f, 0.0f, 0.0f));
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.0f;
	CameraBoom->bDoCollisionTest = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;
}

void AMelodiaCharacterBase::BeginPlay()
{
	Super::BeginPlay();
	EnsureDisplayMesh();
	InitializeExplorationSystems();

	if (GlideComponent)
	{
		GlideComponent->ApplyMovementDefaults();
	}
}

void AMelodiaCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (APlayerController* PlayerController = Cast<APlayerController>(NewController))
	{
		PlayerController->SetViewTarget(this);
	}

	if (bAutoBindExplorationInput && ExplorationInputComponent)
	{
		ExplorationInputComponent->BindExplorationInput();
	}
}

void AMelodiaCharacterBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	check(PlayerInputComponent);

	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AMelodiaCharacterBase::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"), this, &AMelodiaCharacterBase::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"), this, &AMelodiaCharacterBase::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUp"), this, &AMelodiaCharacterBase::LookUp);
	PlayerInputComponent->BindAxis(TEXT("TurnRate"), this, &AMelodiaCharacterBase::Turn);
	PlayerInputComponent->BindAxis(TEXT("LookUpRate"), this, &AMelodiaCharacterBase::LookUp);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed, this, &AMelodiaCharacterBase::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &AMelodiaCharacterBase::StopJumping);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Pressed, this, &AMelodiaCharacterBase::Jump);
	PlayerInputComponent->BindKey(EKeys::SpaceBar, IE_Released, this, &AMelodiaCharacterBase::StopJumping);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Pressed, this, &AMelodiaCharacterBase::Jump);
	PlayerInputComponent->BindKey(EKeys::LeftShift, IE_Released, this, &AMelodiaCharacterBase::StopJumping);
}

void AMelodiaCharacterBase::Jump()
{
	Super::Jump();
	if (GlideComponent)
	{
		GlideComponent->NotifyJumpPressed();
	}
}

void AMelodiaCharacterBase::StopJumping()
{
	Super::StopJumping();
	if (GlideComponent)
	{
		GlideComponent->NotifyJumpReleased();
	}
}

void AMelodiaCharacterBase::MoveForward(const float Value)
{
	if (Value == 0.0f || !Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X), Value);
}

void AMelodiaCharacterBase::MoveRight(const float Value)
{
	if (Value == 0.0f || !Controller)
	{
		return;
	}

	const FRotator ControlRotation = Controller->GetControlRotation();
	const FRotator YawRotation(0.0f, ControlRotation.Yaw, 0.0f);
	AddMovementInput(FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y), Value);
}

void AMelodiaCharacterBase::Turn(const float Value)
{
	AddControllerYawInput(Value);
}

void AMelodiaCharacterBase::LookUp(const float Value)
{
	AddControllerPitchInput(Value);
}

void AMelodiaCharacterBase::InitializeExplorationSystems()
{
	EnsureDisplayMesh();

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

void AMelodiaCharacterBase::EnsureDisplayMesh()
{
	if (!bUsePlaceholderMeshWhenEmpty || PlaceholderSkeletalMesh.IsNull())
	{
		return;
	}

	USkeletalMeshComponent* MeshComponent = GetMesh();
	if (!MeshComponent || MeshComponent->GetSkeletalMeshAsset() != nullptr)
	{
		return;
	}

	if (USkeletalMesh* PlaceholderMesh = PlaceholderSkeletalMesh.LoadSynchronous())
	{
		MeshComponent->SetSkeletalMesh(PlaceholderMesh);
		UE_LOG(LogTemp, Log, TEXT("MelodiaCharacterBase: applied placeholder mesh %s"), *PlaceholderMesh->GetName());
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
