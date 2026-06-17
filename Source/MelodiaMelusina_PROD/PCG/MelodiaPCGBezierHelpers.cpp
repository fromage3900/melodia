// Copyright Melodia Project. All Rights Reserved.

#include "MelodiaPCGBezierHelpers.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"

namespace MelodiaPCGBezierHelpers
{
	namespace
	{
		struct FAttrCache
		{
			FPCGMetadataAttribute<int32>* RoleAttr = nullptr;
			FPCGMetadataAttribute<bool>* WalkAttr = nullptr;
			FPCGMetadataAttribute<float>* SlopeAttr = nullptr;
			FPCGMetadataAttribute<int32>* SegmentAttr = nullptr;
			FPCGMetadataAttribute<float>* PathParamAttr = nullptr;
			FPCGMetadataAttribute<FVector>* TangentAttr = nullptr;
		};

		FAttrCache EnsureAttributes(UPCGPointData* PointData)
		{
			FAttrCache Cache;
			UPCGMetadata* Metadata = PointData ? PointData->Metadata : nullptr;
			if (!Metadata)
			{
				return Cache;
			}

			Cache.RoleAttr = Metadata->FindOrCreateAttribute<int32>(
				FMelodiaPCGAttrs::ArchitecturalRoleAttr,
				static_cast<int32>(EPCGArchitecturalRole::None),
				false,
				true);

			Cache.WalkAttr = Metadata->FindOrCreateAttribute<bool>(
				FMelodiaPCGAttrs::WalkableAttr,
				false,
				false,
				true);

			Cache.SlopeAttr = Metadata->FindOrCreateAttribute<float>(
				FMelodiaPCGAttrs::SlopeAngleAttr,
				0.0f,
				true,
				true);

			Cache.SegmentAttr = Metadata->FindOrCreateAttribute<int32>(
				FMelodiaPCGAttrs::BezierSegmentAttr,
				0,
				false,
				true);

			Cache.PathParamAttr = Metadata->FindOrCreateAttribute<float>(
				FMelodiaPCGAttrs::PathParamAttr,
				0.0f,
				true,
				true);

			Cache.TangentAttr = Metadata->FindOrCreateAttribute<FVector>(
				FMelodiaPCGAttrs::PathTangentAttr,
				FVector::ForwardVector,
				true,
				true);

			return Cache;
		}
	}

	void StampPointAttributes(
		UPCGPointData* PointData,
		int32 PointIndex,
		EPCGArchitecturalRole Role,
		bool bWalkable,
		float SlopeAngle,
		int32 SegmentIndex,
		float PathParam,
		const FVector& PathTangent)
	{
		if (!PointData || !PointData->Metadata)
		{
			return;
		}

		TArray<FPCGPoint>& Points = PointData->GetMutablePoints();
		if (!Points.IsValidIndex(PointIndex))
		{
			return;
		}

		const FAttrCache Cache = EnsureAttributes(PointData);
		FPCGPoint& Point = Points[PointIndex];
		if (Point.MetadataEntry == PCGInvalidEntryKey)
		{
			PointData->Metadata->InitializeOnSet(Point.MetadataEntry);
		}

		if (Cache.RoleAttr)
		{
			Cache.RoleAttr->SetValue(Point.MetadataEntry, static_cast<int32>(Role));
		}
		if (Cache.WalkAttr)
		{
			Cache.WalkAttr->SetValue(Point.MetadataEntry, bWalkable);
		}
		if (Cache.SlopeAttr)
		{
			Cache.SlopeAttr->SetValue(Point.MetadataEntry, SlopeAngle);
		}
		if (Cache.SegmentAttr)
		{
			Cache.SegmentAttr->SetValue(Point.MetadataEntry, SegmentIndex);
		}
		if (Cache.PathParamAttr)
		{
			Cache.PathParamAttr->SetValue(Point.MetadataEntry, PathParam);
		}
		if (Cache.TangentAttr)
		{
			Cache.TangentAttr->SetValue(Point.MetadataEntry, PathTangent.GetSafeNormal());
		}
	}

	UPCGPointData* CreatePointDataFromSamples(
		const TArray<FMelodiaBezierPathSample>& Samples,
		EPCGArchitecturalRole Role,
		bool bWalkable,
		float SlopeAngle,
		int32 Seed,
		const FTransform& LocalTransform)
	{
		UPCGPointData* PointData = NewObject<UPCGPointData>();
		TArray<FPCGPoint>& Points = PointData->GetMutablePoints();
		Points.Reserve(Samples.Num());
		EnsureAttributes(PointData);

		for (int32 Index = 0; Index < Samples.Num(); ++Index)
		{
			const FMelodiaBezierPathSample& Sample = Samples[Index];
			FPCGPoint& Point = Points.AddDefaulted_GetRef();
			Point.Transform = FTransform(
				Sample.Tangent.Rotation(),
				LocalTransform.TransformPosition(Sample.Position));
			Point.Seed = Seed ^ (Index * 2654435761);

			StampPointAttributes(
				PointData,
				Index,
				Role,
				bWalkable,
				SlopeAngle,
				Sample.SegmentIndex,
				Sample.PathAlpha,
				Sample.Tangent);
		}

		return PointData;
	}

	void EmitPointData(FPCGContext* Context, UPCGPointData* PointData, FName OutputPin)
	{
		if (!Context || !PointData)
		{
			return;
		}

		FPCGTaggedData& Output = Context->OutputData.TaggedData.AddDefaulted_GetRef();
		Output.Data = PointData;
		Output.Pin = OutputPin;
	}
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
