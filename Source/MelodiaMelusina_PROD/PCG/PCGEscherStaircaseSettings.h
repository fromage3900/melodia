// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Escher infinite staircase loop generator — header.
//
// Generates a closed helix of points that visually loops like an Escher
// ascending staircase.  The quadratic Z taper ensures the final step lands
// back near Z=0 so the loop closes within ≤ 1 cm (Requirement 1.3).

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"

#include "PCGEscherStaircaseSettings.generated.h"

struct FPCGContext;

// ---------------------------------------------------------------------------
// Settings — artist-editable parameters for the infinite staircase generator.
// ---------------------------------------------------------------------------
UCLASS(BlueprintType)
class MELODIAMELUSINA_PROD_API UPCGEscherStaircaseSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	/** Number of steps in the staircase loop.  Clamped to ≥ 3 at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escher", meta = (ClampMin = "3"))
	int32 StepCount = 16;

	/** Vertical rise per step (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escher")
	float StepHeight = 25.0f;

	/** Tread width per step (cm).  Reserved for future mesh-offset use. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escher")
	float StepWidth = 150.0f;

	/** Helix radius (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Escher")
	float LoopRadius = 600.0f;

	// UPCGSettings interface
	virtual FPCGElementPtr CreateElement() const override;
};

// ---------------------------------------------------------------------------
// Element — stateless executor.  All temporaries are stack-local or
// context-lifetime managed (Requirement 12.4 thread-safety).
// ---------------------------------------------------------------------------
class MELODIAMELUSINA_PROD_API FPCGEscherStaircaseElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
