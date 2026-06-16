// Copyright Melodia Project. All Rights Reserved.
// Programmatic test harness for validating PCG element output.

#include "MelodiaPCGTestHarness.h"
#include "MelodiaPCGLibrary.h"

#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "Engine/World.h"
#include "EngineUtils.h"

AMelodiaPCGTestHarness::AMelodiaPCGTestHarness()
{
	PrimaryActorTick.bCanEverTick = false;
}

void AMelodiaPCGTestHarness::BeginPlay()
{
	Super::BeginPlay();

	if (bRunOnBeginPlay)
	{
		FTimerHandle TimerHandle;
		GetWorldTimerManager().SetTimer(
			TimerHandle,
			this,
			&AMelodiaPCGTestHarness::RunAllTestsDelayed,
			DelaySeconds,
			false);
	}
}

void AMelodiaPCGTestHarness::RunAllTestsDelayed()
{
	RunAllTests();
}

TArray<FPCGElementTestResult> AMelodiaPCGTestHarness::ResetAndRerun()
{
	ClearDebugMarkers();
	return RunAllTests();
}

// ---------------------------------------------------------------------------
// RunAllTests
// ---------------------------------------------------------------------------

TArray<FPCGElementTestResult> AMelodiaPCGTestHarness::RunAllTests()
{
	ClearDebugMarkers();

	TArray<FPCGElementTestResult> Results;

	UWorld* World = GetWorld();
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("PCGTestHarness: No world available."));
		return Results;
	}

	int32 ComponentCount = 0;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		UPCGComponent* PCGComp = It->FindComponentByClass<UPCGComponent>();
		if (!PCGComp)
		{
			continue;
		}

		++ComponentCount;
		FPCGElementTestResult Result = TestPCGComponent(PCGComp);
		Results.Add(Result);

		const FString Status = Result.bPassed ? TEXT("PASS") : TEXT("FAIL");
		UE_LOG(LogTemp, Log,
			TEXT("PCGTestHarness [%s]: %s — points=%d walkable=%d %s"),
			*Status,
			*Result.ElementName,
			Result.PointCount,
			Result.WalkableCount,
			*Result.FailureReason);
	}

	if (ComponentCount == 0)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("PCGTestHarness: No PCG components found in the world."));
	}

	// Summary.
	int32 PassCount = 0;
	for (const FPCGElementTestResult& R : Results)
	{
		if (R.bPassed) ++PassCount;
	}

	UE_LOG(LogTemp, Log,
		TEXT("PCGTestHarness: %d / %d tests passed."),
		PassCount, Results.Num());

	return Results;
}

// ---------------------------------------------------------------------------
// TestPCGComponent
// ---------------------------------------------------------------------------

FPCGElementTestResult AMelodiaPCGTestHarness::TestPCGComponent(UPCGComponent* PCGComp)
{
	FPCGElementTestResult Result;
	Result.bPassed = false;

	if (!PCGComp)
	{
		Result.ElementName = TEXT("null");
		Result.FailureReason = TEXT("null component");
		return Result;
	}

	// Determine element name from graph or settings class.
	const UPCGGraph* Graph = PCGComp->GetGraph();
	if (Graph)
	{
		Result.ElementName = Graph->GetName();
	}
	else
	{
		Result.ElementName = TEXT("NoGraph");
		Result.FailureReason = TEXT("no graph assigned");
		return Result;
	}

	// Try to read point data from the component via GetGeneratedGraphOutput() (project standard).
	// Aggregate across ALL tagged data blocks (multi-tier elements like RecursiveArch
	// emit per-tier on separate blocks).
	const FPCGDataCollection& Output = PCGComp->GetGeneratedGraphOutput();

	struct FBlockInfo { const UPCGPointData* Data; TArray<FPCGPoint> Points; UPCGMetadata* Meta; };
	TArray<FBlockInfo> Blocks;

	for (const FPCGTaggedData& Tagged : Output.TaggedData)
	{
		const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged.Data);
		if (!PointData) continue;

		FBlockInfo BI;
		BI.Data = PointData;
		BI.Points = PointData->GetPoints();
		BI.Meta = PointData->Metadata;
		Blocks.Add(BI);

		Result.PointCount += BI.Points.Num();
	}

	if (Blocks.Num() == 0)
	{
		Result.FailureReason = TEXT("no point data generated (component may not have run yet)");
		return Result;
	}

	// Count walkable points across all blocks.
	for (const FBlockInfo& BI : Blocks)
	{
		if (!BI.Meta) continue;
		const FPCGMetadataAttribute<bool>* WalkAttr =
			BI.Meta->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
		if (WalkAttr)
		{
			for (const FPCGPoint& Pt : BI.Points)
			{
				if (WalkAttr->GetValueFromItemKey(Pt.MetadataEntry))
				{
					++Result.WalkableCount;
				}
			}
		}
	}

	// Determine element type by name and run specific validation.
	// Validate each block independently; fail if any block fails.
	bool bValid = false;

	for (const FBlockInfo& BI : Blocks)
	{
		if (!BI.Meta)
		{
			Result.FailureReason = TEXT("no metadata on point data block");
			bValid = false;
			break;
		}

		if (Result.ElementName.Contains(TEXT("Escher")))
		{
			bValid = ValidateEscherStaircase(BI.Points, BI.Meta, Result.FailureReason);
		}
		else if (Result.ElementName.Contains(TEXT("Gravity")))
		{
			bValid = ValidateGravityZone(BI.Points, BI.Meta, Result.FailureReason);
		}
		else if (Result.ElementName.Contains(TEXT("RecursiveArch")) ||
				 Result.ElementName.Contains(TEXT("Arch")))
		{
			bValid = ValidateRecursiveArch(BI.Points, BI.Meta, Result.FailureReason);
		}
		else if (Result.ElementName.Contains(TEXT("Tessellation")))
		{
			bValid = ValidateTessellation(BI.Points, BI.Meta, Result.FailureReason);
		}
		else
		{
			// Unknown element — pass if it has points.
			bValid = (BI.Points.Num() > 0);
			if (!bValid)
			{
				Result.FailureReason = TEXT("unknown element with no points");
			}
		}

		if (!bValid) break;
	}

	// For unknown elements, also check aggregate walkable count.
	if (!Result.ElementName.Contains(TEXT("Escher")) &&
		!Result.ElementName.Contains(TEXT("Gravity")) &&
		!Result.ElementName.Contains(TEXT("RecursiveArch")) &&
		!Result.ElementName.Contains(TEXT("Arch")) &&
		!Result.ElementName.Contains(TEXT("Tessellation")))
	{
		bValid = (Result.PointCount > 0 && Result.WalkableCount > 0);
		if (!bValid)
		{
			Result.FailureReason = TEXT("unknown element with no walkable points");
		}
	}

	Result.bPassed = bValid;

	// Spawn debug markers if enabled — iterate all blocks.
	if (bSpawnDebugMarkers && bValid)
	{
		AActor* PCGOwner = PCGComp->GetOwner();
		const FTransform WorldXform = PCGOwner ? PCGOwner->GetActorTransform() : FTransform::Identity;
		for (const FBlockInfo& BI : Blocks)
		{
			if (!BI.Meta) continue;
			const FPCGMetadataAttribute<bool>* WalkAttr =
				BI.Meta->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
			for (const FPCGPoint& Pt : BI.Points)
			{
				if (WalkAttr && WalkAttr->GetValueFromItemKey(Pt.MetadataEntry))
				{
					const FVector Loc = WorldXform.TransformPosition(Pt.Transform.GetLocation());
					SpawnDebugMarker(Loc, FLinearColor::Green);
				}
			}
		}
	}

	return Result;
}

// ---------------------------------------------------------------------------
// Per-element validators
// ---------------------------------------------------------------------------

bool AMelodiaPCGTestHarness::ValidateEscherStaircase(
	const TArray<FPCGPoint>& Points,
	UPCGMetadata* Meta,
	FString& OutReason)
{
	if (Points.Num() < 3)
	{
		OutReason = TEXT("Escher: too few points (need >= 3)");
		return false;
	}

	// Check ArchitecturalRole = Stair on all points.
	const FPCGMetadataAttribute<int32>* RoleAttr =
		Meta->GetConstTypedAttribute<int32>(FMelodiaPCGAttrs::ArchitecturalRoleAttr);
	if (!RoleAttr)
	{
		OutReason = TEXT("Escher: missing ArchitecturalRole attribute");
		return false;
	}

	const int32 StairRole = static_cast<int32>(EPCGArchitecturalRole::Stair);
	for (const FPCGPoint& Pt : Points)
	{
		const int32 PointRole = RoleAttr->GetValueFromItemKey(Pt.MetadataEntry);
		if (PointRole != StairRole)
		{
			OutReason = FString::Printf(TEXT("Escher: point has PointRole=%d, expected Stair(%d)"), PointRole, StairRole);
			return false;
		}
	}

	// Check Walkable = true on all points.
	const FPCGMetadataAttribute<bool>* WalkAttr =
		Meta->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
	if (!WalkAttr)
	{
		OutReason = TEXT("Escher: missing Walkable attribute");
		return false;
	}

	for (const FPCGPoint& Pt : Points)
	{
		if (!WalkAttr->GetValueFromItemKey(Pt.MetadataEntry))
		{
			OutReason = TEXT("Escher: point has Walkable=false");
			return false;
		}
	}

	return true;
}

bool AMelodiaPCGTestHarness::ValidateGravityZone(
	const TArray<FPCGPoint>& Points,
	UPCGMetadata* Meta,
	FString& OutReason)
{
	if (Points.Num() == 0)
	{
		OutReason = TEXT("GravityZone: no points");
		return false;
	}

	// Check that Walkable attribute exists.
	const FPCGMetadataAttribute<bool>* WalkAttr =
		Meta->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
	if (!WalkAttr)
	{
		OutReason = TEXT("GravityZone: missing Walkable attribute");
		return false;
	}

	// Check that SlopeAngle attribute exists.
	const FPCGMetadataAttribute<float>* SlopeAttr =
		Meta->GetConstTypedAttribute<float>(FMelodiaPCGAttrs::SlopeAngleAttr);
	if (!SlopeAttr)
	{
		OutReason = TEXT("GravityZone: missing SlopeAngle attribute");
		return false;
	}

	return true;
}

bool AMelodiaPCGTestHarness::ValidateRecursiveArch(
	const TArray<FPCGPoint>& Points,
	UPCGMetadata* Meta,
	FString& OutReason)
{
	if (Points.Num() == 0)
	{
		OutReason = TEXT("RecursiveArch: no points");
		return false;
	}

	// Check ArchitecturalRole = Column on all points.
	const FPCGMetadataAttribute<int32>* RoleAttr =
		Meta->GetConstTypedAttribute<int32>(FMelodiaPCGAttrs::ArchitecturalRoleAttr);
	if (!RoleAttr)
	{
		OutReason = TEXT("RecursiveArch: missing ArchitecturalRole attribute");
		return false;
	}

	const int32 ColumnRole = static_cast<int32>(EPCGArchitecturalRole::Column);
	for (const FPCGPoint& Pt : Points)
	{
		const int32 PointRole = RoleAttr->GetValueFromItemKey(Pt.MetadataEntry);
		if (PointRole != ColumnRole)
		{
			OutReason = FString::Printf(TEXT("RecursiveArch: point has PointRole=%d, expected Column(%d)"), PointRole, ColumnRole);
			return false;
		}
	}

	return true;
}

bool AMelodiaPCGTestHarness::ValidateTessellation(
	const TArray<FPCGPoint>& Points,
	UPCGMetadata* Meta,
	FString& OutReason)
{
	if (Points.Num() == 0)
	{
		OutReason = TEXT("Tessellation: no points");
		return false;
	}

	// Check ArchitecturalRole = Tile on all points.
	const FPCGMetadataAttribute<int32>* RoleAttr =
		Meta->GetConstTypedAttribute<int32>(FMelodiaPCGAttrs::ArchitecturalRoleAttr);
	if (!RoleAttr)
	{
		OutReason = TEXT("Tessellation: missing ArchitecturalRole attribute");
		return false;
	}

	const int32 TileRole = static_cast<int32>(EPCGArchitecturalRole::Tile);
	for (const FPCGPoint& Pt : Points)
	{
		const int32 PointRole = RoleAttr->GetValueFromItemKey(Pt.MetadataEntry);
		if (PointRole != TileRole)
		{
			OutReason = FString::Printf(TEXT("Tessellation: point has PointRole=%d, expected Tile(%d)"), PointRole, TileRole);
			return false;
		}
	}

	// Check Walkable = true on all points.
	const FPCGMetadataAttribute<bool>* WalkAttr =
		Meta->GetConstTypedAttribute<bool>(FMelodiaPCGAttrs::WalkableAttr);
	if (!WalkAttr)
	{
		OutReason = TEXT("Tessellation: missing Walkable attribute");
		return false;
	}

	for (const FPCGPoint& Pt : Points)
	{
		if (!WalkAttr->GetValueFromItemKey(Pt.MetadataEntry))
		{
			OutReason = TEXT("Tessellation: point has Walkable=false");
			return false;
		}
	}

	return true;
}

// ---------------------------------------------------------------------------
// Debug markers
// ---------------------------------------------------------------------------

void AMelodiaPCGTestHarness::SpawnDebugMarker(const FVector& Location, FLinearColor Color)
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	// Use a simple sphere component actor for visualization.
	// In a real implementation, this would spawn a custom debug actor.
	// For now, we use DrawDebugSphere which is editor-only but sufficient for testing.
#if WITH_EDITOR
	DrawDebugSphere(
		World,
		Location,
		DebugMarkerRadius,
		12,
		Color.ToFColor(true),
		false,
		-1.0f,
		0,
		2.0f);
#endif
}

void AMelodiaPCGTestHarness::ClearDebugMarkers()
{
	DebugMarkers.Empty();
	// DrawDebugSphere is transient, so no persistent cleanup needed.
}
