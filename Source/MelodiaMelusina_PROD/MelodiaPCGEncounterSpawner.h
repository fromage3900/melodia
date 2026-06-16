// Copyright Melodia Project. All Rights Reserved.
// Reads PCG ArchitecturalRole attributes from generated points and spawns
// encounter triggers at semantically appropriate locations (e.g. Stair, Tile).

#pragma once

#include "CoreMinimal.h"
#include "MelodiaPCGSpawnerBase.h"
#include "MelodiaPCGEncounterSpawner.generated.h"

/**
 * Scans PCG-generated points for ArchitecturalRole attributes and spawns
 * AMelodiaEncounterTrigger actors at matching locations.  Designed to be
 * driven by AMelodiaReverieRunManager during procedural area generation.
 *
 * Inherits common spawner logic from AMelodiaPCGSpawnerBase; only implements
 * the encounter-specific SpawnActorAt() hook.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGEncounterSpawner : public AMelodiaPCGSpawnerBase
{
	GENERATED_BODY()

public:
	AMelodiaPCGEncounterSpawner();

protected:
	/** Create one AMelodiaEncounterTrigger at the given position. */
	virtual bool SpawnActorAt(const FVector& Position, EPCGArchitecturalRole ArchRole, AActor*& OutActor) override;
};
