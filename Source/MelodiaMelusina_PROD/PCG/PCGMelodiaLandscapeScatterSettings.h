// Copyright Melodia Project. All Rights Reserved.
// PCG element: scatter rocks and ground cover on landscape around input path bounds.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaAttributes.h"
#include "PCGMelodiaLandscapeScatterSettings.generated.h"

struct FPCGContext;

UENUM(BlueprintType)
enum class EMelodiaLandscapeScatterKind : uint8
{
	Rocks UMETA(DisplayName = "Rocks"),
	GroundCover UMETA(DisplayName = "Ground Cover"),
	Mixed UMETA(DisplayName = "Mixed"),
};

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Melodia Landscape Scatter", Category = "Custom Elements|Terrain"))
class MELODIAMELUSINA_PROD_API UPCGMelodiaLandscapeScatterSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	EMelodiaLandscapeScatterKind ScatterKind = EMelodiaLandscapeScatterKind::Mixed;

	/** Extra margin around the input point bounds (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0"))
	float BoundsMargin = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "1"))
	int32 TargetCount = 120;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "1"))
	int32 MaxAttempts = 2000;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0", ClampMax = "90"))
	float MinSlopeDegrees = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter", meta = (ClampMin = "0", ClampMax = "90"))
	float MaxSlopeDegrees = 40.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Scatter")
	int32 ScatterSeed = 1337;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FMelodiaPCGTerrainProjection TerrainProjection;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGMelodiaLandscapeScatterElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
