// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierOrnamentScatterSettings.h"
#include "MelodiaPCGBezierMath.h"
#include "MelodiaPCGBezierHelpers.h"
#include "MelodiaPCGTerrain.h"
#include "MelodiaPCGBezierPresetLibrary.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGPointData.h"

UPCGBezierOrnamentScatterSettings::UPCGBezierOrnamentScatterSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierOrnamentScatterSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
}

FPCGElementPtr UPCGBezierOrnamentScatterSettings::CreateElement() const
{
	return MakeShared<FPCGBezierOrnamentScatterElement>();
}

bool FPCGBezierOrnamentScatterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierOrnamentScatterElement::ExecuteInternal);
	check(Context);

	const UPCGBezierOrnamentScatterSettings* Settings = Context->GetInputSettings<UPCGBezierOrnamentScatterSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 2)
	{
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(Settings->ControlPoints, Settings->bClosedLoop, Segments);
	const float TotalLength = MelodiaPCGBezierMath::ComputeTotalArcLength(Segments);
	const int32 Count = FMath::Clamp(Settings->ScatterCount, 1, 512);
	const float StartAlpha = FMath::Clamp(Settings->PathStartAlpha, 0.0f, 1.0f);
	const float EndAlpha = FMath::Max(StartAlpha, FMath::Clamp(Settings->PathEndAlpha, 0.0f, 1.0f));

	FRandomStream Rng(Settings->Seed);
	UPCGPointData* PointData = NewObject<UPCGPointData>();
	int32 PointIndex = 0;

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = FMath::Lerp(StartAlpha, EndAlpha, static_cast<float>(Index) / FMath::Max(Count - 1, 1));
		const float Distance = Alpha * TotalLength;

		int32 SegmentIndex = 0;
		float SegmentT = 0.0f;
		MelodiaPCGBezierMath::DistanceToSegmentT(Segments, Distance, SegmentIndex, SegmentT);

		const FVector Center = MelodiaPCGBezierMath::EvaluateCubic(Segments[SegmentIndex], SegmentT);
		const FVector Tangent = MelodiaPCGBezierMath::EvaluateCubicTangent(Segments[SegmentIndex], SegmentT).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();

		const FVector Position = Center
			+ Right * Rng.FRandRange(-Settings->LateralJitter, Settings->LateralJitter)
			+ FVector(0.0f, 0.0f, Rng.FRandRange(-Settings->VerticalJitter, Settings->VerticalJitter));

		FMelodiaBezierPathSample Sample;
		Sample.Position = Position;
		Sample.Tangent = Tangent;
		Sample.PathAlpha = Alpha;
		Sample.SegmentIndex = SegmentIndex;
		Sample.SegmentT = SegmentT;

		if (PointIndex == 0)
		{
			TArray<FMelodiaBezierPathSample> Single;
			Single.Add(Sample);
			PointData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
				Single,
				Settings->OrnamentRole,
				false,
				0.0f,
				Settings->Seed ^ Index,
				FTransform::Identity);
			PointIndex = 1;
		}
		else
		{
			FPCGPoint& Point = PointData->GetMutablePoints().AddDefaulted_GetRef();
			Point.Transform = FTransform(Tangent.Rotation(), Position);
			Point.Seed = Settings->Seed ^ (Index * 2654435761);
			MelodiaPCGBezierHelpers::StampPointAttributes(
				PointData, PointIndex, Settings->OrnamentRole, false, 0.0f,
				SegmentIndex, Alpha, Tangent);
			++PointIndex;
		}
	}

	UWorld* World = MelodiaPCGTerrain::GetWorldFromPCGContext(Context);
	MelodiaPCGTerrain::ApplyToPointData(PointData, World, Settings->TerrainProjection);

	MelodiaPCGBezierHelpers::EmitPointData(Context, PointData, PCGPinConstants::DefaultOutputLabel);
	return true;
}

#endif
