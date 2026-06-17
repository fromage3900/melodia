// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Recursive arch generator — header.
//
// Generates nested semicircular arch tiers, each scaled by ScaleFactor
// (default 0.618 — golden ratio) relative to the parent.  Each tier is
// emitted on its own output pin (Out_Tier0 … Out_TierN) with a
// RecursionTier (int32) attribute for downstream filtering.

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"

#include "PCGRecursiveArchSettings.generated.h"

struct FPCGContext;

// ---------------------------------------------------------------------------
// Settings — artist-editable parameters for the recursive arch generator.
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Recursive Arch", Category = "Custom Elements|Architecture"))
class MELODIAMELUSINA_PROD_API UPCGRecursiveArchSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	/** Width of the outermost arch (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recursive Arch")
	float ArchWidth = 400.0f;

	/** Height of the outermost arch (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recursive Arch")
	float ArchHeight = 600.0f;

	/** Number of recursion tiers.  Clamped to [1, 4] at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recursive Arch", meta = (ClampMin = "1", ClampMax = "4"))
	int32 RecursionDepth = 3;

	/** Per-tier shrink factor.  Clamped to ≥ 0.3 at runtime.  Default = golden ratio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Recursive Arch", meta = (ClampMin = "0.3"))
	float ScaleFactor = 0.618f;

	// UPCGSettings interface
	virtual FPCGElementPtr CreateElement() const override;
};

// ---------------------------------------------------------------------------
// Element — generates semicircular arc points per tier with RecursionTier
// attribute.  No static mutable state (Requirement 12.4).
// ---------------------------------------------------------------------------
class MELODIAMELUSINA_PROD_API FPCGRecursiveArchElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
