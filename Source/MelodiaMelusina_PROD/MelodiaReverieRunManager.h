// Copyright Melodia Project. All Rights Reserved.
// Reverie Run Manager — orchestrates procedurally-generated run sessions.
//
// Each "reverie run" is a self-contained procedural dungeon experience:
//   1. Generate a run seed (or accept one for deterministic replay).
//   2. Configure PCG graphs with seed + difficulty parameters.
//   3. Trigger PCG generation and wait for completion.
//   4. Signal the game mode that the exploration phase is ready.
//   5. On battle victory, optionally regenerate the next area or end the run.
//
// This class is designed to be placed in the level as a Blueprint actor
// (BP_ReverieRunManager) and referenced by the MelodiaRhythmGameModeBase.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MelodiaReverieRunManager.generated.h"

class UPCGComponent;
class UPCGGraph;
class AMelodiaPCGEncounterSpawner;

/** Current state of the procedural run. */
UENUM(BlueprintType)
enum class EReverieRunState : uint8
{
	Idle           UMETA(DisplayName = "Idle"),
	Generating     UMETA(DisplayName = "Generating"),
	Ready          UMETA(DisplayName = "Ready"),
	InProgress     UMETA(DisplayName = "In Progress"),
	Transitioning  UMETA(DisplayName = "Transitioning"),
	Completed      UMETA(DisplayName = "Completed"),
	Failed         UMETA(DisplayName = "Failed"),
};

/** Configuration for a single area/room within a run. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FReverieAreaConfig
{
	GENERATED_BODY()

	/** Soft class path to the PCG graph asset for this area type. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reverie|Area")
	FSoftObjectPath PCGGraphAsset;

	/** Minimum number of encounter triggers to place in this area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reverie|Area", meta = (ClampMin = "0"))
	int32 MinEncounters = 1;

	/** Maximum number of encounter triggers to place in this area. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reverie|Area", meta = (ClampMin = "1"))
	int32 MaxEncounters = 3;

	/** Difficulty multiplier for enemies in this area (1.0 = base). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reverie|Area", meta = (ClampMin = "0.1"))
	float DifficultyMultiplier = 1.0f;

	/** Optional display name for UI. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Reverie|Area")
	FString AreaDisplayName = TEXT("Unknown Area");
};

// ---------------------------------------------------------------------------
// AMelodiaReverieRunManager
// ---------------------------------------------------------------------------
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaReverieRunManager : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaReverieRunManager();

	// --- Run configuration ---

	/** Number of areas in a single run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverie|Config")
	int32 AreasPerRun = 3;

	/** Base seed for the run.  0 = random seed each run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverie|Config")
	int32 RunSeed = 0;

	/** Area templates to choose from when generating a run. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverie|Config")
	TArray<FReverieAreaConfig> AreaTemplates;

	/** Whether to auto-start generation on BeginPlay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverie|Config")
	bool bAutoGenerateOnBeginPlay = false;

	// --- Runtime state (read-only) ---

	UPROPERTY(BlueprintReadOnly, Category = "Reverie|State")
	EReverieRunState RunState = EReverieRunState::Idle;

	UPROPERTY(BlueprintReadOnly, Category = "Reverie|State")
	int32 CurrentAreaIndex = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Reverie|State")
	int32 CurrentRunSeed = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Reverie|State")
	int32 AreasCompleted = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Reverie|State")
	TArray<FReverieAreaConfig> GeneratedAreaSequence;

	/** Optional class override for the encounter spawner. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Reverie|Encounters")
	TSubclassOf<AMelodiaPCGEncounterSpawner> EncounterSpawnerClass;

	/** Active encounter spawner (created during area generation). */
	UPROPERTY(BlueprintReadOnly, Category = "Reverie|Encounters")
	TObjectPtr<AMelodiaPCGEncounterSpawner> ActiveEncounterSpawner;

	// --- Blueprint-callable API ---

	/** Start a new procedural run.  Generates the area sequence and begins generation. */
	UFUNCTION(BlueprintCallable, Category = "Reverie|Run")
	void StartRun(int32 Seed = 0);

	/** Regenerate the current area with a new seed (e.g. after victory). */
	UFUNCTION(BlueprintCallable, Category = "Reverie|Run")
	void AdvanceToNextArea();

	/** Abort the current run and return to Idle. */
	UFUNCTION(BlueprintCallable, Category = "Reverie|Run")
	void AbortRun();

	/** Get the seed for the current area. */
	UFUNCTION(BlueprintPure, Category = "Reverie|Run")
	int32 GetCurrentAreaSeed() const;

	/** Check if the run is complete (all areas cleared). */
	UFUNCTION(BlueprintPure, Category = "Reverie|Run")
	bool IsRunComplete() const;

protected:
	virtual void BeginPlay() override;

	/** Called when PCG generation completes for the current area. */
	UFUNCTION(BlueprintCallable, Category = "Reverie|Internal")
	void OnAreaGenerationComplete();

	/** Generate the sequence of areas for this run using the run seed. */
	void GenerateAreaSequence();

	/** Configure and trigger PCG generation for the current area. */
	void GenerateCurrentArea();

	/** Find the first PCG component in the world that we can drive. */
	UPCGComponent* FindPCGComponent() const;

	/** Derive a deterministic per-area seed from the run seed and area index. */
	int32 ComputeAreaSeed(int32 AreaIndex) const;

	/** Spawn encounter triggers for the current area using the encounter spawner. */
	void SpawnEncountersForCurrentArea();

private:
	/** FStreamableHandle for async-loaded PCG graph assets. */
	TSharedPtr<struct FStreamableHandle> ActiveStreamableHandle;
};
