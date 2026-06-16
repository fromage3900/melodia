// Blueprintable exploration portal for Melodia map/area flow.

#include "MelodiaPortal.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"
#include "UObject/UObjectIterator.h"

AMelodiaPortal::AMelodiaPortal()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("InteractionSphere"));
	RootComponent = InteractionSphere;
	InteractionSphere->InitSphereRadius(240.0f);
	InteractionSphere->SetCollisionProfileName(TEXT("Trigger"));

	VisualMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PortalMarker"));
	VisualMesh->SetupAttachment(InteractionSphere);
	VisualMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	VisualMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 120.0f));
	VisualMesh->SetRelativeScale3D(FVector(1.1f, 1.1f, 1.1f));
	VisualMesh->SetRenderCustomDepth(true);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (SphereMesh.Succeeded())
	{
		VisualMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AMelodiaPortal::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Log, TEXT("%s ready at %s"), *DisplayName, *GetActorLocation().ToString());
}

bool AMelodiaPortal::IsPawnInRange(const APawn* Pawn) const
{
	if (!Pawn || !InteractionSphere)
	{
		return false;
	}

	const float Radius = InteractionSphere->GetScaledSphereRadius();
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

bool AMelodiaPortal::ActivatePortal(APawn* InstigatorPawn)
{
	if (!InstigatorPawn || !IsPawnInRange(InstigatorPawn))
	{
		return false;
	}

	++ActivationCount;

	if (!TargetLevelName.IsNone())
	{
		bLastActivationSucceeded = true;
		PublishPortalFeedback(GetWorld(), FString::Printf(TEXT("Entering %s"), *TargetLevelName.ToString()));
		UGameplayStatics::OpenLevel(this, TargetLevelName);
		return true;
	}

	if (bUseTargetWorldLocation && !TargetWorldLocation.IsNearlyZero())
	{
		InstigatorPawn->SetActorLocation(TargetWorldLocation, false, nullptr, ETeleportType::TeleportPhysics);
		bLastActivationSucceeded = true;
		PublishPortalFeedback(GetWorld(), TEXT("Portal shift complete"));
		return true;
	}

	bLastActivationSucceeded = false;
	PublishPortalFeedback(GetWorld(), TEXT("Portal has no destination yet"));
	return false;
}

void AMelodiaPortal::PublishPortalFeedback(UWorld* World, const FString& Text) const
{
	if (!World)
	{
		return;
	}

	for (TObjectIterator<UMelodiaRhythmHUDWidget> It; It; ++It)
	{
		UMelodiaRhythmHUDWidget* Widget = *It;
		if (Widget && Widget->GetWorld() == World)
		{
			Widget->ShowBattleStatus(Text);
			Widget->ShowActionPrompt(Text);
			Widget->PushFloatingCombatText(TEXT("PORTAL"), false, FLinearColor(0.86f, 0.62f, 1.0f, 1.0f));
		}
	}
}
