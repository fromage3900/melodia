// Copyright Melodia Project. All Rights Reserved.
// PCG element: stacked terrace landings along a Bezier path (portfolio hero geometry).

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierTerraceSettings.generated.h"

struct FPCGContext;

class UPCGPointData;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Terrace", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierTerraceSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierTerraceSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace")
	bool bClosedLoop = false;

	/** Number of terrace landings distributed along arc length. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "2"))
	int32 TerraceCount = 8;

	/** Width of each terrace landing perpendicular to the path (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "100"))
	float TerraceWidth = 700.0f;

	/** Depth of each terrace along the path tangent (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "100"))
	float TerraceDepth = 450.0f;

	/** Grid subdivisions across terrace width. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "2"))
	int32 WidthSubdivisions = 5;

	/** Grid subdivisions along terrace depth. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "2"))
	int32 DepthSubdivisions = 4;

	/** Vertical drop between successive terraces (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace")
	float StepDrop = 35.0f;

	/** Samples along the center walk line between terraces. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "4"))
	int32 PathSamplesPerSegment = 16;

	/** Emit Out_Path, Out_Terrace, Out_Railing on separate pins. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace")
	bool bEmitSeparatePins = true;

	/** Half-width for railing sweep on both sides of the path. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Terrace", meta = (ClampMin = "0"))
	float RailingHalfWidth = 90.0f;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void UsePortfolioTerraceDefaults();

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierTerraceElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;

private:
	static void BuildTerraceGrid(
		const FVector& Center,
		const FVector& Tangent,
		float Width,
		float Depth,
		int32 WidthSteps,
		int32 DepthSteps,
		float Height,
		EPCGArchitecturalRole Role,
		int32 Seed,
		int32 SegmentIndex,
		float PathParam,
		UPCGPointData* OutData,
		int32& InOutPointIndex);
};
