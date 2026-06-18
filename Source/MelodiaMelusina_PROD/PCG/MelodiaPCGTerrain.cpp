// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGTerrain.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "MelodiaPCGBezierHelpers.h"
#include "PCGComponent.h"
#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGMelodiaAttributes.h"

namespace MelodiaPCGTerrain
{
	UWorld* GetWorldFromPCGContext(const FPCGContext* Context)
	{
		if (!Context)
		{
			return nullptr;
		}

		const UPCGComponent* SourceComponent = Cast<UPCGComponent>(Context->ExecutionSource.Get());
		const AActor* Owner = SourceComponent ? SourceComponent->GetOwner() : nullptr;
		return Owner ? Owner->GetWorld() : nullptr;
	}

	float ComputeSlopeDegrees(const FVector& SurfaceNormal)
	{
		const float Dot = FVector::DotProduct(SurfaceNormal.GetSafeNormal(), FVector::UpVector);
		return FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Dot, -1.0f, 1.0f)));
	}

	bool ProjectWorldPosition(
		const UWorld* World,
		FVector& InOutPosition,
		FVector& OutSurfaceNormal,
		const FMelodiaPCGTerrainProjection& Options)
	{
		if (!World || !Options.bProjectToLandscape)
		{
			return false;
		}

		const float DesignHeightOffset = Options.bPreserveDesignHeightOffset ? InOutPosition.Z : 0.0f;
		const FVector TraceStart = InOutPosition + FVector(0.0f, 0.0f, Options.TraceStartOffset);
		const FVector TraceEnd = InOutPosition - FVector(0.0f, 0.0f, Options.MaxTraceDistance);

		FHitResult Hit;
		FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MelodiaPCGTerrain), false);
		QueryParams.bTraceComplex = true;

		if (!World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_WorldStatic, QueryParams))
		{
			return false;
		}

		OutSurfaceNormal = Hit.ImpactNormal.GetSafeNormal();
		const float Slope = ComputeSlopeDegrees(OutSurfaceNormal);
		if (Slope > Options.MaxWalkableSlopeDegrees)
		{
			return false;
		}

		InOutPosition = Hit.ImpactPoint + OutSurfaceNormal * Options.SurfaceOffset;
		if (Options.bPreserveDesignHeightOffset)
		{
			InOutPosition.Z += DesignHeightOffset;
		}

		return true;
	}

	void ApplyToPointData(
		UPCGPointData* PointData,
		const UWorld* World,
		const FMelodiaPCGTerrainProjection& Options)
	{
		if (!PointData || !World || !Options.bProjectToLandscape)
		{
			return;
		}

		TArray<FPCGPoint>& Points = PointData->GetMutablePoints();
		for (int32 Index = 0; Index < Points.Num(); ++Index)
		{
			FPCGPoint& Point = Points[Index];
			FVector Position = Point.Transform.GetLocation();
			FVector SurfaceNormal = FVector::UpVector;

			if (!ProjectWorldPosition(World, Position, SurfaceNormal, Options))
			{
				continue;
			}

			if (Options.bAlignRotationToSurface)
			{
				const FVector Tangent = Point.Transform.GetUnitAxis(EAxis::X);
				FVector Forward = FVector::VectorPlaneProject(Tangent, SurfaceNormal).GetSafeNormal();
				if (Forward.IsNearlyZero())
				{
					Forward = FVector::CrossProduct(SurfaceNormal, FVector::RightVector).GetSafeNormal();
				}
				Point.Transform.SetRotation(FRotationMatrix::MakeFromXZ(Forward, SurfaceNormal).ToQuat());
			}

			Point.Transform.SetLocation(Position);

			const float Slope = ComputeSlopeDegrees(SurfaceNormal);
			const bool bWalkable = Slope <= Options.MaxWalkableSlopeDegrees;

			if (PointData->Metadata && Point.MetadataEntry != PCGInvalidEntryKey)
			{
				if (FPCGMetadataAttribute<bool>* WalkAttr = PointData->Metadata->FindOrCreateAttribute<bool>(
					FMelodiaPCGAttrs::WalkableAttr, false, false, true))
				{
					WalkAttr->SetValue(Point.MetadataEntry, bWalkable);
				}
				if (FPCGMetadataAttribute<float>* SlopeAttr = PointData->Metadata->FindOrCreateAttribute<float>(
					FMelodiaPCGAttrs::SlopeAngleAttr, 0.0f, true, true))
				{
					SlopeAttr->SetValue(Point.MetadataEntry, Slope);
				}
			}
		}
	}
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
