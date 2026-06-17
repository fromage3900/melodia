// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierColonnadeSettings.h"
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

UPCGBezierColonnadeSettings::UPCGBezierColonnadeSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierColonnadeSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
}

FPCGElementPtr UPCGBezierColonnadeSettings::CreateElement() const
{
	return MakeShared<FPCGBezierColonnadeElement>();
}

bool FPCGBezierColonnadeElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierColonnadeElement::ExecuteInternal);
	check(Context);

	const UPCGBezierColonnadeSettings* Settings = Context->GetInputSettings<UPCGBezierColonnadeSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 2)
	{
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(Settings->ControlPoints, Settings->bClosedLoop, Segments);
	const float TotalLength = MelodiaPCGBezierMath::ComputeTotalArcLength(Segments);
	const float Spacing = FMath::Max(Settings->ColumnSpacing, 100.0f);
	const int32 ColumnCount = FMath::Max(FMath::CeilToInt(TotalLength / Spacing) + 1, 2);

	UPCGPointData* PointData = NewObject<UPCGPointData>();
	int32 PointIndex = 0;

	for (int32 ColumnIndex = 0; ColumnIndex < ColumnCount; ++ColumnIndex)
	{
		const float Alpha = static_cast<float>(ColumnIndex) / static_cast<float>(ColumnCount - 1);
		const float Distance = Alpha * TotalLength;

		int32 SegmentIndex = 0;
		float SegmentT = 0.0f;
		MelodiaPCGBezierMath::DistanceToSegmentT(Segments, Distance, SegmentIndex, SegmentT);

		const FVector Center = MelodiaPCGBezierMath::EvaluateCubic(Segments[SegmentIndex], SegmentT);
		FVector Tangent = MelodiaPCGBezierMath::EvaluateCubicTangent(Segments[SegmentIndex], SegmentT).GetSafeNormal();
		FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
		if (Settings->bFaceOutwardFromPath)
		{
			Right = -Right;
		}

		auto EmitColumn = [&](const float LateralOffset)
		{
			const FVector Position = Center + Right * LateralOffset;
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
					Settings->ColumnRole,
					false,
					0.0f,
					Settings->Seed,
					FTransform::Identity);
				PointIndex = 1;
			}
			else
			{
				FPCGPoint& Point = PointData->GetMutablePoints().AddDefaulted_GetRef();
				Point.Transform = FTransform(Tangent.Rotation(), Position);
				Point.Seed = Settings->Seed ^ (PointIndex * 2654435761);
				MelodiaPCGBezierHelpers::StampPointAttributes(
					PointData, PointIndex, Settings->ColumnRole, false, 0.0f,
					SegmentIndex, Alpha, Tangent);
				++PointIndex;
			}
		};

		EmitColumn(0.0f);
		if (Settings->bDualRow)
		{
			EmitColumn(Settings->RowOffset);
			EmitColumn(-Settings->RowOffset);
		}
	}

	MelodiaPCGBezierHelpers::EmitPointData(Context, PointData, PCGPinConstants::DefaultOutputLabel);
	return true;
}

#endif
