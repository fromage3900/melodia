// Copyright Melodia Project. All Rights Reserved.
// Programmatic test harness for validating PCG element output.
//
// Drop this actor into any test level.  On demand (or auto on BeginPlay),
// it discovers all PCG components, validates their generated output against
// per-element rules, and logs detailed pass/fail diagnostics.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Data/PCGPointData.h"
#include "PCGMelodiaAttributes.h"
#include "PCGPoint.h"
#include "MelodiaPCGTestHarness.generated.h"

class UPCGComponent;
class UPCGPointData;
class UPCGMetadata;

/** Result of testing a single PCG component. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FPCGElementTestResult
{
	GENERATED_BODY()

	/** Name of the element / graph being tested. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Test")
	FString ElementName;

	/** Whether the test passed. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Test")
	bool bPassed = false;

	/** Total number of points generated. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Test")
	int32 PointCount = 0;

	/** Number of walkable points found. */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Test")
	int32 WalkableCount = 0;

	/** Human-readable failure reason (empty if passed). */
	UPROPERTY(BlueprintReadOnly, Category = "Melodia|PCG Test")
	FString FailureReason;
};

/**
 * Drop-in test harness for validating PCG element output in test levels.
 * Place one in each test level; configure bRunOnBeginPlay for CI-style checks.
 */
UCLASS(Blueprintable)
class MELODIAMELUSINA_PROD_API AMelodiaPCGTestHarness : public AActor
{
	GENERATED_BODY()

public:
	AMelodiaPCGTestHarness();

	/** Automatically run tests on BeginPlay after a delay. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Test")
	bool bRunOnBeginPlay = false;

	/** Delay before auto-running tests (allows PCG to finish generating). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Test",
		meta = (EditCondition = "bRunOnBeginPlay", ClampMin = "0"))
	float DelaySeconds = 2.0f;

	/** If true, spawn debug spheres at walkable points for visual inspection. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Test")
	bool bSpawnDebugMarkers = false;

	/** Radius of debug marker spheres (cm). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Melodia|PCG Test",
		meta = (EditCondition = "bSpawnDebugMarkers", ClampMin = "1"))
	float DebugMarkerRadius = 50.0f;

	/** Run all PCG validation tests and return results. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Test")
	TArray<FPCGElementTestResult> RunAllTests();

	/** Clear debug markers and re-run all tests. Convenience for iterative level design. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Test")
	TArray<FPCGElementTestResult> ResetAndRerun();

	/** Remove all debug markers spawned by the last test run. */
	UFUNCTION(BlueprintCallable, Category = "Melodia|PCG Test")
	void ClearDebugMarkers();

protected:
	virtual void BeginPlay() override;

private:
	/** Test a single PCG component and return the result. */
	FPCGElementTestResult TestPCGComponent(UPCGComponent* PCGComp);

	void RunAllTestsDelayed();

	/** Per-element validation rules. */
	bool ValidateEscherStaircase(
		const TArray<FPCGPoint>& Points,
		UPCGMetadata* Meta,
		FString& OutReason);

	bool ValidateGravityZone(
		const TArray<FPCGPoint>& Points,
		UPCGMetadata* Meta,
		FString& OutReason);

	bool ValidateRecursiveArch(
		const TArray<FPCGPoint>& Points,
		UPCGMetadata* Meta,
		FString& OutReason);

	bool ValidateTessellation(
		const TArray<FPCGPoint>& Points,
		UPCGMetadata* Meta,
		FString& OutReason);

	bool ValidateBezierPath(
		const TArray<FPCGPoint>& Points,
		UPCGMetadata* Meta,
		FString& OutReason);

	/** Spawn a debug sphere at a location. */
	void SpawnDebugMarker(const FVector& Location, FLinearColor Color);

	/** All debug marker actors spawned during the last test run. */
	TArray<TObjectPtr<AActor>> DebugMarkers;
};
