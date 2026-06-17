// Cockatoo companion: follows player in explore, grants battle party buff at Lv8+.

#include "MelodiaCompanionActor.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaCombatStateComponent.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"

AMelodiaCompanionActor::AMelodiaCompanionActor()
{
	PrimaryActorTick.bCanEverTick = true;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	SetRootComponent(InteractionSphere);
	InteractionSphere->InitSphereRadius(120.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VisualMesh"));
	VisualMesh->SetupAttachment(InteractionSphere);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -40.0f));
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
		VisualMesh->SetRelativeScale3D(FVector(0.35f));
	}
}

void AMelodiaCompanionActor::BeginPlay()
{
	Super::BeginPlay();
	RefreshFollowTarget();

	if (UGameInstance* GI = GetGameInstance())
	{
		if (const UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
		{
			SetCompanionUnlocked(Progression->State.bCompanionUnlocked);
		}
	}

	SetActorHiddenInGame(!bCompanionUnlocked);
}

void AMelodiaCompanionActor::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (bCompanionUnlocked && bFollowingPlayer)
	{
		TickFollow(DeltaSeconds);
	}
}

void AMelodiaCompanionActor::SetCompanionUnlocked(const bool bUnlocked)
{
	bCompanionUnlocked = bUnlocked;
	SetActorHiddenInGame(!bUnlocked);
	if (bUnlocked)
	{
		RefreshFollowTarget();
	}
}

void AMelodiaCompanionActor::ApplyBattleStartBuff(UMelodiaCombatStateComponent* CombatState) const
{
	if (!bCompanionUnlocked || !CombatState)
	{
		return;
	}

	switch (BattleBuffType)
	{
	case EMelodiaCompanionBuffType::DamageBoost:
		CombatState->SkillPoints = FMath::Clamp(CombatState->SkillPoints, 0, CombatState->SkillPointMax);
		break;
	case EMelodiaCompanionBuffType::SkillPointRegen:
	default:
		CombatState->AddSkillPoints(BonusSkillPointsAtBattleStart);
		break;
	}
}

void AMelodiaCompanionActor::RefreshFollowTarget()
{
	FollowTarget = ResolvePlayerPawn();
	bFollowingPlayer = FollowTarget.IsValid();
}

FString AMelodiaCompanionActor::GetDisplayName_Implementation() const
{
	return DisplayName;
}

FString AMelodiaCompanionActor::GetInteractionPrompt_Implementation() const
{
	if (!bCompanionUnlocked)
	{
		return FString::Printf(TEXT("Locked until Mechanic Lv %d"), UnlockMechanicLevel);
	}
	return InteractionPrompt;
}

bool AMelodiaCompanionActor::ActivateInteraction_Implementation(APawn* InstigatorPawn)
{
	if (!bCompanionUnlocked)
	{
		return false;
	}

	if (UWorld* World = GetWorld())
	{
		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->SetJudgmentString(FString::Printf(TEXT("%s chirps happily!"), *DisplayName));
			Widget->TriggerSparkleBurst();
		}
	}
	return true;
}

bool AMelodiaCompanionActor::CanInteract_Implementation(APawn* InstigatorPawn) const
{
	return bCompanionUnlocked && InstigatorPawn != nullptr;
}

USphereComponent* AMelodiaCompanionActor::GetInteractionSphere_Implementation() const
{
	return InteractionSphere;
}

void AMelodiaCompanionActor::TickFollow(const float DeltaSeconds)
{
	APawn* Target = FollowTarget.Get();
	if (!Target)
	{
		Target = ResolvePlayerPawn();
		FollowTarget = Target;
	}

	if (!Target)
	{
		return;
	}

	const FVector TargetLocation = Target->GetActorLocation();
	const FVector ToTarget = TargetLocation - GetActorLocation();
	const float Distance = ToTarget.Size2D();

	if (Distance > FollowDistance)
	{
		const FVector Direction = ToTarget.GetSafeNormal2D();
		const FVector NewLocation = GetActorLocation() + Direction * FollowSpeed * DeltaSeconds;
		SetActorLocation(FVector(NewLocation.X, NewLocation.Y, TargetLocation.Z + 60.0f));
	}
}

APawn* AMelodiaCompanionActor::ResolvePlayerPawn() const
{
	if (UWorld* World = GetWorld())
	{
		return UGameplayStatics::GetPlayerPawn(World, 0);
	}
	return nullptr;
}
