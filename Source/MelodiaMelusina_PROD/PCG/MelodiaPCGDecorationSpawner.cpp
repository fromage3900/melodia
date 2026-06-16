// Copyright Melodia Project. All Rights Reserved.
// Places decorative static meshes at PCG-generated points based on architectural
// role (Railing, Ornament, Column, Cornice).

#include "MelodiaPCGDecorationSpawner.h"

#include "MelodiaPCGWalkableIndex.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"

AMelodiaPCGDecorationSpawner::AMelodiaPCGDecorationSpawner()
{
	// Decoration-specific defaults.
	MaxSpawnCount = 32;
	MinSpacingCm = 300.0f;

	// Default decorative roles.
	AcceptedRoles.Add(EPCGArchitecturalRole::Railing);
	AcceptedRoles.Add(EPCGArchitecturalRole::Ornament);
	AcceptedRoles.Add(EPCGArchitecturalRole::Column);
	AcceptedRoles.Add(EPCGArchitecturalRole::Cornice);
	AcceptedRoles.Add(EPCGArchitecturalRole::Roof);
}

bool AMelodiaPCGDecorationSpawner::SpawnActorAt(
	const FVector& Position,
	EPCGArchitecturalRole ArchRole,
	AActor*& OutActor)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const FMelodiaDecorationMapping* Mapping = RoleToMesh.Find(ArchRole);
	if (!Mapping || !Mapping->Mesh)
	{
		return false;
	}

	// Compute orientation: align up to surface normal if available.
	FVector SurfaceNormal = FVector::UpVector;
	AMelodiaPCGWalkableIndex* Index = WalkableIndex;
	if (!Index)
	{
		Index = FindWalkableIndex();
	}

	if (Index && Index->GetCachedPointCount() > 0)
	{
		const TArray<FVector> CachedPositions = Index->GetCachedPositions();
		const TArray<FVector> CachedNormals = Index->GetCachedNormals();

		float BestDistSq = FLT_MAX;
		for (int32 ni = 0; ni < CachedPositions.Num(); ++ni)
		{
			const float DistSq = FVector::DistSquared(CachedPositions[ni], Position);
			if (DistSq < BestDistSq)
			{
				BestDistSq = DistSq;
				if (CachedNormals.IsValidIndex(ni))
				{
					SurfaceNormal = CachedNormals[ni];
				}
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

	FActorSpawnParameters SpawnParams;
	SpawnParams.SpawnCollisionHandlingOverride =
		ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

	OutActor = World->SpawnActor<AActor>(
		AActor::StaticClass(),
		Position,
		SpawnRot,
		SpawnParams);

	if (OutActor)
	{
		// Attach a static mesh component.
		UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(
			OutActor, TEXT("DecorationMesh"));
		MeshComp->SetStaticMesh(Mapping->Mesh);
		MeshComp->SetWorldScale3D(FVector(Mapping->Scale));
		MeshComp->SetMobility(EComponentMobility::Static);
		MeshComp->RegisterComponent();
		OutActor->SetRootComponent(MeshComp);
		return true;
	}
	return false;
}
