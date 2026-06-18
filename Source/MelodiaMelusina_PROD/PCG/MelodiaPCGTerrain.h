// Copyright Melodia Project. All Rights Reserved.
// Landscape line-trace projection for Melodia PCG points.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBezierTypes.h"

class UPCGPointData;
class UWorld;
struct FPCGContext;

namespace MelodiaPCGTerrain
{
	MELODIAMELUSINA_PROD_API UWorld* GetWorldFromPCGContext(const FPCGContext* Context);

	MELODIAMELUSINA_PROD_API float ComputeSlopeDegrees(const FVector& SurfaceNormal);

	/** Trace downward and update position (and optionally normal). Returns false if no hit. */
	MELODIAMELUSINA_PROD_API bool ProjectWorldPosition(
		const UWorld* World,
		FVector& InOutPosition,
		FVector& OutSurfaceNormal,
		const FMelodiaPCGTerrainProjection& Options);

	MELODIAMELUSINA_PROD_API void ApplyToPointData(
		UPCGPointData* PointData,
		const UWorld* World,
		const FMelodiaPCGTerrainProjection& Options);
}
