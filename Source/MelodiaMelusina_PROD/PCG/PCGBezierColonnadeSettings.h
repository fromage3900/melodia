// Copyright Melodia Project. All Rights Reserved.
// PCG element: column rows along a cubic Bezier path.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierColonnadeSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Colonnade", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierColonnadeSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPCGBezierColonnadeSettings();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Layout|Presets", meta = (DisplayPriority = 0))
	EMelodiaBezierLayoutPreset LayoutPreset = EMelodiaBezierLayoutPreset::ColonnadeAvenue;

	UFUNCTION(BlueprintCallable, CallInEditor, Category = "Layout|Presets")
	void ApplyLayoutPreset();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade")
	TArray<FMelodiaBezierAnchorPoint> ControlPoints;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade")
	bool bClosedLoop = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade", meta = (ClampMin = "100"))
	float ColumnSpacing = 350.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade", meta = (ClampMin = "0"))
	float RowOffset = 280.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade")
	bool bDualRow = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade")
	bool bFaceOutwardFromPath = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier Colonnade")
	EPCGArchitecturalRole ColumnRole = EPCGArchitecturalRole::Column;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierColonnadeElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
