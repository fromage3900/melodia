// Copyright Melodia Project. All Rights Reserved.
// Cubic Bezier evaluation and arc-length resampling for Melodia PCG elements.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBezierTypes.h"

/** One cubic Bezier segment in world space. */
struct MELODIAMELUSINA_PROD_API FMelodiaBezierSegment
{
	FVector P0 = FVector::ZeroVector;
	FVector P1 = FVector::ZeroVector;
	FVector P2 = FVector::ZeroVector;
	FVector P3 = FVector::ZeroVector;
};

/** Sample along a multi-segment Bezier path. */
struct MELODIAMELUSINA_PROD_API FMelodiaBezierPathSample
{
	int32 SegmentIndex = 0;
	float SegmentT = 0.0f;
	float PathAlpha = 0.0f;
	FVector Position = FVector::ZeroVector;
	FVector Tangent = FVector::ForwardVector;
	float CumulativeDistance = 0.0f;
};

namespace MelodiaPCGBezierMath
{
	MELODIAMELUSINA_PROD_API FVector EvaluateCubic(
		const FMelodiaBezierSegment& Segment,
		float T);

	MELODIAMELUSINA_PROD_API FVector EvaluateCubicTangent(
		const FMelodiaBezierSegment& Segment,
		float T);

	MELODIAMELUSINA_PROD_API void BuildSegmentsFromAnchors(
		const TArray<FMelodiaBezierAnchorPoint>& Anchors,
		bool bClosedLoop,
		TArray<FMelodiaBezierSegment>& OutSegments);

	MELODIAMELUSINA_PROD_API float ComputeSegmentArcLength(
		const FMelodiaBezierSegment& Segment,
		int32 Subdivisions = 32);

	MELODIAMELUSINA_PROD_API float ComputeTotalArcLength(
		const TArray<FMelodiaBezierSegment>& Segments,
		int32 SubdivisionsPerSegment = 32);

	/** Map global distance along the path to segment index + local t. */
	MELODIAMELUSINA_PROD_API bool DistanceToSegmentT(
		const TArray<FMelodiaBezierSegment>& Segments,
		float Distance,
		int32& OutSegmentIndex,
		float& OutSegmentT,
		int32 SubdivisionsPerSegment = 32);

	MELODIAMELUSINA_PROD_API void ResamplePath(
		const TArray<FMelodiaBezierSegment>& Segments,
		EMelodiaBezierSampleMode SampleMode,
		int32 SamplesPerSegment,
		int32 MinTotalSamples,
		TArray<FMelodiaBezierPathSample>& OutSamples);

	/** Portfolio hero S-curve: rising terrace walk with vista corners. */
	MELODIAMELUSINA_PROD_API TArray<FMelodiaBezierAnchorPoint> MakePortfolioTerraceDefaults();
}
