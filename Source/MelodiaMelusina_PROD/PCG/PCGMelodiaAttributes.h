// Copyright Melodia Project. All Rights Reserved.
// Shared PCG attribute vocabulary for Melodia PCG elements.
//
// Defines canonical attribute names and enums used across all custom PCG
// elements so downstream filters, spawners, and gameplay systems can query
// points by semantic role.

#pragma once

#include "CoreMinimal.h"
#include "PCGMelodiaAttributes.generated.h"

// PCGExtendedToolkit is not enabled for this project. Define the guard macro so
// `#if WITH_PCGEX` blocks compile cleanly (avoids C4668 warning-as-error).
#ifndef WITH_PCGEX
#define WITH_PCGEX 0
#endif

/** Semantic role of a PCG-generated point within the architecture.
 *  Used by downstream spawners, encounter triggers, and the loop verifier
 *  to decide which meshes / gameplay actors to place at each point. */
UENUM(BlueprintType)
enum class EPCGArchitecturalRole : uint8
{
	None        UMETA(DisplayName = "None"),
	Wall        UMETA(DisplayName = "Wall"),
	Column      UMETA(DisplayName = "Column"),
	Cornice     UMETA(DisplayName = "Cornice"),
	Roof        UMETA(DisplayName = "Roof"),
	Door        UMETA(DisplayName = "Door"),
	Bridge      UMETA(DisplayName = "Bridge"),
	Tower       UMETA(DisplayName = "Tower"),
	Stair       UMETA(DisplayName = "Stair"),
	Tile        UMETA(DisplayName = "Tile"),
	Railing     UMETA(DisplayName = "Railing"),
	Floor       UMETA(DisplayName = "Floor"),
	Ornament    UMETA(DisplayName = "Ornament"),
};

/** Canonical attribute name constants shared across all Melodia PCG elements. */
namespace FMelodiaPCGAttrs
{
	/** int32 — EPCGArchitecturalRole value describing the point's semantic role. */
	static const FName ArchitecturalRoleAttr(TEXT("ArchitecturalRole"));

	/** bool — true if the point is on a walkable surface (slope ≤ 50°). */
	static const FName WalkableAttr(TEXT("Walkable"));

	/** float — surface slope angle in degrees (0 = flat, 90 = vertical). */
	static const FName SlopeAngleAttr(TEXT("SlopeAngle"));

	/** FVector — gravity direction unit vector (default: (0,0,-1)). */
	static const FName GravityDirAttr(TEXT("GravityDir"));

	/** int32 — recursion tier index (0 = outermost arch). */
	static const FName RecursionTierAttr(TEXT("RecursionTier"));

	/** int32 — tile type (0 = square/fat, 1 = thin rhombus). */
	static const FName TileTypeAttr(TEXT("TileType"));
}
