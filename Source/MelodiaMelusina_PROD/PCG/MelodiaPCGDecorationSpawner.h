// Copyright Melodia Project. All Rights Reserved.
// Places decorative static meshes at PCG-generated points based on architectural
// role (Railing, Ornament, Column, Cornice).  Uses AMelodiaPCGWalkableIndex for
// fast position queries and FMelodiaWalkablePoint::Normal for orientation.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaPCGSpawnerBase.h"
#include "MelodiaPCGDecorationSpawner.generated.h"

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
 *
 * Inherits common spawner logic from AMelodiaPCGSpawnerBase; only implements
 * the decoration-specific SpawnActorAt() hook.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGDecorationSpawner : public AMelodiaPCGSpawnerBase
{
	GENERATED_BODY()

public:
	AMelodiaPCGDecorationSpawner();

	/** Per-role mesh mapping.  Roles not present here are skipped. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Decoration")
	TMap<EPCGArchitecturalRole, FMelodiaDecorationMapping> RoleToMesh;

protected:
	/** Create one decoration actor at the given position with normal-based orientation. */
	virtual bool SpawnActorAt(const FVector& Position, EPCGArchitecturalRole ArchRole, AActor*& OutActor) override;
};
