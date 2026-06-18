// Copyright Melodia Project. All Rights Reserved.

#include "PCGMelodiaLandscapeScatterSettings.h"
#include "MelodiaPCGBezierHelpers.h"
#include "MelodiaPCGTerrain.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "PCGPin.h"

FPCGElementPtr UPCGMelodiaLandscapeScatterSettings::CreateElement() const
{
	return MakeShared<FPCGMelodiaLandscapeScatterElement>();
}

bool FPCGMelodiaLandscapeScatterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGMelodiaLandscapeScatterElement::ExecuteInternal);
	check(Context);

	const UPCGMelodiaLandscapeScatterSettings* Settings =
		Context->GetInputSettings<UPCGMelodiaLandscapeScatterSettings>();
	check(Settings);

	const TArray<FPCGTaggedData> Inputs = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);
	UWorld* World = MelodiaPCGTerrain::GetWorldFromPCGContext(Context);
	if (!World)
	{
		return true;
	}

	FBox Bounds(ForceInit);
	for (const FPCGTaggedData& Input : Inputs)
	{
		const UPCGPointData* InputPointData = Cast<UPCGPointData>(Input.Data);
		if (!InputPointData)
		{
			continue;
		}

		for (const FPCGPoint& Point : InputPointData->GetPoints())
		{
			Bounds += Point.Transform.GetLocation();
		}
	}

	if (!Bounds.IsValid)
	{
		return true;
	}

	Bounds = Bounds.ExpandBy(Settings->BoundsMargin);

	UPCGPointData* OutputPointData = FPCGContext::NewObject_AnyThread<UPCGPointData>(Context);
	TArray<FPCGPoint>& Points = OutputPointData->GetMutablePoints();
	Points.Reserve(Settings->TargetCount);

	FRandomStream RandomStream(Settings->ScatterSeed);
	int32 Placed = 0;
	int32 Attempts = 0;

	while (Placed < Settings->TargetCount && Attempts < Settings->MaxAttempts)
	{
		++Attempts;

		const float X = RandomStream.FRandRange(Bounds.Min.X, Bounds.Max.X);
		const float Y = RandomStream.FRandRange(Bounds.Min.Y, Bounds.Max.Y);
		FVector Position(X, Y, Bounds.Max.Z);
		FVector SurfaceNormal = FVector::UpVector;

		FMelodiaPCGTerrainProjection TraceOptions = Settings->TerrainProjection;
		TraceOptions.bPreserveDesignHeightOffset = false;
		if (!MelodiaPCGTerrain::ProjectWorldPosition(World, Position, SurfaceNormal, TraceOptions))
		{
			continue;
		}

		const float Slope = MelodiaPCGTerrain::ComputeSlopeDegrees(SurfaceNormal);
		if (Slope < Settings->MinSlopeDegrees || Slope > Settings->MaxSlopeDegrees)
		{
			continue;
		}

		const float Yaw = RandomStream.FRandRange(0.0f, 360.0f);
		FVector Forward = FVector::ForwardVector.RotateAngleAxis(Yaw, FVector::UpVector);
		if (Settings->TerrainProjection.bAlignRotationToSurface)
		{
			Forward = FVector::VectorPlaneProject(Forward, SurfaceNormal).GetSafeNormal();
		}

		FPCGPoint& Point = Points.AddDefaulted_GetRef();
		Point.Transform = FTransform(FRotationMatrix::MakeFromXZ(Forward, SurfaceNormal).ToQuat(), Position);
		Point.Transform.SetScale3D(FVector(RandomStream.FRandRange(0.85f, 1.15f)));
		Point.Seed = Settings->ScatterSeed ^ (Placed * 92837111);

		EPCGArchitecturalRole Role = EPCGArchitecturalRole::Ornament;
		switch (Settings->ScatterKind)
		{
		case EMelodiaLandscapeScatterKind::Rocks:
			Role = EPCGArchitecturalRole::Ornament;
			break;
		case EMelodiaLandscapeScatterKind::GroundCover:
			Role = EPCGArchitecturalRole::Tile;
			break;
		default:
			Role = (Placed % 3 == 0) ? EPCGArchitecturalRole::Ornament : EPCGArchitecturalRole::Tile;
			break;
		}

		MelodiaPCGBezierHelpers::StampPointAttributes(
			OutputPointData,
			Placed,
			Role,
			Slope <= Settings->TerrainProjection.MaxWalkableSlopeDegrees,
			Slope,
			0,
			0.0f,
			Forward);

		++Placed;
	}

	FPCGTaggedData& Output = Context->OutputData.TaggedData.AddDefaulted_GetRef();
	Output.Data = OutputPointData;
	Output.Pin = PCGPinConstants::DefaultOutputLabel;
	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
