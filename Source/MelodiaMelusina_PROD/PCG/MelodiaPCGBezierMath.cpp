// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGBezierMath.h"

namespace MelodiaPCGBezierMath
{
	FVector EvaluateCubic(const FMelodiaBezierSegment& Segment, float T)
	{
		const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
		const float U = 1.0f - ClampedT;
		const float U2 = U * U;
		const float U3 = U2 * U;
		const float T2 = ClampedT * ClampedT;
		const float T3 = T2 * ClampedT;

		return U3 * Segment.P0
			+ 3.0f * U2 * ClampedT * Segment.P1
			+ 3.0f * U * T2 * Segment.P2
			+ T3 * Segment.P3;
	}

	FVector EvaluateCubicTangent(const FMelodiaBezierSegment& Segment, float T)
	{
		const float ClampedT = FMath::Clamp(T, 0.0f, 1.0f);
		const float U = 1.0f - ClampedT;

		return 3.0f * U * U * (Segment.P1 - Segment.P0)
			+ 6.0f * U * ClampedT * (Segment.P2 - Segment.P1)
			+ 3.0f * ClampedT * ClampedT * (Segment.P3 - Segment.P2);
	}

	void BuildSegmentsFromAnchors(
		const TArray<FMelodiaBezierAnchorPoint>& Anchors,
		bool bClosedLoop,
		TArray<FMelodiaBezierSegment>& OutSegments)
	{
		OutSegments.Reset();
		if (Anchors.Num() < 2)
		{
			return;
		}

		const int32 SegmentCount = bClosedLoop ? Anchors.Num() : Anchors.Num() - 1;
		OutSegments.Reserve(SegmentCount);

		for (int32 Index = 0; Index < SegmentCount; ++Index)
		{
			const int32 NextIndex = (Index + 1) % Anchors.Num();
			const FMelodiaBezierAnchorPoint& Start = Anchors[Index];
			const FMelodiaBezierAnchorPoint& End = Anchors[NextIndex];

			FMelodiaBezierSegment Segment;
			Segment.P0 = Start.Position;
			Segment.P1 = Start.Position + Start.LeaveTangent;
			Segment.P2 = End.Position + End.ArriveTangent;
			Segment.P3 = End.Position;
			OutSegments.Add(Segment);
		}
	}

	float ComputeSegmentArcLength(const FMelodiaBezierSegment& Segment, int32 Subdivisions)
	{
		const int32 Steps = FMath::Max(Subdivisions, 4);
		float Length = 0.0f;
		FVector Previous = EvaluateCubic(Segment, 0.0f);

		for (int32 Step = 1; Step <= Steps; ++Step)
		{
			const float T = static_cast<float>(Step) / static_cast<float>(Steps);
			const FVector Current = EvaluateCubic(Segment, T);
			Length += FVector::Dist(Previous, Current);
			Previous = Current;
		}

		return Length;
	}

	float ComputeTotalArcLength(const TArray<FMelodiaBezierSegment>& Segments, int32 SubdivisionsPerSegment)
	{
		float Total = 0.0f;
		for (const FMelodiaBezierSegment& Segment : Segments)
		{
			Total += ComputeSegmentArcLength(Segment, SubdivisionsPerSegment);
		}
		return Total;
	}

	bool DistanceToSegmentT(
		const TArray<FMelodiaBezierSegment>& Segments,
		float Distance,
		int32& OutSegmentIndex,
		float& OutSegmentT,
		int32 SubdivisionsPerSegment)
	{
		if (Segments.IsEmpty())
		{
			return false;
		}

		const float ClampedDistance = FMath::Max(0.0f, Distance);
		float Remaining = ClampedDistance;

		for (int32 SegmentIndex = 0; SegmentIndex < Segments.Num(); ++SegmentIndex)
		{
			const FMelodiaBezierSegment& Segment = Segments[SegmentIndex];
			const int32 Steps = FMath::Max(SubdivisionsPerSegment, 4);
			float SegmentLength = 0.0f;
			FVector Previous = EvaluateCubic(Segment, 0.0f);

			for (int32 Step = 1; Step <= Steps; ++Step)
			{
				const float T = static_cast<float>(Step) / static_cast<float>(Steps);
				const FVector Current = EvaluateCubic(Segment, T);
				const float StepLength = FVector::Dist(Previous, Current);

				if (SegmentLength + StepLength >= Remaining)
				{
					const float LocalAlpha = StepLength > KINDA_SMALL_NUMBER
						? (Remaining - SegmentLength) / StepLength
						: 0.0f;
					const float PrevT = static_cast<float>(Step - 1) / static_cast<float>(Steps);
					OutSegmentIndex = SegmentIndex;
					OutSegmentT = FMath::Lerp(PrevT, T, LocalAlpha);
					return true;
				}

				SegmentLength += StepLength;
				Previous = Current;
			}

			Remaining -= SegmentLength;
		}

		OutSegmentIndex = Segments.Num() - 1;
		OutSegmentT = 1.0f;
		return true;
	}

	void ResamplePath(
		const TArray<FMelodiaBezierSegment>& Segments,
		EMelodiaBezierSampleMode SampleMode,
		int32 SamplesPerSegment,
		int32 MinTotalSamples,
		TArray<FMelodiaBezierPathSample>& OutSamples)
	{
		OutSamples.Reset();
		if (Segments.IsEmpty())
		{
			return;
		}

		const int32 PerSegment = FMath::Max(SamplesPerSegment, 2);
		const int32 TotalSamples = FMath::Max(MinTotalSamples, PerSegment * Segments.Num());

		if (SampleMode == EMelodiaBezierSampleMode::UniformParameter)
		{
			OutSamples.Reserve(TotalSamples);
			const float PathDenom = static_cast<float>(FMath::Max(TotalSamples - 1, 1));

			for (int32 SampleIndex = 0; SampleIndex < TotalSamples; ++SampleIndex)
			{
				const float PathAlpha = static_cast<float>(SampleIndex) / PathDenom;
				const float Scaled = PathAlpha * Segments.Num();
				const int32 SegmentIndex = FMath::Clamp(FMath::FloorToInt(Scaled), 0, Segments.Num() - 1);
				const float SegmentT = FMath::Frac(Scaled);

				FMelodiaBezierPathSample Sample;
				Sample.SegmentIndex = SegmentIndex;
				Sample.SegmentT = SegmentIndex == Segments.Num() - 1 && SampleIndex == TotalSamples - 1 ? 1.0f : SegmentT;
				Sample.PathAlpha = PathAlpha;
				Sample.Position = EvaluateCubic(Segments[SegmentIndex], Sample.SegmentT);
				Sample.Tangent = EvaluateCubicTangent(Segments[SegmentIndex], Sample.SegmentT).GetSafeNormal();
				OutSamples.Add(Sample);
			}
			return;
		}

		const float TotalLength = ComputeTotalArcLength(Segments);
		if (TotalLength <= KINDA_SMALL_NUMBER)
		{
			return;
		}

		OutSamples.Reserve(TotalSamples);
		const float LengthDenom = static_cast<float>(FMath::Max(TotalSamples - 1, 1));
		float AccumulatedDistance = 0.0f;

		for (int32 SampleIndex = 0; SampleIndex < TotalSamples; ++SampleIndex)
		{
			const float TargetDistance = (static_cast<float>(SampleIndex) / LengthDenom) * TotalLength;
			int32 SegmentIndex = 0;
			float SegmentT = 0.0f;
			DistanceToSegmentT(Segments, TargetDistance, SegmentIndex, SegmentT);

			FMelodiaBezierPathSample Sample;
			Sample.SegmentIndex = SegmentIndex;
			Sample.SegmentT = SegmentT;
			Sample.PathAlpha = static_cast<float>(SampleIndex) / LengthDenom;
			Sample.Position = EvaluateCubic(Segments[SegmentIndex], SegmentT);
			Sample.Tangent = EvaluateCubicTangent(Segments[SegmentIndex], SegmentT).GetSafeNormal();
			Sample.CumulativeDistance = TargetDistance;

			if (SampleIndex > 0)
			{
				AccumulatedDistance += FVector::Dist(OutSamples.Last().Position, Sample.Position);
			}
			OutSamples.Add(Sample);
		}
	}

	TArray<FMelodiaBezierAnchorPoint> MakePortfolioTerraceDefaults()
	{
		TArray<FMelodiaBezierAnchorPoint> Anchors;
		Anchors.Reserve(5);

		auto AddAnchor = [&Anchors](const FVector& Position, const FVector& Arrive, const FVector& Leave)
		{
			FMelodiaBezierAnchorPoint Anchor;
			Anchor.Position = Position;
			Anchor.ArriveTangent = Arrive;
			Anchor.LeaveTangent = Leave;
			Anchors.Add(Anchor);
		};

		// Rising S-curve terrace walk — tuned for portfolio establishing + walkthrough shots.
		AddAnchor(FVector(-2400.0f, -1800.0f, 0.0f), FVector(-600.0f, 0.0f, 0.0f), FVector(900.0f, 400.0f, 0.0f));
		AddAnchor(FVector(-600.0f, -400.0f, 60.0f), FVector(-500.0f, -200.0f, 20.0f), FVector(700.0f, 300.0f, 10.0f));
		AddAnchor(FVector(700.0f, 500.0f, 120.0f), FVector(-400.0f, 200.0f, 0.0f), FVector(800.0f, 500.0f, 0.0f));
		AddAnchor(FVector(1800.0f, 1400.0f, 180.0f), FVector(-700.0f, 300.0f, 0.0f), FVector(600.0f, 400.0f, 0.0f));
		AddAnchor(FVector(3200.0f, 2200.0f, 240.0f), FVector(-500.0f, 200.0f, 0.0f), FVector(400.0f, 0.0f, 0.0f));

		return Anchors;
	}
}
