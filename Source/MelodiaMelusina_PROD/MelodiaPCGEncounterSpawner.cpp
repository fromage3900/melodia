// Copyright Melodia Project. All Rights Reserved.
// Reads PCG ArchitecturalRole attributes from generated points and spawns
// encounter triggers at semantically appropriate locations.

#include "MelodiaPCGEncounterSpawner.h"

#include "MelodiaEncounterTrigger.h"

AMelodiaPCGEncounterSpawner::AMelodiaPCGEncounterSpawner()
{
	// Encounter-specific defaults.
	MaxSpawnCount = 3;
	MinSpacingCm = 800.0f;

	// Default: accept stair and tile roles as encounter sites.
	AcceptedRoles.Add(EPCGArchitecturalRole::Stair);
	AcceptedRoles.Add(EPCGArchitecturalRole::Tile);
}

bool AMelodiaPCGEncounterSpawner::SpawnActorAt(
	const FVector& Position,
	EPCGArchitecturalRole ArchRole,
	AActor*& OutActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	OutActor = World->SpawnActor<AMelodiaEncounterTrigger>(
		AMelodiaEncounterTrigger::StaticClass(),
		Position + FVector(0.0f, 0.0f, 5.0f),
		FRotator::ZeroRotator,
		SpawnParams);

	if (OutActor)
	{
		UE_LOG(LogTemp, Log,
			TEXT("PCGEncounterSpawner: Spawned encounter at %s."),
			*Position.ToString());
		return true;
	}
	return false;
}
