// Copyright Melodia Project. All Rights Reserved.
// Places decorative static meshes at PCG-generated points based on architectural
// role (Railing, Ornament, Column, Cornice).

#include "MelodiaPCGDecorationSpawner.h"

#include "MelodiaPCGWalkableIndex.h"
#include "MelodiaPCGLibrary.h"
#include "PCGComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"

AMelodiaPCGDecorationSpawner::AMelodiaPCGDecorationSpawner()
{
	PrimaryActorTick.bCanEverTick = false;

	// Default decorative roles.
	AcceptedRoles.Add(EPCGArchitecturalRole::Railing);
	AcceptedRoles.Add(EPCGArchitecturalRole::Ornament);
	AcceptedRoles.Add(EPCGArchitecturalRole::Column);
	AcceptedRoles.Add(EPCGArchitecturalRole::Cornice);
	AcceptedRoles.Add(EPCGArchitecturalRole::Roof);
}

void AMelodiaPCGDecorationSpawner::BeginPlay()
{
	Super::BeginPlay();
}

int32 AMelodiaPCGDecorationSpawner::ScanAndSpawn()
{
	ClearSpawnedDecorations();

	UWorld* World = GetWorld();
	if (!World)
	{
		return 0;
	}

	const TArray<TPair<FVector, EPCGArchitecturalRole>> Candidates = CollectRolePositions();
	if (Candidates.Num() == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PCGDecorationSpawner: No candidate positions found within %.0f cm."),
			SearchRadiusCm);
		return 0;
	}

	// De-duplicate using MinSpacingCm.
	TArray<TPair<FVector, EPCGArchitecturalRole>> Filtered;
	Filtered.Reserve(MaxDecorations);
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
		if (Filtered.Num() >= MaxDecorations)
		{
			break;
		}
	}

	// Collect cached normals from the WalkableIndex for orientation lookups.
	// This avoids re-querying all PCG components just for normals.
	TArray<FVector> CachedNormals;
	TArray<FVector> CachedPositions;
	{
		AMelodiaPCGWalkableIndex* Index = WalkableIndex;
		if (!Index)
		{
			for (TActorIterator<AMelodiaPCGWalkableIndex> It(World); It; ++It)
			{
				Index = *It;
				break;
			}
		}
		if (Index && Index->GetCachedPointCount() > 0)
		{
			CachedPositions = Index->GetCachedPositions();
			CachedNormals = Index->GetCachedNormals();
		}
	}

	// Spawn decoration actors at filtered positions.
	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	int32 SpawnCount = 0;
	for (const TPair<FVector, EPCGArchitecturalRole>& Entry : Filtered)
	{
		const FVector& Pos = Entry.Key;
		const EPCGArchitecturalRole ArchRole = Entry.Value;

		const FMelodiaDecorationMapping* Mapping = RoleToMesh.Find(ArchRole);
		if (!Mapping || !Mapping->Mesh)
		{
			continue;
		}

		// Compute orientation: align up to surface normal if available, else keep world up.
		FVector SurfaceNormal = FVector::UpVector;
		float BestDistSq = FLT_MAX;
		for (int32 ni = 0; ni < CachedPositions.Num(); ++ni)
		{
			const float DistSq = FVector::DistSquared(CachedPositions[ni], Pos);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				if (CachedNormals.IsValidIndex(ni))
				{
					SurfaceNormal = CachedNormals[ni];
				}
			}
		}

		// Build rotation: Z axis = surface normal, X/Y derived from normal.
		FRotator SpawnRot = FRotator::ZeroRotator;
		if (!SurfaceNormal.Equals(FVector::UpVector, KINDA_SMALL_NUMBER))
		{
			const FVector Forward = FVector::CrossProduct(SurfaceNormal, FVector::UpVector).GetSafeNormal();
			if (!Forward.IsZero())
			{
				SpawnRot = FRotationMatrix::MakeFromZY(SurfaceNormal, Forward).Rotator();
			}
		}

		// Apply random yaw if configured.
		if (Mapping->bRandomYaw)
		{
			SpawnRot.Yaw = FMath::FRandRange(0.0f, 360.0f);
		}

		AActor* Decoration = World->SpawnActor<AActor>(
			AActor::StaticClass(),
			Pos,
			SpawnRot,
			SpawnParams);

		if (Decoration)
		{
			// Attach a static mesh component.
			UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(
				Decoration, TEXT("DecorationMesh"));
			MeshComp->SetStaticMesh(Mapping->Mesh);
			MeshComp->SetWorldScale3D(FVector(Mapping->Scale));
			MeshComp->SetMobility(EComponentMobility::Static);
			MeshComp->RegisterComponent();
			Decoration->SetRootComponent(MeshComp);

			SpawnedDecorations.Add(Decoration);
			++SpawnCount;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("PCGDecorationSpawner: Spawned %d / %d candidates."),
		SpawnCount, Candidates.Num());

	return SpawnCount;
}

void AMelodiaPCGDecorationSpawner::ClearSpawnedDecorations()
{
	for (AActor* Deco : SpawnedDecorations)
	{
		if (Deco && !Deco->IsPendingKillPending())
		{
			Deco->Destroy();
		}
	}
	SpawnedDecorations.Empty();
}

TArray<TPair<FVector, EPCGArchitecturalRole>> AMelodiaPCGDecorationSpawner::CollectRolePositions() const
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
					Result.Add(TPair<FVector, EPCGArchitecturalRole>(Pos, Roles[i]));
				}
			}

			UE_LOG(LogTemp, Verbose,
				TEXT("PCGDecorationSpawner: Collected %d positions from walkable index."),
				Result.Num());
			return Result;
		}
	}

	// Fallback: use UMelodiaPCGLibrary to query all PCG components.
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
		TEXT("PCGDecorationSpawner: Collected %d positions from PCG library fallback."),
		Result.Num());

	return Result;
}
