// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierPathSettings.h"
#include "MelodiaPCGBezierMath.h"
#include "MelodiaPCGBezierHelpers.h"
#include "MelodiaPCGBezierPresetLibrary.h"
#include "MelodiaPCGTerrain.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGModule.h"
#include "PCGPin.h"

UPCGBezierPathSettings::UPCGBezierPathSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierPathSettings::UsePortfolioTerraceDefaults()
{
	LayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;
	ApplyLayoutPreset();
}

void UPCGBezierPathSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
}

FPCGElementPtr UPCGBezierPathSettings::CreateElement() const
{
	return MakeShared<FPCGBezierPathElement>();
}

bool FPCGBezierPathElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierPathElement::ExecuteInternal);
	check(Context);

	const UPCGBezierPathSettings* Settings = Context->GetInputSettings<UPCGBezierPathSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 2)
	{
		UE_LOG(LogPCG, Warning, TEXT("PCGBezierPath: need at least 2 control points."));
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(
		Settings->ControlPoints,
		Settings->bClosedLoop,
		Segments);

	TArray<FMelodiaBezierPathSample> Samples;
	MelodiaPCGBezierMath::ResamplePath(
		Segments,
		Settings->SampleMode,
		Settings->SamplesPerSegment,
		Settings->MinTotalSamples,
		Samples);

	UPCGPointData* PointData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
		Samples,
		Settings->PathRole,
		Settings->bWalkable,
		Settings->SlopeAngle,
		Settings->Seed,
		FTransform::Identity);

	MelodiaPCGTerrain::ApplyToPointData(
		PointData,
		MelodiaPCGTerrain::GetWorldFromPCGContext(Context),
		Settings->TerrainProjection);

	MelodiaPCGBezierHelpers::EmitPointData(Context, PointData, PCGPinConstants::DefaultOutputLabel);
	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
