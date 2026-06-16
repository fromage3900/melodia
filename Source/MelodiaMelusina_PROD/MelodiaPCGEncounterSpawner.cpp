// Copyright Melodia Project. All Rights Reserved.
// Reads PCG ArchitecturalRole attributes from generated points and spawns
// encounter triggers at semantically appropriate locations.

#include "MelodiaPCGEncounterSpawner.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "MelodiaEncounterTrigger.h"
#include "MelodiaPCGWalkableIndex.h"
#include "PCG/MelodiaPCGLibrary.h"
#include "CollisionQueryParams.h"

AMelodiaPCGEncounterSpawner::AMelodiaPCGEncounterSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default: accept stair and tile roles as encounter sites.
	AcceptedRoles.Add(EPCGArchitecturalRole::Stair);
	AcceptedRoles.Add(EPCGArchitecturalRole::Tile);
}

void AMelodiaPCGEncounterSpawner::BeginPlay()
{
	Super::BeginPlay();
}

int32 AMelodiaPCGEncounterSpawner::ScanAndSpawn()
{
	ClearSpawnedEncounters();

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const TArray<FVector> Candidates = CollectRolePositions();
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PCGEncounterSpawner: No candidate positions found within %.0f cm."),
			SearchRadiusCm);
		return 0;
	}

	// De-duplicate candidates using MinSpacingCm.
	TArray<FVector> Filtered;
	Filtered.Reserve(MaxEncounters);
	const float MinSpacingSq = MinSpacingCm * MinSpacingCm;

	for (const FVector& Pos : Candidates)
	{
		bool bTooClose = false;
		for (const FVector& Existing : Filtered)
		{
			if (FVector::DistSquared(Pos, Existing) < MinSpacingSq)
			{
				bTooClose = true;
				break;
			}
		}
		if (bTooClose)
		{
			continue;
		}

		Filtered.Add(Pos);
		if (Filtered.Num() >= MaxEncounters)
		{
			break;
		}
	}

	// Spawn encounter triggers at filtered positions.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	for (const FVector& Pos : Filtered)
	{
		AActor* Trigger = World->SpawnActor<AActor>(
			AMelodiaEncounterTrigger::StaticClass(),
			Pos + FVector(0.0f, 0.0f, 5.0f),
			FRotator::ZeroRotator,
			SpawnParams);

		if (Trigger)
		{
			SpawnedEncounters.Add(Trigger);
			UE_LOG(LogTemp, Log,
				TEXT("PCGEncounterSpawner: Spawned encounter at %s."),
				*Pos.ToString());
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("PCGEncounterSpawner: Spawned %d / %d candidates."),
		SpawnedEncounters.Num(), Candidates.Num());

	return SpawnedEncounters.Num();
}

void AMelodiaPCGEncounterSpawner::ClearSpawnedEncounters()
{
	for (AActor* Encounter : SpawnedEncounters)
	{
		if (Encounter && !Encounter->IsPendingKillPending())
		{
			Encounter->Destroy();
		}
	}
	SpawnedEncounters.Empty();
}

TArray<FVector> AMelodiaPCGEncounterSpawner::CollectRolePositions() const
{
	TArray<FVector> Result;

	UWorld* World = GetWorld();
	if (!World)
	{
		return Result;
	}

	const FVector Origin = GetActorLocation();
	const float SearchRadiusSq = SearchRadiusCm * SearchRadiusCm;

	// Build a set of accepted EPCGArchitecturalRole values for fast comparison.
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
			// Auto-discover the walkable index in the world.
			for (TActorIterator<AMelodiaPCGWalkableIndex> It(World); It; ++It)
			{
				Index = *It;
				break;
			}
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
					Result.Add(Pos);
				}
			}

			UE_LOG(LogTemp, Verbose,
				TEXT("PCGEncounterSpawner: Collected %d positions from walkable index."),
				Result.Num());
			return Result;
		}
	}

	// Middle-ground: use PCGLibrary to query PCG data directly (no WalkableIndex needed).
	const TArray<UPCGComponent*> Comps =
		UMelodiaPCGLibrary::CollectPCGComponents(World, Origin, SearchRadiusCm);

	for (const UPCGComponent* Comp : Comps)
	{
		const TArray<FMelodiaWalkablePoint> Walkable = UMelodiaPCGLibrary::GetWalkablePoints(Comp);
		for (const FMelodiaWalkablePoint& WP : Walkable)
		{
			if (FVector::DistSquared(WP.Location, Origin) > SearchRadiusSq)
			{
				continue;
			}
			if (AcceptedRoleValues.Contains(static_cast<int32>(WP.Role)))
			{
				Result.Add(WP.Location);
			}
		}
	}

	if (Result.Num() > 0)
	{
		UE_LOG(LogTemp, Verbose,
			TEXT("PCGEncounterSpawner: Collected %d positions from PCGLibrary fallback."),
			Result.Num());
		return Result;
	}

	// Last resort: grid-sampling raycast approach when PCG data is not accessible.
	// NOTE: This fallback cannot filter by AcceptedRoles because raycasts don't
	// carry PCG attribute data. All walkable surfaces are accepted. For proper
	// role-based filtering, ensure an AMelodiaPCGWalkableIndex exists in the level.
	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(PCGEncounterCollect), false);
	constexpr float GridStep = 400.0f;   // 4 m grid
	constexpr int32 GridExtent = 5;      // ±5 cells = 11×11 grid
	constexpr float MaxWalkableAngle = 50.0f;

	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		const UPCGGraph* Graph = PCGComp->GetGraph();
		if (!Graph)
		{
			continue;
		}

		const FVector CompLoc = It->GetActorLocation();
		if (FVector::DistSquared(CompLoc, Origin) > SearchRadiusSq)
		{
			continue;
		}

		// Grid-sample around the PCG component to find walkable positions.
		for (int32 gx = -GridExtent; gx <= GridExtent; ++gx)
		{
			for (int32 gy = -GridExtent; gy <= GridExtent; ++gy)
			{
				const FVector SamplePos = CompLoc + FVector(
					static_cast<float>(gx) * GridStep,
					static_cast<float>(gy) * GridStep,
					0.0f);

				// Skip if outside search radius.
				if (FVector::DistSquared(SamplePos, Origin) > SearchRadiusSq)
				{
					continue;
				}

				// Downward raycast to find walkable surface.
				const FVector TraceStart = SamplePos + FVector(0.0f, 0.0f, 2000.0f);
				const FVector TraceEnd   = SamplePos - FVector(0.0f, 0.0f, 4000.0f);

				FHitResult Hit;
				if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
				{
					continue;
				}

				const float SurfaceAngle = FMath::RadiansToDegrees(
					FMath::Acos(FVector::DotProduct(Hit.ImpactNormal, FVector::UpVector)));

				if (SurfaceAngle <= MaxWalkableAngle)
				{
					Result.Add(Hit.ImpactPoint);
				}
			}
		}
	}

	UE_LOG(LogTemp, Verbose,
		TEXT("PCGEncounterSpawner: Collected %d candidate positions from PCG areas."),
		Result.Num());

	return Result;
}
