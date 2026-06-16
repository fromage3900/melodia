// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Recursive arch generator — implementation.
//
// Requirements: 1.5, 1.7, 4.3, 4.6, 12.4
// Design: §2.4 (Execute contracts — FPCGRecursiveArchElement),
//         §2.5 (PCGEx guard pattern),
//         §5.1 (RecursionTier attribute)
//
// Algorithm summary
// -----------------
// For depth d = 0 .. RecursionDepth-1:
//   currentWidth  = ArchWidth  * ScaleFactor^d
//   currentHeight = ArchHeight * ScaleFactor^d
//   N             = max(8, 16 - d*2)
//
//   For i = 0 .. N-1:
//     angle = PI * i / (N-1)
//     X     = cos(angle) * (currentWidth / 2)
//     Y     = 0                                   (arch lies in XZ plane)
//     Z     = sin(angle) * currentHeight
//
//   Each tier's points are emitted as a separate FPCGTaggedData on pin
//   "Out_Tier0", "Out_Tier1", … "Out_TierN" so downstream nodes can address
//   individual tiers independently (Requirement 4.3 / design §3.3).
//
//   Every point carries a RecursionTier (int32) attribute = d
//   (Requirement 4.4 / design §5.1).
//
// Bounding-box width invariant (Requirement 4.3):
//   bbox_width[d] = ArchWidth * ScaleFactor^d
//   bbox_width[d+1] / bbox_width[d] = ScaleFactor   (exact, within float precision)
//   The 2% tolerance cited in the requirement is comfortably satisfied because the
//   formula is deterministic multiplication — no approximation error accumulates.
//
// Thread-safety invariant (Requirement 12.4):
//   No static mutable state.  All temporaries are stack-local or allocated through
//   NewObject<> whose lifetime is managed by the PCG framework's context.

#include "PCGRecursiveArchSettings.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 0
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGPin.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"

#if WITH_PCGEX
// PCGEx-specific includes go here when PCGEx types are needed.
// All guarded so shipping builds compile without PCGExtendedToolkit (Requirement 1.8).
// Example: #include "PCGExResamplePath.h"
#endif

#if WITH_EDITOR
// Editor-only includes (e.g. detail panel customisation) go here.
// Requirement 12.5: shipping builds must compile without editor dependencies.
#endif

// ---------------------------------------------------------------------------
// FPCGRecursiveArchElement::ExecuteInternal
// ---------------------------------------------------------------------------

bool FPCGRecursiveArchElement::ExecuteInternal(FPCGContext* Context) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGRecursiveArchElement::ExecuteInternal);

    check(Context);

    // -----------------------------------------------------------------------
    // 1. Retrieve typed settings (Requirement 1.5)
    // -----------------------------------------------------------------------
    const UPCGRecursiveArchSettings* Settings =
        Context->GetInputSettings<UPCGRecursiveArchSettings>();
    check(Settings);

    // -----------------------------------------------------------------------
    // 2. Parameter clamping preamble (Requirements 1.7, 4.6)
    //
    //    RecursionDepth outside [1,4]:
    //      clamp + UE_LOG(LogPCG, Warning, …)
    //
    //    ScaleFactor below 0.3:
    //      clamp to 0.3 + Context->LogAndNotifyUser(…) for blueprint-visible
    //      warning (Requirement 4.6).
    //
    //    All clamped values are stack-local; Settings is never mutated.
    // -----------------------------------------------------------------------

    int32 RecursionDepth = Settings->RecursionDepth;

    if (RecursionDepth < 1)
    {
        UE_LOG(LogPCG, Warning,
               TEXT("PCGRecursiveArchElement: RecursionDepth %d is less than 1, clamped to 1."),
               Settings->RecursionDepth);
        RecursionDepth = 1;
    }
    else if (RecursionDepth > 4)
    {
        UE_LOG(LogPCG, Warning,
               TEXT("PCGRecursiveArchElement: RecursionDepth %d is greater than 4, clamped to 4."),
               Settings->RecursionDepth);
        RecursionDepth = 4;
    }

    float ScaleFactor = Settings->ScaleFactor;

    if (ScaleFactor < 0.3f)
    {
        // Blueprint-visible warning via LogAndNotifyUser (Requirement 4.6).
        // This surfaces in the PCG component's notification panel at editor time
        // so artists see it without opening the output log.
        Context->LogAndNotifyUser(
            FText::Format(
                NSLOCTEXT("PCGRecursiveArch", "ScaleFactorClamped",
                    "PCGRecursiveArchElement: ScaleFactor {0} is below the minimum 0.3 "
                    "and has been clamped. Reduce recursion depth to achieve smaller "
                    "arch scales."),
                FText::AsNumber(Settings->ScaleFactor)));

        ScaleFactor = 0.3f;
    }

    const float ArchWidth  = Settings->ArchWidth;
    const float ArchHeight = Settings->ArchHeight;

    // -----------------------------------------------------------------------
    // 3. The attribute name shared across all tiers (design §5.1).
    //    Declared here so it is constructed once per Execute() call rather
    //    than inside the inner loop.
    // -----------------------------------------------------------------------
    static const FName RecursionTierAttributeName(TEXT("RecursionTier"));

    // -----------------------------------------------------------------------
    // 4. Per-tier generation loop
    //
    //    For d = 0 .. RecursionDepth-1:
    //      - Compute scaled dimensions.
    //      - Generate N semi-circle points in the XZ plane.
    //      - Write RecursionTier (int32) attribute = d to every point.
    //      - Emit as FPCGTaggedData on pin "Out_TierN".
    //
    //    Each iteration allocates a fresh UPCGPointData through NewObject<> so
    //    point sets are independently owned by the PCG context and can be
    //    individually wired downstream (Requirement 12.4 — no shared mutable state).
    // -----------------------------------------------------------------------

    for (int32 d = 0; d < RecursionDepth; ++d)
    {
        // Scale the arch dimensions for this tier.
        const float Scale         = FMath::Pow(ScaleFactor, static_cast<float>(d));
        const float CurrentWidth  = ArchWidth  * Scale;
        const float CurrentHeight = ArchHeight * Scale;

        // Number of arc sample points: max(8, 16 - d*2).
        // Outer tiers get more points (16) for smoother arches; inner tiers
        // reduce by 2 per level down to a minimum of 8, keeping point counts
        // low for the 16 ms budget (Requirement 12.3, design §2.4).
        const int32 N = FMath::Max(8, 16 - d * 2);

        // Allocate the output point data for this tier.
        UPCGPointData* TierData = NewObject<UPCGPointData>();
        TArray<FPCGPoint>& Points = TierData->GetMutablePoints();
        Points.Reserve(N);

        // Semi-circle: angles 0 .. PI, N evenly spaced samples.
        // angle_i = PI * i / (N-1)
        // X = cos(angle_i) * (currentWidth / 2)
        // Y = 0   (arch lies in XZ plane)
        // Z = sin(angle_i) * currentHeight
        const float HalfWidth = CurrentWidth * 0.5f;

        for (int32 i = 0; i < N; ++i)
        {
            const float Angle = PI * static_cast<float>(i) / static_cast<float>(N - 1);
            const float X     =  FMath::Cos(Angle) * HalfWidth;
            const float Y     =  0.f;
            const float Z     =  FMath::Sin(Angle) * CurrentHeight;

            FPCGPoint& Pt = Points.AddDefaulted_GetRef();
            Pt.Transform.SetLocation(FVector(X, Y, Z));

            // Tier-local seed: XOR settings seed with tier index and point index.
            // Uses a large prime spread to keep seeds well-distributed.
            Pt.Seed = Settings->GetSeed() ^ ((d * 31337) + (i * 2654435761u));
        }

        // --------------------------------------------------------------------
        // Write RecursionTier (int32) attribute to every point on this tier.
        //
        // FindOrCreateAttribute is the safe, idempotent PCG metadata API that
        // either retrieves an existing attribute or creates it.  Writing per-
        // point via SetValue ensures downstream attribute filters can select
        // individual tiers (Requirement 4.4, design §5.1).
        // --------------------------------------------------------------------
        UPCGMetadata* Metadata = TierData->Metadata;
        check(Metadata);

        FPCGMetadataAttribute<int32>* TierAttr =
            Metadata->FindOrCreateAttribute<int32>(
                RecursionTierAttributeName,
                /*DefaultValue=*/ 0,
                /*bAllowsInterpolation=*/ false,
                /*bOverrideParent=*/ true);

        if (TierAttr)
        {
            for (FPCGPoint& Pt : Points)
            {
                if (Pt.MetadataEntry == PCGInvalidEntryKey)
                {
                    Metadata->InitializeOnSet(Pt.MetadataEntry);
                }
                TierAttr->SetValue(Pt.MetadataEntry, d);
            }
        }
        else
        {
            UE_LOG(LogPCG, Warning,
                   TEXT("PCGRecursiveArchElement: Failed to create RecursionTier "
                        "attribute for tier %d. Points emitted without attribute."), d);
        }

        // --------------------------------------------------------------------
        // Emit this tier as a separate FPCGTaggedData on its own output pin.
        // Pin name: "Out_Tier0", "Out_Tier1", …, "Out_TierN" (design §2.4).
        // --------------------------------------------------------------------
        FPCGTaggedData& Out = Context->OutputData.TaggedData.AddDefaulted_GetRef();
        Out.Data = TierData;
        Out.Pin  = FName(*FString::Printf(TEXT("Out_Tier%d"), d));
    }

    return true;
}

// ---------------------------------------------------------------------------
// UPCGRecursiveArchSettings::CreateElement
// ---------------------------------------------------------------------------

FPCGElementPtr UPCGRecursiveArchSettings::CreateElement() const
{
    return MakeShared<FPCGRecursiveArchElement>();
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
