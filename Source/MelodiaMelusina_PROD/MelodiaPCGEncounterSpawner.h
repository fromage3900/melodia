// Copyright Melodia Project. All Rights Reserved.
// Reads PCG ArchitecturalRole attributes from generated points and spawns
// encounter triggers at semantically appropriate locations (e.g. Stair, Tile).

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaPCGEncounterSpawner.generated.h"

class UPCGComponent;
class AMelodiaPCGWalkableIndex;

/**
 * Scans PCG-generated points for ArchitecturalRole attributes and spawns
 * AMelodiaEncounterTrigger actors at matching locations.  Designed to be
 * driven by AMelodiaReverieRunManager during procedural area generation.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGEncounterSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGEncounterSpawner();

	/** Which architectural roles are valid encounter spawn sites. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter")
	TArray<EPCGArchitecturalRole> AcceptedRoles;

	/** Maximum encounters to spawn per scan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter", meta = (ClampMin = "1", ClampMax = "16"))
	int32 MaxEncounters = 3;

	/** Minimum distance (cm) between spawned encounters. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter", meta = (ClampMin = "100"))
	float MinSpacingCm = 800.0f;

	/** Radius (cm) to search for PCG components. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter", meta = (ClampMin = "500"))
	float SearchRadiusCm = 5000.0f;

	/** If true, use the walkable index for fast PCG attribute queries (preferred). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter")
	bool bUseWalkableIndex = true;

	/** Optional reference to a walkable index actor. Auto-discovered if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Encounter")
	TObjectPtr<AMelodiaPCGWalkableIndex> WalkableIndex;

	/** Triggers spawned by the last ScanAndSpawn call. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Encounter")
	TArray<TObjectPtr<AActor>> SpawnedEncounters;

	/** Scan nearby PCG data and spawn encounter triggers at matching roles. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Encounter")
	int32 ScanAndSpawn();

	/** Remove all previously spawned encounters. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Encounter")
	void ClearSpawnedEncounters();

protected:
	virtual void BeginPlay() override;

private:
	/** Collect candidate positions from PCG components near this actor. */
	TArray<FVector> CollectRolePositions() const;
};
