// Copyright Melodia Project. All Rights Reserved.
// AMelodiaReverieRunManager — orchestrates procedurally-generated run sessions.

#include "MelodiaReverieRunManager.h"

#include "PCGComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "MelodiaRhythmGameModeBase.h"
#include "MelodiaPCGEncounterSpawner.h"
#include "PCGGraph.h"
#include "UObject/ConstructorHelpers.h"

AMelodiaReverieRunManager::AMelodiaReverieRunManager()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default area templates if none are configured.
	if (AreaTemplates.IsEmpty())
	{
		FReverieAreaConfig DefaultArea;
		DefaultArea.AreaDisplayName = TEXT("Escher Corridor");
		DefaultArea.MinEncounters = 1;
		DefaultArea.MaxEncounters = 2;
		DefaultArea.DifficultyMultiplier = 1.0f;
		AreaTemplates.Add(DefaultArea);
	}
}

void AMelodiaReverieRunManager::BeginPlay()
{
	Super::BeginPlay();

	if (bAutoGenerateOnBeginPlay)
	{
		StartRun(RunSeed);
	}
}

// ---------------------------------------------------------------------------
// Public API
// ---------------------------------------------------------------------------

void AMelodiaReverieRunManager::StartRun(int32 Seed)
{
	if (RunState != EReverieRunState::Idle && RunState != EReverieRunState::Completed && RunState != EReverieRunState::Failed)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: Cannot start run while in state %d."), static_cast<int32>(RunState));
		return;
	}

	// Resolve seed: 0 means pick a random one.
	CurrentRunSeed = (Seed != 0) ? Seed : FMath::Rand();
	CurrentAreaIndex = 0;
	AreasCompleted = 0;
	RunState = EReverieRunState::Generating;

	UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Starting run with seed %d, %d areas."),
		CurrentRunSeed, AreasPerRun);

	GenerateAreaSequence();
	GenerateCurrentArea();
}

void AMelodiaReverieRunManager::AdvanceToNextArea()
{
	if (RunState != EReverieRunState::Ready && RunState != EReverieRunState::InProgress)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: Cannot advance while in state %d."), static_cast<int32>(RunState));
		return;
	}

	++CurrentAreaIndex;
	++AreasCompleted;

	if (IsRunComplete())
	{
		RunState = EReverieRunState::Completed;
		UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Run completed! All %d areas cleared."), AreasPerRun);
		return;
	}

	RunState = EReverieRunState::Transitioning;
	UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Advancing to area %d / %d."), CurrentAreaIndex + 1, AreasPerRun);

	GenerateCurrentArea();
}

void AMelodiaReverieRunManager::AbortRun()
{
	RunState = EReverieRunState::Failed;
	GeneratedAreaSequence.Empty();
	UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: Run aborted at area %d."), CurrentAreaIndex);
}

int32 AMelodiaReverieRunManager::GetCurrentAreaSeed() const
{
	return ComputeAreaSeed(CurrentAreaIndex);
}

bool AMelodiaReverieRunManager::IsRunComplete() const
{
	return CurrentAreaIndex >= AreasPerRun;
}

// ---------------------------------------------------------------------------
// Internal
// ---------------------------------------------------------------------------

void AMelodiaReverieRunManager::OnAreaGenerationComplete()
{
	RunState = EReverieRunState::Ready;
	UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Area %d generation complete (seed=%d)."),
		CurrentAreaIndex, GetCurrentAreaSeed());

	// Spawn encounters for this area using PCG-driven placement.
	SpawnEncountersForCurrentArea();

	// Notify the game mode that exploration is ready.
	if (AMelodiaRhythmGameModeBase* GameMode = Cast<AMelodiaRhythmGameModeBase>(UGameplayStatics::GetGameMode(this)))
	{
		GameMode->SetLoopPhase(EMelodiaLoopPhase::ExplorationReady);
	}
}

void AMelodiaReverieRunManager::GenerateAreaSequence()
{
	GeneratedAreaSequence.Reset();
	GeneratedAreaSequence.Reserve(AreasPerRun);

	// Use a deterministic PRNG seeded by the run seed.
	FRandomStream Rng(CurrentRunSeed);

	for (int32 i = 0; i < AreasPerRun; ++i)
	{
		if (AreaTemplates.Num() > 0)
		{
			const int32 TemplateIdx = Rng.RandRange(0, AreaTemplates.Num() - 1);
			FReverieAreaConfig Area = AreaTemplates[TemplateIdx];

			// Scale difficulty with area progression.
			Area.DifficultyMultiplier *= 1.0f + (static_cast<float>(i) * 0.15f);

			// Randomize encounter count within range.
			Area.MinEncounters = Rng.RandRange(Area.MinEncounters, Area.MaxEncounters);
			Area.MaxEncounters = Area.MinEncounters;

			Area.AreaDisplayName = FString::Printf(TEXT("%s (Area %d)"), *Area.AreaDisplayName, i + 1);
			GeneratedAreaSequence.Add(Area);
		}
		else
		{
			// Fallback: create a default area.
			FReverieAreaConfig DefaultArea;
			DefaultArea.AreaDisplayName = FString::Printf(TEXT("Area %d"), i + 1);
			DefaultArea.DifficultyMultiplier = 1.0f + (static_cast<float>(i) * 0.15f);
			DefaultArea.MinEncounters = Rng.RandRange(1, 3);
			DefaultArea.MaxEncounters = DefaultArea.MinEncounters;
			GeneratedAreaSequence.Add(DefaultArea);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Generated %d-area sequence."), GeneratedAreaSequence.Num());
	for (int32 i = 0; i < GeneratedAreaSequence.Num(); ++i)
	{
		UE_LOG(LogTemp, Verbose, TEXT("  [%d] %s (diff=%.2f, encounters=%d)"),
			i, *GeneratedAreaSequence[i].AreaDisplayName,
			GeneratedAreaSequence[i].DifficultyMultiplier,
			GeneratedAreaSequence[i].MaxEncounters);
	}
}

void AMelodiaReverieRunManager::GenerateCurrentArea()
{
	if (!GeneratedAreaSequence.IsValidIndex(CurrentAreaIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: No area config for index %d."), CurrentAreaIndex);
		OnAreaGenerationComplete();
		return;
	}

	const FReverieAreaConfig& Area = GeneratedAreaSequence[CurrentAreaIndex];
	const int32 AreaSeed = ComputeAreaSeed(CurrentAreaIndex);

	// Find PCG component in the world.
	UPCGComponent* PCGComp = FindPCGComponent();
	if (!PCGComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: No PCG component found. Skipping generation."));
		OnAreaGenerationComplete();
		return;
	}

	// If the area has a specific PCG graph asset, try to assign it.
	if (Area.PCGGraphAsset.IsValid())
	{
		// Async load the graph asset.
		TArray<FSoftObjectPath> AssetsToLoad;
		AssetsToLoad.Add(Area.PCGGraphAsset);

		// For now, attempt a synchronous load (can be made async later).
		UObject* LoadedGraph = Area.PCGGraphAsset.TryLoad();
		if (UPCGGraph* Graph = Cast<UPCGGraph>(LoadedGraph))
		{
			PCGComp->SetGraph(Graph);
			UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Assigned PCG graph '%s' for area %d."),
				*Graph->GetName(), CurrentAreaIndex);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("ReverieRunManager: Failed to load PCG graph '%s'. Using existing graph."),
				*Area.PCGGraphAsset.ToString());
		}
	}

	// Set the seed on the PCG component (UE 5.7 removed SetSeed; assign the property).
	PCGComp->Seed = AreaSeed;

	// Trigger generation.
	PCGComp->Generate();
	UE_LOG(LogTemp, Log, TEXT("ReverieRunManager: Triggered PCG generation for area %d '%s' (seed=%d)."),
		CurrentAreaIndex, *Area.AreaDisplayName, AreaSeed);

	// In a full implementation we'd bind to the PCG component's OnGenerationCompleted
	// delegate.  For the skeleton, we call OnAreaGenerationComplete directly since
	// PCG generation is synchronous in-editor when not using async generation.
	OnAreaGenerationComplete();
}

UPCGComponent* AMelodiaReverieRunManager::FindPCGComponent() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	// Find the first actor with a PCG component.
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (PCGComp)
		{
			return PCGComp;
		}
	}

	return nullptr;
}

int32 AMelodiaReverieRunManager::ComputeAreaSeed(int32 AreaIndex) const
{
	// Deterministic per-area seed: hash the run seed with the area index.
	// Uses a simple but effective mixing function.
	const int32 RawSeed = CurrentRunSeed ^ (AreaIndex * 2654435761);
	return FMath::Abs(RawSeed);
}

// ---------------------------------------------------------------------------
// SpawnEncountersForCurrentArea
// ---------------------------------------------------------------------------

void AMelodiaReverieRunManager::SpawnEncountersForCurrentArea()
{
	if (!GeneratedAreaSequence.IsValidIndex(CurrentAreaIndex))
	{
		return;
	}

	const FReverieAreaConfig& Area = GeneratedAreaSequence[CurrentAreaIndex];
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Find the PCG component to determine spawn location.
	UPCGComponent* PCGComp = FindPCGComponent();
	if (!PCGComp)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("ReverieRunManager: No PCG component for encounter spawning."));
		return;
	}

	const FVector SpawnLoc = PCGComp->GetOwner()->GetActorLocation();

	// Find or create the encounter spawner.
	if (!ActiveEncounterSpawner)
	{
		UClass* SpawnerClass = EncounterSpawnerClass
			? *EncounterSpawnerClass
			: AMelodiaPCGEncounterSpawner::StaticClass();

		FActorSpawnParameters Params;
		Params.SpawnCollisionHandlingOverride =
			ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		ActiveEncounterSpawner = World->SpawnActor<AMelodiaPCGEncounterSpawner>(
			SpawnerClass, SpawnLoc, FRotator::ZeroRotator, Params);
	}

	if (ActiveEncounterSpawner)
	{
		ActiveEncounterSpawner->SetActorLocation(SpawnLoc);
		ActiveEncounterSpawner->MaxEncounters = Area.MaxEncounters;
		const int32 Spawned = ActiveEncounterSpawner->ScanAndSpawn();

		UE_LOG(LogTemp, Log,
			TEXT("ReverieRunManager: Spawned %d encounters for area %d '%s'."),
			Spawned, CurrentAreaIndex, *Area.AreaDisplayName);
	}
}
