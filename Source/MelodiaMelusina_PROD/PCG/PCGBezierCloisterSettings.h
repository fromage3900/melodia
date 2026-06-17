// Copyright Melodia Project. All Rights Reserved.
// PCG element: closed cloister ring with inward-facing terrace platforms.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierCloisterSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Cloister", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierCloisterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierCloisterSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::CloisterRing;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister")
	bool bClosedLoop = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister", meta = (ClampMin = "4"))
	int32 TerraceCount = 12;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister", meta = (ClampMin = "200"))
	float TerraceWidth = 500.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister", meta = (ClampMin = "100"))
	float TerraceDepth = 300.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister", meta = (ClampMin = "2"))
	int32 WidthSubdivisions = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister", meta = (ClampMin = "2"))
	int32 DepthSubdivisions = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister")
	bool bEmitSeparatePins = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Cloister")
	bool bPlaceCornerColumns = true;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierCloisterElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
