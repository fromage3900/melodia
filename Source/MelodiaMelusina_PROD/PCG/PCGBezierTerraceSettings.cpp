// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierTerraceSettings.h"
#include "MelodiaPCGBezierMath.h"
#include "MelodiaPCGBezierHelpers.h"
#include "MelodiaPCGTerrain.h"
#include "MelodiaPCGBezierPresetLibrary.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGPin.h"
#include "Data/PCGPointData.h"

namespace MelodiaPCGBezierTerraceSettingsPrivate
{
	static TArray<FPCGPinProperties> MakeSeparatePointPins(const TArray<FName>& Labels)
	{
		TArray<FPCGPinProperties> Pins;
		Pins.Reserve(Labels.Num());
		for (const FName& Label : Labels)
		{
			Pins.Emplace(Label, FPCGDataTypeIdentifier(EPCGDataType::Point));
		}
		return Pins;
	}
}

TArray<FPCGPinProperties> UPCGBezierTerraceSettings::OutputPinProperties() const
{
	if (bEmitSeparatePins)
	{
		return MelodiaPCGBezierTerraceSettingsPrivate::MakeSeparatePointPins({
			TEXT("Out_Path"),
			TEXT("Out_Terrace"),
			TEXT("Out_Railing"),
		});
	}
	return Super::DefaultPointOutputPinProperties();
}

UPCGBezierTerraceSettings::UPCGBezierTerraceSettings()
{
	ApplyLayoutPreset();
}

void UPCGBezierTerraceSettings::UsePortfolioTerraceDefaults()
{
	LayoutPreset = EMelodiaBezierLayoutPreset::PortfolioTerrace;
	ApplyLayoutPreset();
}

void UPCGBezierTerraceSettings::ApplyLayoutPreset()
{
	UMelodiaPCGBezierPresetLibrary::ApplyLayoutPreset(LayoutPreset, ControlPoints, bClosedLoop);
}

FPCGElementPtr UPCGBezierTerraceSettings::CreateElement() const
{
	return MakeShared<FPCGBezierTerraceElement>();
}

void FPCGBezierTerraceElement::BuildTerraceGrid(
	const FVector& Center,
	const FVector& Tangent,
	const float Width,
	const float Depth,
	const int32 WidthSteps,
	const int32 DepthSteps,
	const float Height,
	const EPCGArchitecturalRole Role,
	const int32 Seed,
	const int32 SegmentIndex,
	const float PathParam,
	UPCGPointData* OutData,
	int32& InOutPointIndex)
{
	const FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
	const FVector Forward = Tangent.GetSafeNormal();

	for (int32 DepthIndex = 0; DepthIndex < DepthSteps; ++DepthIndex)
	{
		const float DepthAlpha = DepthSteps == 1
			? 0.0f
			: (static_cast<float>(DepthIndex) / static_cast<float>(DepthSteps - 1)) - 0.5f;

		for (int32 WidthIndex = 0; WidthIndex < WidthSteps; ++WidthIndex)
		{
			const float WidthAlpha = WidthSteps == 1
				? 0.0f
				: (static_cast<float>(WidthIndex) / static_cast<float>(WidthSteps - 1)) - 0.5f;

			const FVector Position = Center
				+ Right * (WidthAlpha * Width)
				+ Forward * (DepthAlpha * Depth)
				+ FVector(0.0f, 0.0f, Height);

			FPCGPoint& Point = OutData->GetMutablePoints().AddDefaulted_GetRef();
			Point.Transform = FTransform(Forward.Rotation(), Position);
			Point.Seed = Seed ^ (InOutPointIndex * 2654435761);

			MelodiaPCGBezierHelpers::StampPointAttributes(
				OutData,
				InOutPointIndex,
				Role,
				true,
				0.0f,
				SegmentIndex,
				PathParam,
				Forward);

			++InOutPointIndex;
		}
	}
}

bool FPCGBezierTerraceElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierTerraceElement::ExecuteInternal);
	check(Context);

	const UPCGBezierTerraceSettings* Settings = Context->GetInputSettings<UPCGBezierTerraceSettings>();
	check(Settings);

	if (Settings->ControlPoints.Num() < 2)
	{
		return true;
	}

	TArray<FMelodiaBezierSegment> Segments;
	MelodiaPCGBezierMath::BuildSegmentsFromAnchors(
		Settings->ControlPoints,
		Settings->bClosedLoop,
		Segments);

	const float TotalLength = MelodiaPCGBezierMath::ComputeTotalArcLength(Segments);
	const int32 TerraceCount = FMath::Max(Settings->TerraceCount, 2);

	UPCGPointData* PathData = NewObject<UPCGPointData>();
	UPCGPointData* TerraceData = NewObject<UPCGPointData>();
	UPCGPointData* RailingData = NewObject<UPCGPointData>();

	TArray<FMelodiaBezierPathSample> PathSamples;
	MelodiaPCGBezierMath::ResamplePath(
		Segments,
		EMelodiaBezierSampleMode::UniformArcLength,
		Settings->PathSamplesPerSegment,
		Segments.Num() * Settings->PathSamplesPerSegment,
		PathSamples);

	PathData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
		PathSamples,
		EPCGArchitecturalRole::Floor,
		true,
		0.0f,
		Settings->Seed,
		FTransform::Identity);

	int32 TerracePointIndex = 0;
	int32 RailingPointIndex = 0;

	for (int32 TerraceIndex = 0; TerraceIndex < TerraceCount; ++TerraceIndex)
	{
		const float PathAlpha = static_cast<float>(TerraceIndex) / static_cast<float>(TerraceCount - 1);
		const float Distance = PathAlpha * TotalLength;

		int32 SegmentIndex = 0;
		float SegmentT = 0.0f;
		MelodiaPCGBezierMath::DistanceToSegmentT(Segments, Distance, SegmentIndex, SegmentT);

		const FVector Center = MelodiaPCGBezierMath::EvaluateCubic(Segments[SegmentIndex], SegmentT);
		const FVector Tangent = MelodiaPCGBezierMath::EvaluateCubicTangent(Segments[SegmentIndex], SegmentT).GetSafeNormal();
		const float Height = static_cast<float>(TerraceIndex) * Settings->StepDrop;

		BuildTerraceGrid(
			Center,
			Tangent,
			Settings->TerraceWidth,
			Settings->TerraceDepth,
			Settings->WidthSubdivisions,
			Settings->DepthSubdivisions,
			Height,
			EPCGArchitecturalRole::Tile,
			Settings->Seed,
			SegmentIndex,
			PathAlpha,
			TerraceData,
			TerracePointIndex);

		if (Settings->RailingHalfWidth > 0.0f)
		{
			const FVector Right = FVector::CrossProduct(Tangent, FVector::UpVector).GetSafeNormal();
			const FVector Up = FVector::CrossProduct(Right, Tangent).GetSafeNormal();
			const float RailingHeight = Height + 110.0f;

			for (int32 Side = -1; Side <= 1; Side += 2)
			{
				const FVector RailingPos = Center
					+ Right * (static_cast<float>(Side) * (Settings->TerraceWidth * 0.5f + Settings->RailingHalfWidth))
					+ FVector(0.0f, 0.0f, RailingHeight);

				if (RailingData->GetPoints().IsEmpty())
				{
					TArray<FMelodiaBezierPathSample> Single;
					FMelodiaBezierPathSample Sample;
					Sample.Position = RailingPos;
					Sample.Tangent = Tangent;
					Sample.SegmentIndex = SegmentIndex;
					Sample.PathAlpha = PathAlpha;
					Single.Add(Sample);
					RailingData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
						Single,
						EPCGArchitecturalRole::Railing,
						false,
						0.0f,
						Settings->Seed,
						FTransform::Identity);
					RailingPointIndex = 1;
				}
				else
				{
					FPCGPoint& Point = RailingData->GetMutablePoints().AddDefaulted_GetRef();
					Point.Transform = FTransform(Tangent.Rotation(), RailingPos);
					Point.Seed = Settings->Seed ^ (RailingPointIndex * 2654435761);
					MelodiaPCGBezierHelpers::StampPointAttributes(
						RailingData,
						RailingPointIndex,
						EPCGArchitecturalRole::Railing,
						false,
						0.0f,
						SegmentIndex,
						PathAlpha,
						Tangent);
					++RailingPointIndex;
				}
			}
		}
	}

	UWorld* World = MelodiaPCGTerrain::GetWorldFromPCGContext(Context);
	MelodiaPCGTerrain::ApplyToPointData(PathData, World, Settings->TerrainProjection);
	MelodiaPCGTerrain::ApplyToPointData(TerraceData, World, Settings->TerrainProjection);
	if (RailingData)
	{
		MelodiaPCGTerrain::ApplyToPointData(RailingData, World, Settings->TerrainProjection);
	}

	if (Settings->bEmitSeparatePins)
	{
		MelodiaPCGBezierHelpers::EmitPointData(Context, PathData, TEXT("Out_Path"));
		MelodiaPCGBezierHelpers::EmitPointData(Context, TerraceData, TEXT("Out_Terrace"));
		if (RailingData && RailingData->GetPoints().Num() > 0)
		{
			MelodiaPCGBezierHelpers::EmitPointData(Context, RailingData, TEXT("Out_Railing"));
		}
	}
	else
	{
		UPCGPointData* Combined = PathData;
		for (const FPCGPoint& Pt : TerraceData->GetPoints())
		{
			const int32 Index = Combined->GetMutablePoints().Add(Pt);
			MelodiaPCGBezierHelpers::StampPointAttributes(
				Combined,
				Index,
				EPCGArchitecturalRole::Tile,
				true,
				0.0f,
				0,
				0.0f,
				FVector::ForwardVector);
		}
		if (RailingData)
		{
			for (const FPCGPoint& Pt : RailingData->GetPoints())
			{
				const int32 Index = Combined->GetMutablePoints().Add(Pt);
				MelodiaPCGBezierHelpers::StampPointAttributes(
					Combined,
					Index,
					EPCGArchitecturalRole::Railing,
					false,
					0.0f,
					0,
					0.0f,
					FVector::ForwardVector);
			}
		}
		MelodiaPCGBezierHelpers::EmitPointData(Context, Combined, PCGPinConstants::DefaultOutputLabel);
	}

	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
