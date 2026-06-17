// Copyright Melodia Project. All Rights Reserved.
// Layout preset library — one-click Bezier control-point layouts for level building.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBezierTypes.h"

struct MELODIAMELUSINA_PROD_API FMelodiaBezierLayoutPresetData
{
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;
	bool bClosedLoop = false;
};

namespace MelodiaBezierPresets
{
	MELODIAMELUSINA_PROD_API FMelodiaBezierLayoutPresetData ResolveLayoutPreset(EMelodiaBezierLayoutPreset Preset);

	MELODIAMELUSINA_PROD_API FText GetLayoutPresetDisplayName(EMelodiaBezierLayoutPreset Preset);

	MELODIAMELUSINA_PROD_API FText GetLayoutPresetDescription(EMelodiaBezierLayoutPreset Preset);

	MELODIAMELUSINA_PROD_API EMelodiaBezierLayoutPreset GetSuggestedPresetForGraph(EMelodiaPCGGraphId GraphId);
}
