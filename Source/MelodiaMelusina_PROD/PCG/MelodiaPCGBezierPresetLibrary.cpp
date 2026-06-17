// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGBezierPresetLibrary.h"
#include "MelodiaBezierPresets.h"

void UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(
	const EMelodiaBezierLayoutPreset Preset,
	TArray<FMelodiaBezierAnchorPoint>& ControlPoints,
	bool& bClosedLoop)
{
	if (Preset == EMelodiaBezierLayoutPreset::Custom)
	{
		return;
	}

	const FMelodiaBezierLayoutPresetData Data = MelodiaBezierPresets::ResolveLayoutPreset(Preset);
	ControlPoints = Data.ControlPoints;
	bClosedLoop = Data.bClosedLoop;
}

FText UMelodiaPCGBezierPresetLibrary::GetLayoutPresetDescription(const EMelodiaBezierLayoutPreset Preset)
{
	return MelodiaBezierPresets::GetLayoutPresetDescription(Preset);
}
