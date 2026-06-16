// Copyright Melodia Project. All Rights Reserved.
// Runtime walkable-point cache for PCG-generated data.

#include "MelodiaPCGWalkableIndex.h"
#include "MelodiaPCGLibrary.h"

#include "PCGComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMelodiaPCGWalkableIndex::AMelodiaPCGWalkableIndex()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMelodiaPCGWalkableIndex::BeginPlay()
{
	Super::BeginPlay();

	DiscoverPCGComponents();
	RebuildCache();

	// Schedule periodic rebuilds to catch late PCG generation.
	// PCG components may generate asynchronously after BeginPlay.
	GetWorldTimerManager().SetTimer(
		RebuildTimerHandle,
		this,
		&AMelodiaPCGWalkableIndex::RebuildCache,
		2.0f,   // check every 2 seconds
		true);  // loop
}

void AMelodiaPCGWalkableIndex::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(RebuildTimerHandle);
	Super::EndPlay(EndPlayReason);
}

// ---------------------------------------------------------------------------
// DiscoverPCGComponents
// ---------------------------------------------------------------------------

void AMelodiaPCGWalkableIndex::DiscoverPCGComponents()
{
	TrackedComponents.Empty();

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	const FVector Origin = GetActorLocation();
	const float RadiusSq = DiscoveryRadius * DiscoveryRadius;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		if (FVector::DistSquared(It->GetActorLocation(), Origin) <= RadiusSq)
		{
			TrackedComponents.Add(PCGComp);
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("PCGWalkableIndex: Discovered %d PCG components within %.0f cm."),
		TrackedComponents.Num(), DiscoveryRadius);
}

// ---------------------------------------------------------------------------
// RebuildCache
// ---------------------------------------------------------------------------

void AMelodiaPCGWalkableIndex::RebuildCache()
{
	CachedPositions.Empty();
	CachedRoles.Empty();
	CachedNormals.Empty();

	for (const TWeakObjectPtr<UPCGComponent>& WeakComp : TrackedComponents)
	{
		const UPCGComponent* Comp = WeakComp.Get();
		if (!Comp)
		{
			continue;
		}

		const TArray<FMelodiaWalkablePoint> Walkable =
			UMelodiaPCGLibrary::GetWalkablePoints(Comp, MaxSlopeAngle);

		for (const FMelodiaWalkablePoint& WP : Walkable)
		{
			CachedPositions.Add(WP.Location);
			CachedRoles.Add(WP.Role);
			CachedNormals.Add(WP.Normal);
		}
	}

	if (CachedPositions.Num() > 0)
	{
		UE_LOG(LogTemp, Log,
			TEXT("PCGWalkableIndex: Cached %d walkable points from %d components."),
			CachedPositions.Num(), TrackedComponents.Num());
	}
}

// ---------------------------------------------------------------------------
// FindNearestWalkable
// ---------------------------------------------------------------------------

bool AMelodiaPCGWalkableIndex::FindNearestWalkable(
	const FVector& Location,
	float MaxRadius,
	FVector& OutPosition,
	EPCGArchitecturalRole& OutRole,
	FVector& OutNormal) const
{
	if (CachedPositions.Num() == 0)
	{
		return false;
	}

	float BestDistSq = MaxRadius * MaxRadius;
	int32 BestIdx = -1;

	for (int32 i = 0; i < CachedPositions.Num(); ++i)
	{
		const float DistSq = FVector::DistSquared(CachedPositions[i], Location);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			BestIdx = i;
		}
	}

	if (BestIdx >= 0)
	{
		OutPosition = CachedPositions[BestIdx];
		OutRole = CachedRoles[BestIdx];
		if (CachedNormals.IsValidIndex(BestIdx))
		{
			OutNormal = CachedNormals[BestIdx];
		}
		else
		{
			OutNormal = FVector::UpVector;
		}
		return true;
	}

	return false;
}

// ---------------------------------------------------------------------------
// FindRandomWalkable
// ---------------------------------------------------------------------------

bool AMelodiaPCGWalkableIndex::FindRandomWalkable(
	const FVector& Location,
	float Radius,
	int32 Seed,
	FVector& OutPosition) const
{
	if (CachedPositions.Num() == 0)
	{
		return false;
	}

	// Collect candidates within radius.
	TArray<int32> Candidates;
	const float RadiusSq = Radius * Radius;

	for (int32 i = 0; i < CachedPositions.Num(); ++i)
	{
		if (FVector::DistSquared(CachedPositions[i], Location) <= RadiusSq)
		{
			Candidates.Add(i);
		}
	}

	if (Candidates.Num() == 0)
	{
		return false;
	}

	// Deterministic random selection.
	FRandomStream RNG(Seed);
	const int32 ChosenIdx = Candidates[RNG.RandRange(0, Candidates.Num() - 1)];
	OutPosition = CachedPositions[ChosenIdx];
	return true;
}

// ---------------------------------------------------------------------------
// Getters
// ---------------------------------------------------------------------------

int32 AMelodiaPCGWalkableIndex::GetCachedPointCount() const
{
	return CachedPositions.Num();
}

TArray<FVector> AMelodiaPCGWalkableIndex::GetCachedPositions() const
{
	return CachedPositions;
}

TArray<EPCGArchitecturalRole> AMelodiaPCGWalkableIndex::GetCachedRoles() const
{
	return CachedRoles;
}

TArray<FVector> AMelodiaPCGWalkableIndex::GetCachedNormals() const
{
	return CachedNormals;
}

void AMelodiaPCGWalkableIndex::ScheduleRebuild()
{
	// Debounce: rebuild on next timer tick.
	GetWorldTimerManager().SetTimer(
		RebuildTimerHandle,
		this,
		&AMelodiaPCGWalkableIndex::RebuildCache,
		0.1f,
		false);
}
