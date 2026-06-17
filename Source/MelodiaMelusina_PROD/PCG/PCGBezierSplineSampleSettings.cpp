// Copyright Melodia Project. All Rights Reserved.

#include "PCGBezierSplineSampleSettings.h"
#include "MelodiaPCGBezierMath.h"
#include "MelodiaPCGBezierHelpers.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGComponent.h"
#include "PCGPin.h"
#include "Components/SplineComponent.h"
#include "EngineUtils.h"

FPCGElementPtr UPCGBezierSplineSampleSettings::CreateElement() const
{
	return MakeShared<FPCGBezierSplineSampleElement>();
}

bool FPCGBezierSplineSampleElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGBezierSplineSampleElement::ExecuteInternal);
	check(Context);

	const UPCGBezierSplineSampleSettings* Settings = Context->GetInputSettings<UPCGBezierSplineSampleSettings>();
	check(Settings);

	const UPCGComponent* SourceComponent = Cast<UPCGComponent>(Context->ExecutionSource.Get());
	const AActor* Owner = SourceComponent ? SourceComponent->GetOwner() : nullptr;
	UWorld* World = Owner ? Owner->GetWorld() : nullptr;
	if (!World)
	{
		UE_LOG(LogPCG, Warning, TEXT("PCGBezierSplineSample: no world context."));
		return true;
	}

	USplineComponent* FoundSpline = nullptr;
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		if (!Settings->SplineActorTag.IsNone() && !It->ActorHasTag(Settings->SplineActorTag))
		{
			continue;
		}

		FoundSpline = It->FindComponentByClass<USplineComponent>();
		if (FoundSpline)
		{
			break;
		}
	}

	if (!FoundSpline)
	{
		UE_LOG(LogPCG, Warning, TEXT("PCGBezierSplineSample: no spline with tag '%s'."), *Settings->SplineActorTag.ToString());
		return true;
	}

	const float SplineLength = FoundSpline->GetSplineLength();
	const float Spacing = FMath::Max(Settings->SampleSpacing, 25.0f);
	const int32 SampleCount = FMath::Clamp(FMath::CeilToInt(SplineLength / Spacing) + 1, 2, Settings->MaxSamples);

	TArray<FMelodiaBezierPathSample> Samples;
	Samples.Reserve(SampleCount);

	const FTransform SplineTransform = FoundSpline->GetComponentTransform();
	for (int32 Index = 0; Index < SampleCount; ++Index)
	{
		const float Alpha = static_cast<float>(Index) / static_cast<float>(SampleCount - 1);
		const float Distance = Alpha * SplineLength;

		FMelodiaBezierPathSample Sample;
		Sample.PathAlpha = Alpha;
		Sample.SegmentIndex = 0;
		Sample.SegmentT = Alpha;
		Sample.Position = SplineTransform.TransformPosition(FoundSpline->GetLocationAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local));
		Sample.Tangent = SplineTransform.TransformVectorNoScale(
			FoundSpline->GetTangentAtDistanceAlongSpline(Distance, ESplineCoordinateSpace::Local)).GetSafeNormal();
		Samples.Add(Sample);
	}

	UPCGPointData* PointData = MelodiaPCGBezierHelpers::CreatePointDataFromSamples(
		Samples,
		Settings->PathRole,
		Settings->bWalkable,
		0.0f,
		Settings->Seed,
		FTransform::Identity);

	MelodiaPCGBezierHelpers::EmitPointData(Context, PointData, PCGPinConstants::DefaultOutputLabel);
	return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
