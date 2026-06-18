// Copyright Melodia Project. All Rights Reserved.
// PCG element: resample cubic Bezier control points into walkable path points.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierPathSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Path", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierPathSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierPathSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	/** Anchor points defining the cubic Bezier spline. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	/** Connect last anchor back to the first. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path")
	bool bClosedLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path")
	EMelodiaBezierSampleMode SampleMode = EMelodiaBezierSampleMode::UniformArcLength;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path", meta = (ClampMin = "2"))
	int32 SamplesPerSegment = 24;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path", meta = (ClampMin = "2"))
	int32 MinTotalSamples = 48;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path")
	EPCGArchitecturalRole PathRole = EPCGArchitecturalRole::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path")
	bool bWalkable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Path", meta = (ClampMin = "0", ClampMax = "90"))
	float SlopeAngle = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FMelodiaPCGTerrainProjection TerrainProjection;

	/** Load the portfolio hero S-curve terrace anchors. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void UsePortfolioTerraceDefaults();

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierPathElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
