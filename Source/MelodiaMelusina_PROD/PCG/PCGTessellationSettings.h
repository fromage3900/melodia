// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Tessellation surface generator — header.
//
// Supports three tile shapes:
//   - Square:   uniform grid, TileType = 0 for all points.
//   - Hexagon:  offset-row hex grid, TileType = 0 for all points.
//   - Penrose:  P2 rhombus substitution tiling (sun seed + deflation).
//               TileType = 0 (fat) or 1 (thin); both guaranteed present
//               for surface area ≥ 4 × TileScale² (Requirement 3.3).

#pragma once

#include "CoreMinimal.h"
#include "PCGSettings.h"
#include "PCGElement.h"

#include "PCGTessellationSettings.generated.h"

struct FPCGContext;

/** Tile shape selector for the tessellation element. */
UENUM(BlueprintType)
enum class EPCGTileShape : uint8
{
	Square   UMETA(DisplayName = "Square"),
	Hexagon  UMETA(DisplayName = "Hexagon"),
	Penrose  UMETA(DisplayName = "Penrose")
};

// ---------------------------------------------------------------------------
// Settings — artist-editable parameters for surface tessellation.
// ---------------------------------------------------------------------------
UCLASS(BlueprintType, EditInlineNew, meta = (DisplayName = "Tessellation", Category = "Custom Elements|Geometry"))
class MELODIAMELUSINA_PROD_API UPCGTessellationSettings : public UPCGSettings
{
	GENERATED_BODY()

public:
	/** Tile shape to generate. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tessellation")
	EPCGTileShape TileShape = EPCGTileShape::Hexagon;

	/** Tile edge length / grid spacing (cm).  Clamped to ≥ 1 at runtime. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Tessellation", meta = (ClampMin = "1.0"))
	float TileScale = 200.0f;

	// UPCGSettings interface
	virtual FPCGElementPtr CreateElement() const override;
};

// ---------------------------------------------------------------------------
// Element — computes AABB from input points (or 2000×2000 cm default),
// dispatches to Square / Hexagon / Penrose builder.  No static mutable
// state (Requirement 12.4).
// ---------------------------------------------------------------------------
class MELODIAMELUSINA_PROD_API FPCGTessellationElement : public IPCGElement
{
protected:
	virtual bool ExecuteInternal(FPCGContext* Context) const override;
};
