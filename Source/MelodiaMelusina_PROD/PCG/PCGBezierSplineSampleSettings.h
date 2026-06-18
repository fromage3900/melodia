// Copyright Melodia Project. All Rights Reserved.
// PCG element: sample an in-level USplineComponent into Melodia path points.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGBezierSplineSampleSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Bezier Spline Sample", Category = "Custom Elements|Bezier Portfolio"))
class MELODIAMELUSINA_PROD_API UPCGBezierSplineSampleSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	/** Actor tag used to locate a level spline (first match wins). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Sample")
	FName SplineActorTag = TEXT("Melodia.Portfolio.PathSpline");

	/** Distance between samples along spline length (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Sample", meta = (ClampMin = "25"))
	float SampleSpacing = 120.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Sample", meta = (ClampMin = "2", ClampMax = "512"))
	int32 MaxSamples = 256;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Sample")
	EPCGArchitecturalRole PathRole = EPCGArchitecturalRole::Floor;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Spline Sample")
	bool bWalkable = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FMelodiaPCGTerrainProjection TerrainProjection;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGBezierSplineSampleElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
