// PCGGravityZoneSettings.cpp
// Implements FPCGGravityZoneElement::ExecuteInternal and
// UPCGGravityZoneSettings::CreateElement.
//
// Requirements: 1.4, 12.4
// Design: §2.4 (Execute contracts), §5.4 (GravityDir propagation)

#include "PCGGravityZoneSettings.h"
#include "PCGMelodiaAttributes.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGPin.h"

#if WITH_PCGEX
// PCGEx-specific includes go here, guarded so shipping builds compile without PCGEx.
// #include "PCGExResamplePath.h"
#endif

// ---------------------------------------------------------------------------
// FPCGGravityZoneElement
// ---------------------------------------------------------------------------

bool FPCGGravityZoneElement::ExecuteInternal(FPCGContext* Context) const
{
	TRACE_CPUPROFILER_EVENT_SCOPE(FPCGGravityZoneElement::ExecuteInternal);

	check(Context);

	// Retrieve the typed settings object (never null if the graph is wired correctly).
	const UPCGGravityZoneSettings* Settings = Context->GetInputSettings<UPCGGravityZoneSettings>();
	check(Settings);

	// Normalise the gravity direction at execution time so downstream consumers always
	// receive a unit vector, regardless of what the artist typed in the detail panel.
	// If the vector is zero (degenerate input) fall back to standard downward gravity.
	FVector GravityDir = Settings->GravityDir;
	if (!GravityDir.IsNearlyZero())
	{
		GravityDir.Normalize();
	}
	else
	{
		UE_LOG(LogPCG, Warning,
		       TEXT("PCGGravityZoneElement: GravityDir is a zero vector; "
		            "falling back to (0, 0, -1)."));
		GravityDir = FVector(0.f, 0.f, -1.f);
	}

	// Gather all tagged input data on the default "In" pin.
	TArray<FPCGTaggedData> InputData = Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);

	// Handle empty input gracefully (N=0 → return true with empty output, no crash).
	// Requirement 1.4 / task 3.2: "Handle empty input (N=0) → return true with empty output, no crash."
	if (InputData.IsEmpty())
	{
		return true;
	}

	TArray<FPCGTaggedData>& OutputData = Context->OutputData.TaggedData;

	for (const FPCGTaggedData& TaggedInput : InputData)
	{
		// We only operate on point data; pass other data types through unchanged.
		const UPCGPointData* InputPointData = Cast<UPCGPointData>(TaggedInput.Data);
		if (!InputPointData)
		{
			// Pass non-point data through unmodified.
			OutputData.Add(TaggedInput);
			continue;
		}

		// Duplicate the point data so we own the copy we are about to mutate.
		// DuplicateData gives us a deep copy of points + metadata (including any
		// existing attributes) while keeping the memory lifetime tied to this context.
		UPCGPointData* OutputPointData = NewObject<UPCGPointData>();
		OutputPointData->InitializeFromData(InputPointData);

		// Copy every point from input to output verbatim.
		const TArray<FPCGPoint>& SrcPoints = InputPointData->GetPoints();
		TArray<FPCGPoint>& DstPoints = OutputPointData->GetMutablePoints();
		DstPoints = SrcPoints;   // value-copy; FPCGPoint is a plain struct

		// Obtain (or create) the GravityDir FVector attribute on the output metadata.
		// FindOrCreateAttribute is the safe, idempotent PCG API call that either
		// retrieves an existing typed attribute or creates a new one with the supplied
		// default value.  It is the canonical way to write a custom attribute per the
		// UE5 PCG attribute writing contract (Requirement 1.4).
		UPCGMetadata* Metadata = OutputPointData->Metadata;
		check(Metadata);

		FPCGMetadataAttribute<FVector>* GravityAttr =
			Metadata->FindOrCreateAttribute<FVector>(
				FMelodiaPCGAttrs::GravityDirAttr,
				FVector(0.f, 0.f, -1.f),   // default value (standard gravity)
				/*bAllowsInterpolation=*/ false,
				/*bOverrideParent=*/ true);

		if (!GravityAttr)
		{
			UE_LOG(LogPCG, Warning,
			       TEXT("PCGGravityZoneElement: Failed to create GravityDir attribute "
			            "on output metadata. Skipping attribute write for this data set."));
		}
		else
		{
			// Stamp the normalised GravityDir onto every output point.
			// Also compute Walkable + SlopeAngle from gravity alignment.
			FPCGMetadataAttribute<bool>* WalkAttr =
				Metadata->FindOrCreateAttribute<bool>(
					FMelodiaPCGAttrs::WalkableAttr, true,
					/*bAllowsInterpolation=*/ false,
					/*bOverrideParent=*/ true);

			FPCGMetadataAttribute<float>* SlopeAttr =
				Metadata->FindOrCreateAttribute<float>(
					FMelodiaPCGAttrs::SlopeAngleAttr, 0.f,
					/*bAllowsInterpolation=*/ true,
					/*bOverrideParent=*/ true);

			// Slope angle: deviation of GravityDir from standard down (0,0,-1).
			const float GravityAngle = FMath::RadiansToDegrees(
				FMath::Acos(FMath::Clamp(
					FVector::DotProduct(GravityDir, FVector(0.f, 0.f, -1.f)),
					-1.f, 1.f)));
			const bool bWalkable = GravityAngle <= 50.f;

			for (FPCGPoint& Pt : DstPoints)
			{
				if (Pt.MetadataEntry == PCGInvalidEntryKey)
				{
					Metadata->InitializeOnSet(Pt.MetadataEntry);
				}

				GravityAttr->SetValue(Pt.MetadataEntry, GravityDir);
				if (WalkAttr) WalkAttr->SetValue(Pt.MetadataEntry, bWalkable);
				if (SlopeAttr) SlopeAttr->SetValue(Pt.MetadataEntry, GravityAngle);
			}
		}

		// Forward the output tagged data with the same tags as the input.
		FPCGTaggedData& Out = OutputData.Emplace_GetRef();
		Out.Data = OutputPointData;
		Out.Tags = TaggedInput.Tags;
		Out.Pin = PCGPinConstants::DefaultOutputLabel;
	}

	return true;
}

// ---------------------------------------------------------------------------
// UPCGGravityZoneSettings
// ---------------------------------------------------------------------------

FPCGElementPtr UPCGGravityZoneSettings::CreateElement() const
{
	return MakeShared<FPCGGravityZoneElement>();
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
