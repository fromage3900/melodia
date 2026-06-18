// Copyright Melodia Project. All Rights Reserved.
// PCG element: project input points onto landscape via line trace.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"
#include "MelodiaBezierTypes.h"
#include "PCGMelodiaProjectLandscapeSettings.generated.h"

struct FPCGContext;

UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Melodia Project Landscape", Category = "Custom Elements|Terrain"))
class MELODIAMELUSINA_PROD_API UPCGMelodiaProjectLandscapeSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Terrain")
	FMelodiaPCGTerrainProjection TerrainProjection;

	virtual FPCGElementPtr CreateElement() const override;
};

class MELODIAMELUSINA_PROD_API FPCGMelodiaProjectLandscapeElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
