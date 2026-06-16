// Copyright Melodia Project. All Rights Reserved.
// Runtime walkable-point cache for PCG-generated data.
//
// Place one AMelodiaPCGWalkableIndex in the level.  On BeginPlay it
// auto-discovers nearby PCG components and rebuilds its cache whenever
// any tracked component finishes generation.  Gameplay systems query
// the cache for O(1) nearest-walkable-point lookups.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PCGMelodiaAttributes.h"
#include "MelodiaPCGWalkableIndex.generated.h"

class UPCGComponent;

/**
 * Caches walkable PCG point positions for fast gameplay queries.
 * Rebuilds automatically when PCG generation completes.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGWalkableIndex : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGWalkableIndex();

	/** Search radius for auto-discovering PCG components (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Index",
		meta = (ClampMin = "100"))
	float DiscoveryRadius = 20000.0f;

	/** Maximum slope angle to consider walkable (degrees). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Index",
		meta = (ClampMin = "0", ClampMax = "90"))
	float MaxSlopeAngle = 50.0f;

	/** Rebuild the cache now from all tracked PCG components. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Index")
	void RebuildCache();

	/** Find the nearest cached walkable point to Location.
	 *  Returns true if found within MaxRadius. OutNormal is set if available. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Index")
	bool FindNearestWalkable(
		const FVector& Location,
		float MaxRadius,
		FVector& OutPosition,
		EPCGArchitecturalRole& OutRole,
		FVector& OutNormal) const;

	/** Find a random cached walkable point near Location within Radius. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Index")
	bool FindRandomWalkable(
		const FVector& Location,
		float Radius,
		int32 Seed,
		FVector& OutPosition) const;

	/** Number of cached walkable points. */
	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Index")
	int32 GetCachedPointCount() const;

	/** Get all cached positions (for debugging / visualization). */
	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Index")
	TArray<FVector> GetCachedPositions() const;

	/** Get all cached roles (same order as positions). */
	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Index")
	TArray<EPCGArchitecturalRole> GetCachedRoles() const;

	/** Get all cached surface normals (same order as positions). */
	UFUNCTION(BlueprintPure, Category = "Melodia|PCG Index")
	TArray<FVector> GetCachedNormals() const;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

private:
	/** Cached walkable point positions. */
	TArray<FVector> CachedPositions;

	/** Cached architectural roles (parallel array to CachedPositions). */
	TArray<EPCGArchitecturalRole> CachedRoles;

	/** Cached surface normals (parallel array to CachedPositions). */
	TArray<FVector> CachedNormals;

	/** Weak references to tracked PCG components. */
	TArray<TWeakObjectPtr<UPCGComponent>> TrackedComponents;

	/** Timer handle for debounced rebuilds. */
	FTimerHandle RebuildTimerHandle;

	/** Discover PCG components within DiscoveryRadius. */
	void DiscoverPCGComponents();

	/** Schedule a debounced rebuild on the next tick. */
	void ScheduleRebuild();
};
