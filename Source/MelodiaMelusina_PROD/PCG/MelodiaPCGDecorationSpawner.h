// Copyright Melodia Project. All Rights Reserved.
// Places decorative static meshes at PCG-generated points based on architectural
// role (Railing, Ornament, Column, Cornice).  Uses AMelodiaPCGWalkableIndex for
// fast position queries and FMelodiaWalkablePoint::Normal for orientation.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaPCGDecorationSpawner.generated.h"

class AMelodiaPCGWalkableIndex;

/** Maps an architectural role to a decorative mesh and optional scale range. */
USTRUCT(BlueprintType)
struct FMelodiaDecorationMapping
{
	GENERATED_BODY()

	/** Static mesh to spawn for this role. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	TObjectPtr<UStaticMesh> Mesh = nullptr;

	/** Uniform scale applied to spawned decorations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	float Scale = 1.0f;

	/** If true, randomise yaw rotation for visual variety. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	bool bRandomYaw = true;
};

/**
 * Scans PCG-generated points for decorative architectural roles and spawns
 * static-mesh actors at those locations.  Complements AMelodiaPCGEncounterSpawner
 * which handles gameplay encounters; this actor handles visual dressing.
 *
 * Supported decorative roles: Railing, Ornament, Column, Cornice, Roof.
 * Configure RoleToMesh to map each role to a mesh + scale.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGDecorationSpawner : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGDecorationSpawner();

	/** Which architectural roles receive decoration meshes. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	TArray<EPCGArchitecturalRole> AcceptedRoles;

	/** Per-role mesh mapping.  Roles not present here are skipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	TMap<EPCGArchitecturalRole, FMelodiaDecorationMapping> RoleToMesh;

	/** Maximum decorations to spawn per scan. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration", meta = (ClampMin = "1", ClampMax = "256"))
	int32 MaxDecorations = 32;

	/** Minimum distance (cm) between spawned decorations. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration", meta = (ClampMin = "50"))
	float MinSpacingCm = 300.0f;

	/** Radius (cm) to search for PCG walkable points. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration", meta = (ClampMin = "500"))
	float SearchRadiusCm = 5000.0f;

	/** If true, use the walkable index for fast PCG attribute queries (preferred). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	bool bUseWalkableIndex = true;

	/** Optional reference to a walkable index actor. Auto-discovered if null. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	TObjectPtr<AMelodiaPCGWalkableIndex> WalkableIndex;

	/** Decoration actors spawned by the last ScanAndSpawn call. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Decoration")
	TArray<TObjectPtr<AActor>> SpawnedDecorations;

	/** Scan nearby PCG data and spawn decoration meshes at matching roles. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Decoration")
	int32 ScanAndSpawn();

	/** Remove all previously spawned decoration actors. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Decoration")
	void ClearSpawnedDecorations();

protected:
	virtual void BeginPlay() override;

private:
	/** Collect candidate positions + roles from PCG data near this actor. */
	TArray<TPair<FVector, EPCGArchitecturalRole>> CollectRolePositions() const;
};
