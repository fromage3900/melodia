// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Gravity zone attribute writer — header.
//
// Accepts an input point set and stamps a GravityDir (FVector) metadata
// attribute on every point so downstream spawners can orient meshes to
// a non-standard gravity axis (upside-down, sideways, etc.).

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"

#include "PCGGravityZoneSettings.generated.h"

struct FPCGContext;

// ---------------------------------------------------------------------------
// Settings — configurable gravity direction for impossible-geometry zones.
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Gravity Zone", Category = "Custom Elements|Environment"))
class MELODIAMELUSINA_PROD_API UPCGGravityZoneSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	/** Override gravity direction.  Normalised at execution time; (0,0,-1) = standard down. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Gravity Zone")
	FVector GravityDir = FVector(0.0f, 0.0f, -1.0f);

	// UPCGSettings interface
	virtual FPCGElementPtr CreateElement() const override;
};

// ---------------------------------------------------------------------------
// Element — reads input point data, duplicates it, writes GravityDir attribute
// per point.  Handles empty input gracefully (N=0 → empty output, no crash).
// ---------------------------------------------------------------------------
class MELODIAMELUSINA_PROD_API FPCGGravityZoneElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
