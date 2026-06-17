// Copyright Melodia Project. All Rights Reserved.
// Shared layout-preset helpers for all Bezier PCG settings classes.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MelodiaBezierTypes.h"
#include "MelodiaPCGBezierPresetLibrary.generated.h"

UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPCGBezierPresetLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG|Bezier")
	static void ApplyLayoutPreset(
		EMelodiaBezierLayoutPreset Preset,
		UPARAM(ref) TArray<FMelodiaBezierAnchorPoint>& ControlPoints,
		UPARAM(ref) bool& bClosedLoop);

	UFUNCTION(BlueprintPure, Category = "Melodia|PCG|Bezier")
	static FText GetLayoutPresetDescription(EMelodiaBezierLayoutPreset Preset);
};
