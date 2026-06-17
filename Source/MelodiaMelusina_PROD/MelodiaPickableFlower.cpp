#include "MelodiaPickableFlower.h"

#include "Components/SphereComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "GameFramework/Pawn.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "MelodiaInventoryComponent.h"
#include "MelodiaMechanicProgressionSubsystem.h"
#include "MelodiaQuestManagerBase.h"
#include "MelodiaRhythmHUDWidget.h"
#include "UObject/ConstructorHelpers.h"

AMelodiaPickableFlower::AMelodiaPickableFlower()
{
	PrimaryActorTick.bCanEverTick = false;

	PickRadius = CreateDefaultSubobject<USphereComponent>(TEXT("PickRadius"));
	RootComponent = PickRadius;
	PickRadius->InitSphereRadius(120.0f);
	PickRadius->SetCollisionProfileName(TEXT("OverlapAllDynamic"));

	StemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StemMesh"));
	StemMesh->SetupAttachment(PickRadius);
	StemMesh->SetRelativeLocation(FVector(0.0f, 0.0f, -20.0f));
	StemMesh->SetRelativeScale3D(FVector(0.08f, 0.08f, 0.35f));
	StemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	BloomMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BloomMesh"));
	BloomMesh->SetupAttachment(StemMesh);
	BloomMesh->SetRelativeLocation(FVector(0.0f, 0.0f, 55.0f));
	BloomMesh->SetRelativeScale3D(FVector(0.22f, 0.22f, 0.12f));
	BloomMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderMesh(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> SphereMesh(TEXT("/Engine/BasicShapes/Sphere.Sphere"));
	if (CylinderMesh.Succeeded())
	{
		StemMesh->SetStaticMesh(CylinderMesh.Object);
	}
	if (SphereMesh.Succeeded())
	{
		BloomMesh->SetStaticMesh(SphereMesh.Object);
	}
}

void AMelodiaPickableFlower::BeginPlay()
{
	Super::BeginPlay();
	ApplyBloomTint();
}

void AMelodiaPickableFlower::ApplyBloomTint()
{
	if (BloomMesh)
	{
		BloomMesh->SetVectorParameterValueOnMaterials(TEXT("Color"), FVector(BloomTint.R, BloomTint.G, BloomTint.B));
		if (UMaterialInstanceDynamic* MID = BloomMesh->CreateAndSetMaterialInstanceDynamic(0))
		{
			MID->SetVectorParameterValue(TEXT("Color"), BloomTint);
		}
	}
}

bool AMelodiaPickableFlower::CanPick(const APawn* Pawn) const
{
	if (bPicked || !Pawn || !PickRadius)
	{
		return false;
	}

	const float Radius = PickRadius->GetScaledSphereRadius();
	return FVector::DistSquared(Pawn->GetActorLocation(), GetActorLocation()) <= FMath::Square(Radius);
}

FString AMelodiaPickableFlower::GetPickPromptText() const
{
	return bPicked ? FString() : PickPrompt;
}

bool AMelodiaPickableFlower::TryPick(APawn* InstigatorPawn)
{
	if (!CanPick(InstigatorPawn))
	{
		return false;
	}

	UMelodiaInventoryComponent* Inventory = InstigatorPawn->FindComponentByClass<UMelodiaInventoryComponent>();
	if (!Inventory || !Inventory->AddItem(FlowerItemId, DisplayName, PickQuantity, BloomTint))
	{
		return false;
	}

	++PickCount;
	bPicked = true;
	HideFlowerVisuals();

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<AMelodiaQuestManagerBase> It(World); It; ++It)
		{
			It->NotifyFlowerPicked(FlowerItemId, PickQuantity);
			It->SyncHUD(World);
			break;
		}

		if (UGameInstance* GI = World->GetGameInstance())
		{
			if (UMelodiaMechanicProgressionSubsystem* Progression = GI->GetSubsystem<UMelodiaMechanicProgressionSubsystem>())
			{
				Progression->GrantMechanicXP(8, TEXT("Picked flower"));
			}
		}

		if (UMelodiaRhythmHUDWidget* Widget = UMelodiaRhythmHUDWidget::FindFirst(World))
		{
			Widget->ShowActionPrompt(FString::Printf(TEXT("Picked %s"), *DisplayName));
			Widget->PushFloatingCombatText(TEXT("+Blossom"), false, BloomTint);
			Widget->TriggerSparkleBurst();
		}
	}

	UE_LOG(LogTemp, Log, TEXT("Melodia flower picked: %s"), *DisplayName);
	return true;
}

void AMelodiaPickableFlower::HideFlowerVisuals()
{
	if (StemMesh)
	{
		StemMesh->SetHiddenInGame(true);
	}
	if (BloomMesh)
	{
		BloomMesh->SetHiddenInGame(true);
	}
	if (PickRadius)
	{
		PickRadius->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}
}
