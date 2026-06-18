// Copyright Melodia Project. All Rights Reserved.
// Shared Blueprint function library for querying PCG-generated data at runtime.

#include "MelodiaPCGLibrary.h"
#include "MelodiaPCGGraphRegistry.h"

#include "MelodiaPCGWalkableIndex.h"
#include "../MelodiaPCGEncounterSpawner.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "PCGSubsystem.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

namespace MelodiaPCGLibraryPrivate
{
	bool PumpEditorUntilPCGComplete(UPCGComponent* PCGComponent, float MaxWaitSeconds)
	{
		if (!PCGComponent)
		{
			return false;
		}

		const double Deadline = FPlatformTime::Seconds() + static_cast<double>(MaxWaitSeconds);
		while (FPlatformTime::Seconds() < Deadline)
		{
			if (!PCGComponent->IsGenerating())
			{
				return true;
			}

			// NOTE: Do NOT call GEditor->Tick() here. This pump is reached from contexts that are
			// already inside an engine/editor frame — AActor::OnConstruction during FEditorFileUtils::LoadMap,
			// the editor-startup content bootstrap ticker, etc. Re-entering GEditor->Tick() emits a nested
			// EDynamicResolutionStateEvent::BeginFrame and trips the fatal "Begin dynamic resolution event
			// should be fired exactly once" assert (UnrealEngine.cpp), which crashed the editor on every
			// startup. Ticking only the PCG subsystem advances generation to completion (worker-thread tasks
			// progress regardless) without re-entering the frame loop.
			if (UWorld* World = PCGComponent->GetWorld())
			{
				if (UPCGSubsystem* PCGSubsystem = UPCGSubsystem::GetInstance(World))
				{
					PCGSubsystem->Tick(0.016f);
				}
			}

			FPlatformProcess::Sleep(0.005f);
		}

		return !PCGComponent->IsGenerating();
	}
}

// ---------------------------------------------------------------------------
// ExtractPointsFromData — internal helper (shared by GetWalkablePoints/GetAllPoints)
// ---------------------------------------------------------------------------

void UMelodiaPCGLibrary::ExtractPointsFromData(
	const UPCGPointData* Data,
	const FTransform& WorldTransform,
	float MaxSlopeAngle,
	bool bFilterWalkable,
	TArray<FMelodiaWalkablePoint>& OutPoints)
{
	if (!Data)
	{
		return;
	}

	const UPCGMetadata* Metadata = Data->Metadata;
	if (!Metadata)
	{
		return;
	}

	// Look up attributes by canonical name (read-only path).
	const FPCGMetadataAttribute<int32>* RoleAttr =
		Metadata->GetConstTypedAttribute<int32>(FMelodiaPCGAttrs::ArchitecturalRoleAttr);
	const FPCGMetadataAttribute<bool>* WalkAttr =
		Metadata->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
	const FPCGMetadataAttribute<float>* SlopeAttr =
		Metadata->GetConstTypedAttribute<float>(FMelodiaPCGAttrs::SlopeAngleAttr);

	const TArray<FPCGPoint>& Points = Data->GetPoints();
	OutPoints.Reserve(OutPoints.Num() + Points.Num());

	for (const FPCGPoint& Pt : Points)
	{
		if (bFilterWalkable)
		{
			// Fast path: check Walkable attribute first.
			if (WalkAttr)
			{
				const bool bWalk = WalkAttr->GetValueFromItemKey(Pt.MetadataEntry);
				if (!bWalk)
				{
					continue;
				}
			}

			// Check slope angle if available.
			if (SlopeAttr)
			{
				const float SlopeCheck = SlopeAttr->GetValueFromItemKey(Pt.MetadataEntry);
				if (SlopeCheck > MaxSlopeAngle)
				{
					continue;
				}
			}
		}

		FMelodiaWalkablePoint WP;
		WP.Location = WorldTransform.TransformPosition(Pt.Transform.GetLocation());
		WP.Role = RoleAttr
			? static_cast<EPCGArchitecturalRole>(RoleAttr->GetValueFromItemKey(Pt.MetadataEntry))
			: EPCGArchitecturalRole::None;

		float Slope = 0.0f;
		if (SlopeAttr)
		{
			Slope = SlopeAttr->GetValueFromItemKey(Pt.MetadataEntry);
		}
		WP.SlopeAngle = Slope;

		// Approximate surface normal from slope angle.
		const float SlopeRad = FMath::DegreesToRadians(Slope);
		WP.Normal = FVector(FMath::Sin(SlopeRad), 0.0f, FMath::Cos(SlopeRad)).GetSafeNormal();

		OutPoints.Add(WP);
	}
}

// ---------------------------------------------------------------------------
// GetWalkablePoints
// ---------------------------------------------------------------------------

TArray<FMelodiaWalkablePoint> UMelodiaPCGLibrary::GetWalkablePoints(
	const UPCGComponent* PCGComp, float MaxSlopeAngle)
{
	TArray<FMelodiaWalkablePoint> Result;
	if (!PCGComp)
	{
		return Result;
	}

	AActor* PCGOwner = PCGComp->GetOwner();
	const FTransform WorldXform = PCGOwner ? PCGOwner->GetActorTransform() : FTransform::Identity;

	// UE5.7: access generated output via GetGeneratedGraphOutput() (project standard).
	const FPCGDataCollection& Output = PCGComp->GetGeneratedGraphOutput();
	for (const FPCGTaggedData& Tagged : Output.TaggedData)
	{
		const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged.Data);
		if (PointData)
		{
			ExtractPointsFromData(PointData, WorldXform, MaxSlopeAngle, /*bFilterWalkable=*/true, Result);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// GetAllPoints — returns ALL points (no walkable filter). For debug/visualization.
// ---------------------------------------------------------------------------

TArray<FMelodiaWalkablePoint> UMelodiaPCGLibrary::GetAllPoints(
	const UPCGComponent* PCGComp)
{
	TArray<FMelodiaWalkablePoint> Result;
	if (!PCGComp)
	{
		return Result;
	}

	AActor* PCGOwner = PCGComp->GetOwner();
	const FTransform WorldXform = PCGOwner ? PCGOwner->GetActorTransform() : FTransform::Identity;

	const FPCGDataCollection& Output = PCGComp->GetGeneratedGraphOutput();
	for (const FPCGTaggedData& Tagged : Output.TaggedData)
	{
		const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged.Data);
		if (PointData)
		{
			ExtractPointsFromData(PointData, WorldXform, 0.0f, /*bFilterWalkable=*/false, Result);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// FindNearestWalkablePoint
// ---------------------------------------------------------------------------

bool UMelodiaPCGLibrary::FindNearestWalkablePoint(
	const UPCGComponent* PCGComp,
	const FVector& Origin,
	float SearchRadius,
	FVector& OutLocation)
{
	if (!PCGComp)
	{
		return false;
	}

	const TArray<FMelodiaWalkablePoint> Walkable = GetWalkablePoints(PCGComp);
	const float RadiusSq = SearchRadius * SearchRadius;
	float BestDistSq = RadiusSq;
	bool bFound = false;

	for (const FMelodiaWalkablePoint& WP : Walkable)
	{
		const float DistSq = FVector::DistSquared(WP.Location, Origin);
		if (DistSq < BestDistSq)
		{
			BestDistSq = DistSq;
			OutLocation = WP.Location;
			bFound = true;
		}
	}

	return bFound;
}

// ---------------------------------------------------------------------------
// FilterByRole
// ---------------------------------------------------------------------------

TArray<FMelodiaWalkablePoint> UMelodiaPCGLibrary::FilterByRole(
	const TArray<FMelodiaWalkablePoint>& Points,
	EPCGArchitecturalRole Role)
{
	TArray<FMelodiaWalkablePoint> Result;
	for (const FMelodiaWalkablePoint& WP : Points)
	{
		if (WP.Role == Role)
		{
			Result.Add(WP);
		}
	}
	return Result;
}

// ---------------------------------------------------------------------------
// CollectPCGComponents
// ---------------------------------------------------------------------------

TArray<UPCGComponent*> UMelodiaPCGLibrary::CollectPCGComponents(
	const UObject* WorldContextObject,
	const FVector& Origin,
	float Radius)
{
	TArray<UPCGComponent*> Result;
	if (!WorldContextObject)
	{
		return Result;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return Result;
	}

	const float RadiusSq = Radius * Radius;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		if (FVector::DistSquared(It->GetActorLocation(), Origin) <= RadiusSq)
		{
			Result.Add(PCGComp);
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// GetTotalWalkablePointCount
// ---------------------------------------------------------------------------

int32 UMelodiaPCGLibrary::GetTotalWalkablePointCount(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return 0;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return 0;
	}

	int32 Total = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		Total += GetWalkablePoints(PCGComp).Num();
	}

	return Total;
}

// ---------------------------------------------------------------------------
// IsLocationNearWalkablePoint
// ---------------------------------------------------------------------------

bool UMelodiaPCGLibrary::IsLocationNearWalkablePoint(
	const UObject* WorldContextObject,
	const FVector& Location,
	float Tolerance)
{
	if (!WorldContextObject)
	{
		return false;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return false;
	}

	const float TolSq = Tolerance * Tolerance;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		const TArray<FMelodiaWalkablePoint> Walkable = GetWalkablePoints(PCGComp);
		for (const FMelodiaWalkablePoint& WP : Walkable)
		{
			if (FVector::DistSquared(WP.Location, Location) <= TolSq)
			{
				return true;
			}
		}
	}

	return false;
}

// ---------------------------------------------------------------------------
// GetWalkablePointsByRoleInRadius
// ---------------------------------------------------------------------------

TArray<FMelodiaWalkablePoint> UMelodiaPCGLibrary::GetWalkablePointsByRoleInRadius(
	const UObject* WorldContextObject,
	const FVector& Center,
	float Radius,
	EPCGArchitecturalRole Role)
{
	TArray<FMelodiaWalkablePoint> Result;
	if (!WorldContextObject)
	{
		return Result;
	}

	const TArray<UPCGComponent*> Components = CollectPCGComponents(WorldContextObject, Center, Radius);
	const float RadiusSq = Radius * Radius;

	for (const UPCGComponent* Comp : Components)
	{
		const TArray<FMelodiaWalkablePoint> Walkable = GetWalkablePoints(Comp);
		for (const FMelodiaWalkablePoint& WP : Walkable)
		{
			if (WP.Role == Role && FVector::DistSquared(WP.Location, Center) <= RadiusSq)
			{
				Result.Add(WP);
			}
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// FindWalkableIndex / FindEncounterSpawner — world-level actor discovery
// ---------------------------------------------------------------------------

AMelodiaPCGWalkableIndex* UMelodiaPCGLibrary::FindWalkableIndex(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AMelodiaPCGWalkableIndex> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

AMelodiaPCGEncounterSpawner* UMelodiaPCGLibrary::FindEncounterSpawner(const UObject* WorldContextObject)
{
	if (!WorldContextObject)
	{
		return nullptr;
	}

	UWorld* World = WorldContextObject->GetWorld();
	if (!World)
	{
		return nullptr;
	}

	for (TActorIterator<AMelodiaPCGEncounterSpawner> It(World); It; ++It)
	{
		return *It;
	}
	return nullptr;
}

UPCGGraph* UMelodiaPCGLibrary::LoadPCGGraph(const FSoftObjectPath& GraphAssetPath)
{
	if (GraphAssetPath.IsNull())
	{
		return nullptr;
	}
	return Cast<UPCGGraph>(GraphAssetPath.TryLoad());
}

bool UMelodiaPCGLibrary::AssignGraphToComponent(UPCGComponent* PCGComponent, const FSoftObjectPath& GraphAssetPath, int32 Seed)
{
	if (!PCGComponent)
	{
		return false;
	}

	UPCGGraph* Graph = LoadPCGGraph(GraphAssetPath);
	if (!Graph)
	{
		UE_LOG(LogTemp, Warning, TEXT("MelodiaPCG: failed to load graph %s"), *GraphAssetPath.ToString());
		return false;
	}

	if (!Graph->IsStandaloneGraph())
	{
		Graph->SetFlags(RF_Transactional);
		Graph->bIsStandaloneGraph = true;
		Graph->MarkPackageDirty();
	}

	PCGComponent->SetGraph(Graph);
	PCGComponent->Seed = Seed;
	return true;
}

bool UMelodiaPCGLibrary::GeneratePCGComponent(UPCGComponent* PCGComponent, bool bForce, float MaxWaitSeconds)
{
	if (!PCGComponent)
	{
		return false;
	}

	PCGComponent->bActivated = true;
	PCGComponent->CleanupLocalImmediate(/*bRemoveComponents=*/true);
	PCGComponent->Generate(bForce);
	MelodiaPCGLibraryPrivate::PumpEditorUntilPCGComplete(PCGComponent, MaxWaitSeconds);
	return true;
}

int32 UMelodiaPCGLibrary::CountInstancedMeshInstances(const AActor* Actor)
{
	if (!Actor)
	{
		return 0;
	}

	int32 Total = 0;
	TArray<UInstancedStaticMeshComponent*> ISMComponents;
	Actor->GetComponents<UInstancedStaticMeshComponent>(ISMComponents);
	for (const UInstancedStaticMeshComponent* ISM : ISMComponents)
	{
		if (ISM)
		{
			Total += ISM->GetInstanceCount();
		}
	}
	return Total;
}
