// Copyright Melodia Project. All Rights Reserved.
// PCG element: sweep a lateral profile along a cubic Bezier path (railings, colonnades).

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierSweepSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Sweep", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierSweepSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierSweepSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::GardenPromenade;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep")
	bool bClosedLoop = false;

	/** Distance between samples along the path (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep", meta = (ClampMin = "25"))
	float SampleSpacing = 150.0f;

	/** Half-width of the swept profile (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep", meta = (ClampMin = "1"))
	float ProfileHalfWidth = 80.0f;

	/** Number of lateral samples across the profile (-width .. +width). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep", meta = (ClampMin = "1", ClampMax = "9"))
	int32 ProfileSampleCount = 3;

	/** Vertical offset applied to swept points (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep")
	float ProfileHeightOffset = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep")
	EPCGArchitecturalRole SweepRole = EPCGArchitecturalRole::Railing;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Sweep")
	bool bWalkable = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FMelodiaPCGTerrainProjection TerrainProjection;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void UsePortfolioTerraceDefaults();

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierSweepElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
