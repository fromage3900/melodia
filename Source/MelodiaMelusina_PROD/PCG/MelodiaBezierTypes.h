// Copyright Melodia Project. All Rights Reserved.
// Shared Bezier anchor types for Melodia PCG curve geometry.

#pragma once

#include "CoreMinimal.h"
#include "MelodiaBezierTypes.generated.h"

/** One anchor on a cubic Bezier spline (UE spline tangent convention).
 *  Segment i→i+1 uses P0=Position[i], P1=Position[i]+LeaveTangent[i],
 *  P2=Position[i+1]+ArriveTangent[i+1], P3=Position[i+1]. */
USTRUCT(BlueprintType)
struct MELODIAMELUSINA_PROD_API FMelodiaBezierAnchorPoint
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier")
	FVector Position = FVector::ZeroVector;

	/** Incoming handle offset (added to Position at this anchor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier")
	FVector ArriveTangent = FVector(-400.0f, 0.0f, 0.0f);

	/** Outgoing handle offset (added to Position at this anchor). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bezier")
	FVector LeaveTangent = FVector(400.0f, 0.0f, 0.0f);
};

/** Sampling strategy when converting a Bezier path to discrete PCG points. */
UENUM(BlueprintType)
enum class EMelodiaBezierSampleMode : uint8
{
	/** Evenly spaced in parameter space t∈[0,1] per segment. */
	UniformParameter UMETA(DisplayName = "Uniform Parameter"),

	/** Approximately even spacing by arc length (portfolio walk paths). */
	UniformArcLength UMETA(DisplayName = "Uniform Arc Length"),
};

/** Artist-friendly layout presets — one-click curves for level building. */
UENUM(BlueprintType)
enum class EMelodiaBezierLayoutPreset : uint8
{
	Custom UMETA(DisplayName = "Custom (manual anchors)"),
	PortfolioTerrace UMETA(DisplayName = "Portfolio Terrace S-Curve"),
	CloisterRing UMETA(DisplayName = "Cloister Ring"),
	ColonnadeAvenue UMETA(DisplayName = "Colonnade Avenue"),
	GardenPromenade UMETA(DisplayName = "Garden Promenade"),
	BridgeSpan UMETA(DisplayName = "Bridge Span"),
	CathedralNaveAxis UMETA(DisplayName = "Cathedral Nave Axis"),
	FloatingBalcony UMETA(DisplayName = "Floating Balcony"),
	PenroseApproach UMETA(DisplayName = "Penrose Wavy Approach"),
	EscherSwitchback UMETA(DisplayName = "Escher Switchback"),
};

/** Built-in PCG graph catalog IDs for AMelodiaPCGLevelKit dropdowns. */
UENUM(BlueprintType)
enum class EMelodiaPCGGraphId : uint8
{
	PortfolioTerraceBezier UMETA(DisplayName = "Portfolio Terrace Bezier"),
	BezierPathPortfolio UMETA(DisplayName = "Bezier Path + Sweep"),
	BezierCloisterRing UMETA(DisplayName = "Bezier Cloister Ring"),
	BezierColonnadeAvenue UMETA(DisplayName = "Bezier Colonnade Avenue"),
	BezierGardenPromenade UMETA(DisplayName = "Bezier Garden Promenade"),
	BezierBridgeSpan UMETA(DisplayName = "Bezier Bridge Span"),
	BezierCathedralAxis UMETA(DisplayName = "Bezier Cathedral Axis"),
	BezierVistaTerrace UMETA(DisplayName = "Bezier Vista Terrace"),
	BezierOrnamentGallery UMETA(DisplayName = "Bezier Ornament Gallery"),
	BezierSplineGarden UMETA(DisplayName = "Bezier Spline Garden"),
	TerraceGarden UMETA(DisplayName = "Terrace Garden (classic)"),
	Custom UMETA(DisplayName = "Custom Graph Asset"),
};
