// Copyright Melodia Project. All Rights Reserved.
// Shared Blueprint function library for querying PCG-generated data at runtime.
//
// Provides reusable static functions that gameplay systems (encounter triggers,
// spawners, walkable indexes) use to read PCG point attributes such as
// ArchitecturalRole, Walkable, and SlopeAngle.

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaPCGLibrary.generated.h"

class UPCGComponent;
class UPCGPointData;
class UPCGMetadata;
class AMelodiaPCGWalkableIndex;
class AMelodiaPCGEncounterSpawner;

/** A single walkable point extracted from PCG output data. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaWalkablePoint
{
	GENERATED_BODY()

	/** World-space location of the point. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG")
	FVector Location = FVector::ZeroVector;

	/** Architectural role stamped on this point by the PCG element. */
	UPROPERTY(BlueprintReadWrite, Category = "Melodia|PCG")
	EPCGArchitecturalRole Role = EPCGArchitecturalRole::None;

	/** Surface slope angle in degrees (0 = flat, 90 = vertical). */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG")
	float SlopeAngle = 0.0f;

	/** Surface normal (approximated from slope angle; default: UpVector). */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG")
	FVector Normal = FVector::UpVector;
};

/**
 * Blueprint-accessible library of static helpers for querying PCG data.
 *
 * All functions are safe to call at runtime; they gracefully handle null
 * components, empty output, and missing attributes.
 */
UCLASS()
class MELODIAMELUSINA_PROD_API UMelodiaPCGLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	// -----------------------------------------------------------------
	// Core queries
	// -----------------------------------------------------------------

	/** Extract all walkable points from a single PCG component's generated output. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG")
	static TArray<FMelodiaWalkablePoint> GetWalkablePoints(
		const UPCGComponent* PCGComp,
		float MaxSlopeAngle = 50.0f);

	/** Extract ALL points from a PCG component (no walkable filter). For debugging/visualization. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG")
	static TArray<FMelodiaWalkablePoint> GetAllPoints(
		const UPCGComponent* PCGComp);

	/** Find the nearest walkable point to Origin within SearchRadius.
	 *  Returns true if a point was found; OutLocation is set accordingly. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG")
	static bool FindNearestWalkablePoint(
		const UPCGComponent* PCGComp,
		const FVector& Origin,
		float SearchRadius,
		FVector& OutLocation);

	/** Filter an array of walkable points by architectural role. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG")
	static TArray<FMelodiaWalkablePoint> FilterByRole(
		const TArray<FMelodiaWalkablePoint>& Points,
		EPCGArchitecturalRole Role);

	// -----------------------------------------------------------------
	// World-level queries
	// -----------------------------------------------------------------

	/** Collect all PCG components within Radius of Origin in the world. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static TArray<UPCGComponent*> CollectPCGComponents(
		const UObject* WorldContextObject,
		const FVector& Origin,
		float Radius);

	/** Total walkable point count across all PCG components in the world. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static int32 GetTotalWalkablePointCount(const UObject* WorldContextObject);

	/** Check if any walkable PCG point exists within Tolerance of Location. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static bool IsLocationNearWalkablePoint(
		const UObject* WorldContextObject,
		const FVector& Location,
		float Tolerance);

	/** Get walkable points filtered by role from all PCG components in radius. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static TArray<FMelodiaWalkablePoint> GetWalkablePointsByRoleInRadius(
		const UObject* WorldContextObject,
		const FVector& Center,
		float Radius,
		EPCGArchitecturalRole Role);

	/** Find the first AMelodiaPCGWalkableIndex actor in the world. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static AMelodiaPCGWalkableIndex* FindWalkableIndex(const UObject* WorldContextObject);

	/** Find the first AMelodiaPCGEncounterSpawner actor in the world. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG",
		meta = (WorldContext = "WorldContextObject"))
	static AMelodiaPCGEncounterSpawner* FindEncounterSpawner(const UObject* WorldContextObject);

	// -----------------------------------------------------------------
	// Internal helpers (C++ only — not exposed to Blueprint)
	// -----------------------------------------------------------------

	/** Extract points from a single UPCGPointData block.
	 *  If bFilterWalkable is true, only walkable points within MaxSlopeAngle are included. */
	static void ExtractPointsFromData(
		const UPCGPointData* Data,
		const FTransform& WorldTransform,
		float MaxSlopeAngle,
		bool bFilterWalkable,
		TArray<FMelodiaWalkablePoint>& OutPoints);
};
