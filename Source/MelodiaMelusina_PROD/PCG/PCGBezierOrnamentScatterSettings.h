// Copyright Melodia Project. All Rights Reserved.
// PCG element: scatter ornaments along a Bezier path with lateral jitter.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierOrnamentScatterSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Ornament Scatter", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierOrnamentScatterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierOrnamentScatterSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::GardenPromenade;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter")
	bool bClosedLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter", meta = (ClampMin = "1", ClampMax = "512"))
	int32 ScatterCount = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter", meta = (ClampMin = "0"))
	float LateralJitter = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter", meta = (ClampMin = "0"))
	float VerticalJitter = 30.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter", meta = (ClampMin = "0", ClampMax = "1"))
	float PathStartAlpha = 0.05f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter", meta = (ClampMin = "0", ClampMax = "1"))
	float PathEndAlpha = 0.95f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ornament Scatter")
	EPCGArchitecturalRole OrnamentRole = EPCGArchitecturalRole::Ornament;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierOrnamentScatterElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
