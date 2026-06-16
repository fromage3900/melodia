// Copyright Melodia Project. All Rights Reserved.
// Shared base implementation for PCG-driven spawners.

#include "MelodiaPCGSpawnerBase.h"

#include "MelodiaPCGWalkableIndex.h"
#include "MelodiaPCGLibrary.h"
#include "PCGComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMelodiaPCGSpawnerBase::AMelodiaPCGSpawnerBase()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMelodiaPCGSpawnerBase::BeginPlay()
{
	Super::BeginPlay();
}

bool AMelodiaPCGSpawnerBase::SpawnActorAt(
	const FVector& Position,
	EPCGArchitecturalRole ArchRole,
	AActor*& OutActor)
{
	OutActor = nullptr;
	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
int32 AMelodiaPCGSpawnerBase::ScanAndSpawn()
{
	ClearSpawnedActors();

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	// 1. Collect candidate positions from PCG data.
	const TArray<TPair<FVector, EPCGArchitecturalRole>> Candidates = CollectRolePositions();
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("%s: No candidate positions found within %.0f cm."),
			*GetName(), static_cast<double>(SearchRadiusCm));
		return 0;
	}

	// 2. De-duplicate using MinSpacingCm.
	TArray<TPair<FVector, EPCGArchitecturalRole>> Filtered;
	Filtered.Reserve(MaxSpawnCount);
	const float MinSpacingSq = MinSpacingCm * MinSpacingCm;

	for (const TPair<FVector, EPCGArchitecturalRole>& Candidate : Candidates)
	{
		bool bTooClose = false;
		for (const TPair<FVector, EPCGArchitecturalRole>& Existing : Filtered)
		{
			if (FVector::DistSquared(Candidate.Key, Existing.Key) < MinSpacingSq)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		Filtered.Add(Candidate);
		if (Filtered.Num() >= MaxSpawnCount)
		{
			break;
		}
	}

	// 3. Spawn actors via subclass hook.
	int32 SpawnCount = 0;
	for (const TPair<FVector, EPCGArchitecturalRole>& Entry : Filtered)
	{
		AActor* NewActor = nullptr;
		if (SpawnActorAt(Entry.Key, Entry.Value, NewActor) && NewActor)
		{
			SpawnedActors.Add(NewActor);
			++SpawnCount;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("%s: Spawned %d / %d candidates."),
		*GetName(), SpawnCount, Candidates.Num());

	return SpawnCount;
}

// ─────────────────────────────────────────────────────────────────────────────
TArray<AActor*> AMelodiaPCGSpawnerBase::GetSpawnedActors() const
{
	TArray<AActor*> Result;
	Result.Reserve(SpawnedActors.Num());
	for (const TObjectPtr<AActor>& Actor : SpawnedActors)
	{
		if (Actor)
		{
			Result.Add(Actor.Get());
		}
	}
	return Result;
}

void AMelodiaPCGSpawnerBase::ClearSpawnedActors()
{
	for (AActor* Child : SpawnedActors)
	{
		if (Child && !Child->IsPendingKillPending())
		{
			Child->Destroy();
		}
	}
	SpawnedActors.Empty();
}

// ─────────────────────────────────────────────────────────────────────────────
TArray<TPair<FVector, EPCGArchitecturalRole>> AMelodiaPCGSpawnerBase::CollectRolePositions() const
{
	TArray<TPair<FVector, EPCGArchitecturalRole>> Result;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}

	const FVector Origin = GetActorLocation();
	const float SearchRadiusSq = SearchRadiusCm * SearchRadiusCm;

	// Build set of accepted role values.
	TSet<int32> AcceptedRoleValues;
	for (const EPCGArchitecturalRole AcceptedRole : AcceptedRoles)
	{
		AcceptedRoleValues.Add(static_cast<int32>(AcceptedRole));
	}

	// Fast path: use the walkable index if available and enabled.
	if (bUseWalkableIndex)
	{
		AMelodiaPCGWalkableIndex* Index = WalkableIndex;
		if (!Index)
		{
			Index = FindWalkableIndex();
		}

		if (Index && Index->GetCachedPointCount() > 0)
		{
			const TArray<FVector> Positions = Index->GetCachedPositions();
			const TArray<EPCGArchitecturalRole> Roles = Index->GetCachedRoles();

			for (int32 i = 0; i < Positions.Num(); ++i)
			{
				const FVector& Pos = Positions[i];
				if (FVector::DistSquared(Pos, Origin) > SearchRadiusSq)
				{
					continue;
				}

				const int32 RoleValue = static_cast<int32>(Roles[i]);
				if (AcceptedRoleValues.Contains(RoleValue))
				{
					Result.Add(TPair<FVector, EPCGArchitecturalRole>(Pos, Roles[i]));
				}
			}

			UE_LOG(LogTemp, Verbose,
				TEXT("%s: Collected %d positions from walkable index."),
				*GetName(), Result.Num());
			return Result;
		}
	}

	// Fallback: use UMelodiaPCGLibrary to query PCG components directly.
	const TArray<UPCGComponent*> Comps =
		UMelodiaPCGLibrary::CollectPCGComponents(World, Origin, SearchRadiusCm);

	for (const UPCGComponent* Comp : Comps)
	{
		const TArray<FMelodiaWalkablePoint> AllPts = UMelodiaPCGLibrary::GetAllPoints(Comp);
		for (const FMelodiaWalkablePoint& WP : AllPts)
		{
			if (FVector::DistSquared(WP.Location, Origin) > SearchRadiusSq)
			{
				continue;
			}

			if (AcceptedRoleValues.Contains(static_cast<int32>(WP.Role)))
			{
				Result.Add(TPair<FVector, EPCGArchitecturalRole>(WP.Location, WP.Role));
			}
		}
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("%s: Collected %d positions from PCG library fallback."),
		*GetName(), Result.Num());

	return Result;
}

// ─────────────────────────────────────────────────────────────────────────────
AMelodiaPCGWalkableIndex* AMelodiaPCGSpawnerBase::FindWalkableIndex() const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return nullptr;
	}

	return UMelodiaPCGLibrary::FindWalkableIndex(World);
}
