// Copyright Melodia Project. All Rights Reserved.
// Shared base class for PCG-driven spawners (encounters, decorations).
// Provides common properties (AcceptedRoles, spacing, search radius, walkable
// index reference), candidate collection from PCG data, de-duplication, and
// spawned-actor lifecycle management.  Subclasses override SpawnActorAt() to
// create the appropriate actor type per position.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaPCGSpawnerBase.generated.h"

class AMelodiaPCGWalkableIndex;

/**
 * Abstract base for actors that scan PCG-generated points and spawn child
 * actors at filtered positions.  Handles:
 *   - AcceptedRoles filtering
 *   - MinSpacingCm de-duplication
 *   - WalkableIndex auto-discovery
 *   - Spawned actor tracking and cleanup
 *
 * Subclasses implement SpawnActorAt() to create the concrete actor type.
 */
UCLASS(Abstract)
class MELODIAMELUSINA_PROD_API AMelodiaPCGSpawnerBase : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGSpawnerBase();

	// ── Configuration ────────────────────────────────────────────────

	/** Which architectural roles are valid spawn sites. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner")
	TArray<EPCGArchitecturalRole> AcceptedRoles;

	/** Maximum actors to spawn per scan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner", meta = (ClampMin = "1", ClampMax = "256"))
	int32 MaxSpawnCount = 32;

	/** Minimum distance (cm) between spawned actors. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner", meta = (ClampMin = "50"))
	float MinSpacingCm = 300.0f;

	/** Radius (cm) to search for PCG data. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner", meta = (ClampMin = "500"))
	float SearchRadiusCm = 5000.0f;

	/** If true, use the walkable index for fast PCG attribute queries. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner")
	bool bUseWalkableIndex = true;

	/** Optional explicit reference. Auto-discovered if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Spawner")
	TObjectPtr<AMelodiaPCGWalkableIndex> WalkableIndex;

	// ── Runtime API ──────────────────────────────────────────────────

	/** Scan nearby PCG data and spawn actors at matching positions. Returns count. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Spawner")
	int32 ScanAndSpawn();

	/** Destroy all previously spawned actors. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Spawner")
	virtual void ClearSpawnedActors();

	/** Read-only access to currently spawned actors. */
	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Spawner")
	TArray<AActor*> GetSpawnedActors() const;

protected:
	/** Actors spawned by the last ScanAndSpawn call. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Spawner")
	TArray<TObjectPtr<AActor>> SpawnedActors;

	virtual void BeginPlay() override;

	/**
	 * Create one actor at the given world position.
	 * @param Position   World-space location (already filtered + de-duplicated).
	 * @param ArchRole   Architectural role at this position.
	 * @param OutActor   Set to the spawned actor on success.
	 * @return true if an actor was spawned.
	 */
	virtual bool SpawnActorAt(const FVector& Position, EPCGArchitecturalRole ArchRole, AActor*& OutActor);

	/**
	 * Collect candidate (position, role) pairs from PCG data near this actor.
	 * Default implementation queries WalkableIndex or falls back to PCGLibrary.
	 */
	virtual TArray<TPair<FVector, EPCGArchitecturalRole>> CollectRolePositions() const;

	/** Auto-locate the WalkableIndex actor if not explicitly assigned. */
	AMelodiaPCGWalkableIndex* FindWalkableIndex() const;
};
