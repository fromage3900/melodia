// Copyright Melodia Project. All Rights Reserved.
// Shared helpers for stamping Melodia attributes on Bezier-generated PCG points.

#pragma once

#include "CoreMinimal.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaBezierTypes.h"
#include "MelodiaPCGBezierMath.h"

class UPCGPointData;
struct FPCGContext;
struct FPCGPoint;

namespace MelodiaPCGBezierHelpers
{
	MELODIAMELUSINA_PROD_API void StampPointAttributes(
		UPCGPointData* PointData,
		int32 PointIndex,
		EPCGArchitecturalRole Role,
		bool bWalkable,
		float SlopeAngle,
		int32 SegmentIndex,
		float PathParam,
		const FVector& PathTangent);

	MELODIAMELUSINA_PROD_API UPCGPointData* CreatePointDataFromSamples(
		const TArray<FMelodiaBezierPathSample>& Samples,
		EPCGArchitecturalRole Role,
		bool bWalkable,
		float SlopeAngle,
		int32 Seed,
		const FTransform& LocalTransform);

	MELODIAMELUSINA_PROD_API void EmitPointData(
		FPCGContext* Context,
		UPCGPointData* PointData,
		FName OutputPin);
}
