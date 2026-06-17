#include "MelodiaGameplayLoopTestDirector.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaNPCBase.h"
#include "MelodiaPickableFlower.h"
#include "MelodiaPortal.h"
#include "MelodiaRestPoint.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaWorldEnemy.h"
#include "UObject/ConstructorHelpers.h"

AMelodiaGameplayLoopTestDirector::AMelodiaGameplayLoopTestDirector()
{
	PrimaryActorTick.bCanEverTick = false;
	EnsureDefaultFlowerOffsets();
}

void AMelodiaGameplayLoopTestDirector::EnsureDefaultFlowerOffsets()
{
	if (FlowerOffsets.Num() > 0)
	{
		return;
	}

	FlowerOffsets = {
		FVector(-900.0f, -300.0f, 120.0f),
		FVector(-700.0f, -150.0f, 120.0f),
		FVector(-500.0f, 0.0f, 120.0f),
	};
}

void AMelodiaGameplayLoopTestDirector::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoBuildLayout)
	{
		BuildLayout();
	}

	if (bApplyToGameModeOnBeginPlay)
	{
		if (AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
		{
			ApplyToGameMode(GameMode);
		}
	}
}

bool AMelodiaGameplayLoopTestDirector::BuildLayout()
{
	UWorld* World = GetWorld();
	if (!World || bLayoutBuilt)
	{
		return false;
	}

	EnsureDefaultFlowerOffsets();
	EnsureArenaFloor();
	EnsurePlayerStart();

	const FVector SpawnLocation = GetPlayerSpawnLocation();
	const FVector GateLocation = GetEncounterGateLocation();

	if (!EncounterGate)
	{
		for (TActorIterator<AMelodiaEncounterTrigger> It(World); It; ++It)
		{
			if (It->IsA<AMelodiaWorldEnemy>())
			{
				continue;
			}
			EncounterGate = *It;
			break;
		}
	}

	if (!EncounterGate)
	{
		UClass* GateClass = ResolveOptionalClass(EncounterGateClassPath, AMelodiaEncounterTrigger::StaticClass());
		EncounterGate = Cast<AMelodiaEncounterTrigger>(SpawnLoopActor(GateClass, GateLocation, FRotator::ZeroRotator, TEXT("Melodia.TestLoop.Gate")));
	}

	if (!WorldEnemy)
	{
		for (TActorIterator<AMelodiaWorldEnemy> It(World); It; ++It)
		{
			WorldEnemy = *It;
			break;
		}
	}

	if (!WorldEnemy)
	{
		UClass* EnemyClass = ResolveOptionalClass(WorldEnemyClassPath, AMelodiaWorldEnemy::StaticClass());
		WorldEnemy = Cast<AMelodiaWorldEnemy>(SpawnLoopActor(
			EnemyClass,
			GateLocation + FVector(0.0f, 180.0f, 0.0f),
			FRotator(0.0f, 180.0f, 0.0f),
			TEXT("Melodia.TestLoop.Enemy")));
		if (WorldEnemy)
		{
			WorldEnemy->EnemyDisplayName = TEXT("Garden Slime");
		}
	}

	if (!QuestGiver)
	{
		for (TActorIterator<AMelodiaNPCBase> It(World); It; ++It)
		{
			QuestGiver = *It;
			break;
		}
	}

	if (!QuestGiver)
	{
		UClass* NPCClass = ResolveOptionalClass(QuestGiverClassPath, AMelodiaNPCBase::StaticClass());
		QuestGiver = Cast<AMelodiaNPCBase>(SpawnLoopActor(
			NPCClass,
			ArenaOrigin + QuestGiverOffset,
			FRotator(0.0f, 45.0f, 0.0f),
			TEXT("Melodia.TestLoop.QuestGiver")));
		if (QuestGiver)
		{
			QuestGiver->DisplayName = TEXT("Blossom Tutor");
			QuestGiver->QuestId = TEXT("BlossomGatherer");
			QuestGiver->DialogueLine = TEXT("Pick three Reverie Blossoms, then test the slime at the gate.");
			QuestGiver->InteractionPrompt = TEXT("F: Talk");
		}
	}

	if (!RestBed)
	{
		for (TActorIterator<AMelodiaRestPoint> It(World); It; ++It)
		{
			RestBed = *It;
			break;
		}
	}

	if (!RestBed)
	{
		UClass* BedClass = ResolveOptionalClass(RestBedClassPath, AMelodiaRestPoint::StaticClass());
		RestBed = Cast<AMelodiaRestPoint>(SpawnLoopActor(
			BedClass,
			ArenaOrigin + RestBedOffset,
			FRotator::ZeroRotator,
			TEXT("Melodia.TestLoop.Bed")));
		if (RestBed)
		{
			RestBed->DisplayName = TEXT("Melusina's Bed");
			RestBed->InteractionPrompt = TEXT("E: Rest & save");
		}
	}

	if (!Portal)
	{
		for (TActorIterator<AMelodiaPortal> It(World); It; ++It)
		{
			Portal = *It;
			break;
		}
	}

	if (!Portal)
	{
		UClass* PortalClass = ResolveOptionalClass(PortalClassPath, AMelodiaPortal::StaticClass());
		Portal = Cast<AMelodiaPortal>(SpawnLoopActor(
			PortalClass,
			ArenaOrigin + PortalOffset,
			FRotator(0.0f, -45.0f, 0.0f),
			TEXT("Melodia.TestLoop.Portal")));
		if (Portal)
		{
			Portal->DisplayName = TEXT("Return Portal");
			Portal->InteractionPrompt = TEXT("E: Warp to spawn");
			Portal->bUseTargetWorldLocation = true;
			Portal->TargetWorldLocation = SpawnLocation + FVector(0.0f, 0.0f, 20.0f);
		}
	}

	int32 ExistingFlowers = 0;
	for (TActorIterator<AMelodiaPickableFlower> It(World); It; ++It)
	{
		++ExistingFlowers;
		Flowers.AddUnique(*It);
	}

	if (ExistingFlowers < 3)
	{
		UClass* FlowerClass = ResolveOptionalClass(FlowerClassPath, AMelodiaPickableFlower::StaticClass());
		const FLinearColor FlowerTints[] = {
			FLinearColor(0.98f, 0.52f, 0.86f, 1.0f),
			FLinearColor(0.72f, 0.62f, 0.98f, 1.0f),
			FLinearColor(0.98f, 0.82f, 0.42f, 1.0f),
		};

		for (int32 Index = 0; Index < FlowerOffsets.Num(); ++Index)
		{
			if (Flowers.Num() >= 3 && ExistingFlowers >= 3)
			{
				break;
			}

			const FVector FlowerLocation = ArenaOrigin + FlowerOffsets[Index];
			AMelodiaPickableFlower* Flower = Cast<AMelodiaPickableFlower>(SpawnLoopActor(
				FlowerClass,
				FlowerLocation,
				FRotator(0.0f, static_cast<float>(Index) * 35.0f, 0.0f),
				TEXT("Melodia.TestLoop.Flower")));
			if (Flower)
			{
				Flower->BloomTint = FlowerTints[Index % UE_ARRAY_COUNT(FlowerTints)];
				Flower->DisplayName = FString::Printf(TEXT("Reverie Blossom %d"), Index + 1);
				Flowers.Add(Flower);
			}
		}
	}

	bLayoutBuilt = true;
	UE_LOG(LogTemp, Log, TEXT("Melodia test loop director built arena at %s (spawn=%s gate=%s)."),
		*ArenaOrigin.ToString(),
		*SpawnLocation.ToString(),
		*GateLocation.ToString());
	return true;
}

void AMelodiaGameplayLoopTestDirector::ApplyToGameMode(AMelodiaRhythmGameModeBase* GameMode) const
{
	if (!GameMode)
	{
		return;
	}

	GameMode->ConfigureGameplayLoopTest(GetPlayerSpawnLocation(), GetEncounterGateLocation());
}

void AMelodiaGameplayLoopTestDirector::EnsureArenaFloor()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	for (TActorIterator<AStaticMeshActor> It(World); It; ++It)
	{
		if (It->ActorHasTag(TEXT("Melodia.TestLoop.Floor")))
		{
			return;
		}
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	AStaticMeshActor* Floor = World->SpawnActor<AStaticMeshActor>(AStaticMeshActor::StaticClass(), ArenaOrigin, FRotator::ZeroRotator, Params);
	if (!Floor)
	{
		return;
	}

	Floor->Tags.Add(TEXT("Melodia.TestLoop.Floor"));
	if (UStaticMeshComponent* Mesh = Floor->GetStaticMeshComponent())
	{
		static ConstructorHelpers::FObjectFinder<UStaticMesh> CubeMesh(TEXT("/Engine/BasicShapes/Cube.Cube"));
		if (CubeMesh.Succeeded())
		{
			Mesh->SetStaticMesh(CubeMesh.Object);
		}
		Mesh->SetWorldScale3D(FVector(50.0f, 50.0f, 0.5f));
		Mesh->SetRelativeLocation(FVector(0.0f, 0.0f, -25.0f));
		Mesh->SetCollisionProfileName(TEXT("BlockAll"));
	}
}

void AMelodiaGameplayLoopTestDirector::EnsurePlayerStart()
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector SpawnLocation = GetPlayerSpawnLocation();
	for (TActorIterator<APlayerStart> It(World); It; ++It)
	{
		PlayerStartActor = *It;
		PlayerStartActor->SetActorLocation(SpawnLocation);
		PlayerStartActor->Tags.AddUnique(TEXT("Melodia.TestLoop.Spawn"));
		return;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
	PlayerStartActor = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), SpawnLocation, FRotator::ZeroRotator, Params);
	if (PlayerStartActor)
	{
		PlayerStartActor->Tags.Add(TEXT("Melodia.TestLoop.Spawn"));
	}
}

AActor* AMelodiaGameplayLoopTestDirector::SpawnLoopActor(
	UClass* ActorClass,
	const FVector& Location,
	const FRotator& Rotation,
	const FName ActorTag) const
{
	UWorld* World = GetWorld();
	if (!World || !ActorClass)
	{
		return nullptr;
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
	AActor* Spawned = World->SpawnActor<AActor>(ActorClass, Location, Rotation, Params);
	if (Spawned && !ActorTag.IsNone())
	{
		Spawned->Tags.Add(ActorTag);
	}
	return Spawned;
}

UClass* AMelodiaGameplayLoopTestDirector::ResolveOptionalClass(const FSoftClassPath& ClassPath, UClass* NativeFallback) const
{
	if (UClass* Resolved = ClassPath.TryLoadClass<AActor>())
	{
		return Resolved;
	}
	return NativeFallback;
}
