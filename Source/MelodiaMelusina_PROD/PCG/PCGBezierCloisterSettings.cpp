// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierCloisterSettings.h"
#include "MelodiaPCGBezierMath.h"
#include "MelodiaPCGBezierHelpers.h"
#include "MelodiaPCGBezierPresetLibrary.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGPointData.h"

UPCGBezierCloisterSettings::UPCGBezierCloisterSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierCloisterSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
	bClosedLoop = true;
}

FPCGElementPtr UPCGBezierCloisterSettings::CreateElement() const
{
	return MakeShared<FPCGBezierCloisterElement>();
}

bool FPCGBezierCloisterElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierCloisterElement::ExecuteInternal);
	check(Context);

	const UPCGBezierCloisterSettings* Settings = Context->GetInputSettings<UPCGBezierCloisterSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 3)
	{
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(Settings->ControlPoints, true, Segments);
	const float TotalLength = MelodiaPCGBezierMath::ComputeTotalArcLength(Segments);
	const int32 TerraceCount = FMath::Max(Settings->TerraceCount, 4);

	FVector RingCenter = FVector::ZeroVector;
	for (const FMelodiaBezierAnchorPoint& Anchor : Settings->ControlPoints)
	{
		RingCenter += Anchor.Position;
	}
	RingCenter /= static_cast<float>(Settings->ControlPoints.Num());

	UPCGPointData* PathData = NewObject<UPCGPointData>();
	UPCGPointData* TerraceData = NewObject<UPCGPointData>();
	UPCGPointData* ColumnData = NewObject<UPCGPointData>();

	TArray<FMelodiaBezierPathSample> PathSamples;
	MelodiaPCGBezierMath::ResamplePath(
		Segments,
		EMelodiaBezierSampleMode::UniformArcLength,
		16,
		TerraceCount * 8,
		PathSamples);

	PathData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
		PathSamples,
		EPCGArchitecturalRole::Floor,
		true,
		0.0f,
		Settings->Seed,
		FTransform::Identity);

	int32 TerracePointIndex = 0;
	int32 ColumnPointIndex = 0;

	for (int32 TerraceIndex = 0; TerraceIndex < TerraceCount; ++TerraceIndex)
	{
		const float Alpha = static_cast<float>(TerraceIndex) / static_cast<float>(TerraceCount);
		const float Distance = Alpha * TotalLength;

		int32 SegmentIndex = 0;
		float SegmentT = 0.0f;
		MelodiaPCGBezierMath::DistanceToSegmentT(Segments, Distance, SegmentIndex, SegmentT);

		const FVector Center = MelodiaPCGBezierMath::EvaluateCubic(Segments[SegmentIndex], SegmentT);
		FVector Tangent = MelodiaPCGBezierMath::EvaluateCubicTangent(Segments[SegmentIndex], SegmentT).GetSafeNormal();
		FVector ToCenter = (RingCenter - Center).GetSafeNormal();
		FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
		if (FVector::DotProduct(Right, ToCenter) < 0.0f)
		{
			Right = -Right;
		}

		for (int32 DepthIndex = 0; DepthIndex < Settings->DepthSubdivisions; ++DepthIndex)
		{
			const float DepthAlpha = Settings->DepthSubdivisions == 1
				? 0.0f
				: static_cast<float>(DepthIndex) / static_cast<float>(Settings->DepthSubdivisions - 1);

			for (int32 WidthIndex = 0; WidthIndex < Settings->WidthSubdivisions; ++WidthIndex)
			{
				const float WidthAlpha = Settings->WidthSubdivisions == 1
					? 0.0f
					: (static_cast<float>(WidthIndex) / static_cast<float>(Settings->WidthSubdivisions - 1)) - 0.5f;

				const FVector Position = Center
					+ Right * (WidthAlpha * Settings->TerraceWidth)
					+ ToCenter * (DepthAlpha * Settings->TerraceDepth);

				if (TerracePointIndex == 0)
				{
					TArray<FMelodiaBezierPathSample> Single;
					FMelodiaBezierPathSample Sample;
					Sample.Position = Position;
					Sample.Tangent = ToCenter;
					Sample.PathAlpha = Alpha;
					Sample.SegmentIndex = SegmentIndex;
					Single.Add(Sample);
					TerraceData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
						Single, EPCGArchitecturalRole::Tile, true, 0.0f, Settings->Seed, FTransform::Identity);
					TerracePointIndex = 1;
				}
				else
				{
					FPCGPoint& Point = TerraceData->GetMutablePoints().AddDefaulted_GetRef();
					Point.Transform = FTransform(ToCenter.Rotation(), Position);
					Point.Seed = Settings->Seed ^ (TerracePointIndex * 2654435761);
					MelodiaPCGBezierHelpers::StampPointAttributes(
						TerraceData, TerracePointIndex, EPCGArchitecturalRole::Tile, true, 0.0f,
						SegmentIndex, Alpha, ToCenter);
					++TerracePointIndex;
				}
			}
		}

		if (Settings->bPlaceCornerColumns && TerraceIndex % FMath::Max(TerraceCount / Settings->ControlPoints.Num(), 1) == 0)
		{
			const FVector ColumnPos = Center + ToCenter * (Settings->TerraceDepth * 0.5f);
			if (ColumnPointIndex == 0)
			{
				TArray<FMelodiaBezierPathSample> Single;
				FMelodiaBezierPathSample Sample;
				Sample.Position = ColumnPos;
				Sample.Tangent = ToCenter;
				Sample.PathAlpha = Alpha;
				Single.Add(Sample);
				ColumnData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
					Single, EPCGArchitecturalRole::Column, false, 0.0f, Settings->Seed, FTransform::Identity);
				ColumnPointIndex = 1;
			}
			else
			{
				FPCGPoint& Point = ColumnData->GetMutablePoints().AddDefaulted_GetRef();
				Point.Transform = FTransform(ToCenter.Rotation(), ColumnPos);
				Point.Seed = Settings->Seed ^ (ColumnPointIndex * 2654435761);
				MelodiaPCGBezierHelpers::StampPointAttributes(
					ColumnData, ColumnPointIndex, EPCGArchitecturalRole::Column, false, 0.0f,
					SegmentIndex, Alpha, ToCenter);
				++ColumnPointIndex;
			}
		}
	}

	if (Settings->bEmitSeparatePins)
	{
		MelodiaPCGBezierHelpers::EmitPointData(Context, PathData, TEXT("Out_Path"));
		MelodiaPCGBezierHelpers::EmitPointData(Context, TerraceData, TEXT("Out_Terrace"));
		if (ColumnData->GetPoints().Num() > 0)
		{
			MelodiaPCGBezierHelpers::EmitPointData(Context, ColumnData, TEXT("Out_Column"));
		}
	}
	else
	{
		MelodiaPCGBezierHelpers::EmitPointData(Context, TerraceData, PCGPinConstants::DefaultOutputLabel);
	}

	return true;
}

#endif
