// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierSweepSettings.h"
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

UPCGBezierSweepSettings::UPCGBezierSweepSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierSweepSettings::UsePortfolioTerraceDefaults()
{
	LayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;
	ApplyLayoutPreset();
}

void UPCGBezierSweepSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
}

FPCGElementPtr UPCGBezierSweepSettings::CreateElement() const
{
	return MakeShared<FPCGBezierSweepElement>();
}

bool FPCGBezierSweepElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierSweepElement::ExecuteInternal);
	check(Context);

	const UPCGBezierSweepSettings* Settings = Context->GetInputSettings<UPCGBezierSweepSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 2)
	{
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(
		Settings->ControlPoints,
		Settings->bClosedLoop,
		Segments);

	const float TotalLength = MelodiaPCGBezierMath::ComputeTotalArcLength(Segments);
	const float Spacing = FMath::Max(Settings->SampleSpacing, 25.0f);
	const int32 PathSamples = FMath::Max(FMath::CeilToInt(TotalLength / Spacing) + 1, 2);
	const int32 LateralCount = FMath::Clamp(Settings->ProfileSampleCount, 1, 9);

	UPCGPointData* PointData = NewObject<UPCGPointData>();
	TArray<FPCGPoint>& Points = PointData->GetMutablePoints();
	Points.Reserve(PathSamples * LateralCount);

	int32 PointIndex = 0;
	for (int32 PathIndex = 0; PathIndex < PathSamples; ++PathIndex)
	{
		const float PathAlpha = static_cast<float>(PathIndex) / static_cast<float>(PathSamples - 1);
		const float Distance = PathAlpha * TotalLength;

		int32 SegmentIndex = 0;
		float SegmentT = 0.0f;
		MelodiaPCGBezierMath::DistanceToSegmentT(Segments, Distance, SegmentIndex, SegmentT);

		const FMelodiaBezierSegment& Segment = Segments[SegmentIndex];
		const FVector Center = MelodiaPCGBezierMath::EvaluateCubic(Segment, SegmentT);
		const FVector Tangent = MelodiaPCGBezierMath::EvaluateCubicTangent(Segment, SegmentT).GetSafeNormal();
		const FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
		const FVector Up = FVector::CrossProduct(Right, Tangent).GetSafeNormal();

		for (int32 LateralIndex = 0; LateralIndex < LateralCount; ++LateralIndex)
		{
			const float LateralAlpha = LateralCount == 1
				? 0.0f
				: (static_cast<float>(LateralIndex) / static_cast<float>(LateralCount - 1)) * 2.0f - 1.0f;

			const FVector Offset = Right * (LateralAlpha * Settings->ProfileHalfWidth)
				+ Up * Settings->ProfileHeightOffset;

			FMelodiaBezierPathSample Sample;
			Sample.SegmentIndex = SegmentIndex;
			Sample.SegmentT = SegmentT;
			Sample.PathAlpha = PathAlpha;
			Sample.Position = Center + Offset;
			Sample.Tangent = Tangent;

			TArray<FMelodiaBezierPathSample> SingleSample;
			SingleSample.Add(Sample);

			if (PointIndex == 0)
			{
				PointData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
					SingleSample,
					Settings->SweepRole,
					Settings->bWalkable,
					0.0f,
					Settings->Seed,
					FTransform::Identity);
				PointIndex = 1;
			}
			else
			{
				FPCGPoint& Point = PointData->GetMutablePoints().AddDefaulted_GetRef();
				Point.Transform = FTransform(Tangent.Rotation(), Sample.Position);
				Point.Seed = Settings->Seed ^ (PointIndex * 2654435761);
				MelodiaPCGBezierHelpers::StampPointAttributes(
					PointData,
					PointIndex,
					Settings->SweepRole,
					Settings->bWalkable,
					0.0f,
					SegmentIndex,
					PathAlpha,
					Tangent);
				++PointIndex;
			}
		}
	}

	UWorld* World = MelodiaPCGTerrain::GetWorldFromPCGContext(Context);
	MelodiaPCGTerrain::ApplyToPointData(PointData, World, Settings->TerrainProjection);

	MelodiaPCGBezierHelpers::EmitPointData(Context, PointData, PCGPinConstants::DefaultOutputLabel);
	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
