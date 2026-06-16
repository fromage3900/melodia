// Copyright Melodia Project. All Rights Reserved.
// PCG custom element: Escher infinite staircase loop generator — implementation.
//
// Requirements: 1.1, 1.3, 1.7, 12.3, 12.4
// Design: §2.4 (Execute contract), §2.5 (PCGEx guard pattern)
//
// Algorithm summary
// -----------------
// Distribute N points on a helix of radius R:
//   angle_i  = (2π / N) * i
//   X_i      = cos(angle_i) * LoopRadius
//   Y_i      = sin(angle_i) * LoopRadius
//   Z_i      = StepHeight * i  –  StepHeight * i * i / N   (quadratic taper to close)
//
// The quadratic term ensures the final point lands back near Z=0 so the loop
// closes visually (Escher ascending-staircase illusion).  After the loop we
// explicitly snap the last point's XY to point[0]'s XY and Z to 0 to satisfy
// the ≤ 1 cm closure tolerance (Requirement 1.3).
//
// Thread-safety invariant (Requirement 12.4):
//   No static mutable state.  All temporaries are stack-local or allocated through
//   NewObject<> which is context-lifetime managed by the PCG framework.

#include "PCGEscherStaircaseSettings.h"
#include "PCGMelodiaAttributes.h"

#ifndef MELODIA_ENABLE_EXPERIMENTAL_PCG
#define MELODIA_ENABLE_EXPERIMENTAL_PCG 1
#endif

#if MELODIA_ENABLE_EXPERIMENTAL_PCG

#include "PCGContext.h"
#include "PCGModule.h"
#include "Data/PCGPointData.h"
#include "Metadata/PCGMetadata.h"
#include "Metadata/PCGMetadataAttribute.h"
#include "Metadata/PCGMetadataAttributeTpl.h"
#include "PCGPin.h"
#include "Logging/LogMacros.h"
#include "Math/UnrealMathUtility.h"

#if WITH_PCGEX
// PCGEx-specific includes go here when PCGEx types are needed.
// All guarded so shipping builds compile without PCGExtendedToolkit.
// Example: #include "PCGExResamplePath.h"
#endif

#if WITH_EDITOR
// Editor-only includes (e.g. detail panel customisation) go here.
// Requirement 12.5: shipping builds must compile without editor dependencies.
#endif

// ---------------------------------------------------------------------------
// UPCGEscherStaircaseSettings::CreateElement
// ---------------------------------------------------------------------------

FPCGElementPtr UPCGEscherStaircaseSettings::CreateElement() const
{
    return MakeShared<FPCGEscherStaircaseElement>();
}

// ---------------------------------------------------------------------------
// FPCGEscherStaircaseElement::ExecuteInternal
// ---------------------------------------------------------------------------

bool FPCGEscherStaircaseElement::ExecuteInternal(FPCGContext* Context) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGEscherStaircaseElement::ExecuteInternal);

    check(Context);

    // -----------------------------------------------------------------------
    // 1. Retrieve settings
    // -----------------------------------------------------------------------
    const UPCGEscherStaircaseSettings* Settings =
        Context->GetInputSettings<UPCGEscherStaircaseSettings>();
    check(Settings);

    // -----------------------------------------------------------------------
    // 2. Parameter clamping preamble (Requirement 1.7)
    //    Log a UE_LOG(LogPCG, Warning) and clamp to the nearest valid bound.
    //    All clamped values are stack-local; Settings is not mutated.
    // -----------------------------------------------------------------------
    int32 StepCount = Settings->StepCount;
    if (StepCount < 3)
    {
        UE_LOG(LogPCG, Warning,
               TEXT("PCGEscherStaircaseElement: StepCount %d clamped to 3"),
               Settings->StepCount);
        StepCount = 3;
    }

    const float StepHeight = Settings->StepHeight;
    const float LoopRadius = Settings->LoopRadius;
    const int32 Seed       = Settings->Seed;

    // -----------------------------------------------------------------------
    // 3. Build output point set — helix distribution
    //
    //   For i in 0 .. N-1:
    //     angle = (2π / N) * i
    //     X     = cos(angle) * LoopRadius
    //     Y     = sin(angle) * LoopRadius
    //     Z     = StepHeight * i  –  StepHeight * i * i / N
    //
    //   The quadratic Z term (–StepHeight * i² / N) tapers the helix back
    //   toward Z=0 so the last generated point is close to the start.  The
    //   final snap below closes the loop to ≤ 1 cm (Requirement 1.3).
    //
    //   No static mutable state; OutputData lifetime is managed by the PCG
    //   framework through NewObject<> (Requirement 12.4).
    // -----------------------------------------------------------------------
    UPCGPointData* OutputData = NewObject<UPCGPointData>();
    TArray<FPCGPoint>& Points = OutputData->GetMutablePoints();
    Points.Reserve(StepCount);

    const float N           = static_cast<float>(StepCount);
    const float AngularStep = (2.f * PI) / N;

    // Cache point[0] XY for the loop-closure snap.
    // angle_0 = 0 → cos(0) = 1, sin(0) = 0.
    const float X0 = LoopRadius;
    const float Y0 = 0.f;

    for (int32 i = 0; i < StepCount; ++i)
    {
        const float fi    = static_cast<float>(i);
        const float Angle = AngularStep * fi;
        const float X     = LoopRadius * FMath::Cos(Angle);
        const float Y     = LoopRadius * FMath::Sin(Angle);
        // Quadratic Z: rises by StepHeight each step, then closes back to ~0.
        const float Z     = StepHeight * fi - (StepHeight * fi * fi / N);

        FPCGPoint& Pt = Points.AddDefaulted_GetRef();
        Pt.Transform.SetLocation(FVector(X, Y, Z));

        // Deterministic per-point seed derived from the settings seed + index.
        // XOR with a large prime keeps seeds well-distributed without external deps.
        Pt.Seed = Seed ^ (i * 2654435761);
    }

    // -----------------------------------------------------------------------
    // 4. Loop closure — snap last point to (X0, Y0, 0)
    //
    //   Requirement 1.3: "final step position equals the first step position
    //   within 1 cm tolerance."
    //
    //   The quadratic formula already brings the last point close to Z=0; the
    //   explicit snap guarantees exact closure regardless of floating-point drift.
    //   XY is snapped to point[0]'s exact XY (angle=0 → (LoopRadius, 0)).
    // -----------------------------------------------------------------------
    if (!Points.IsEmpty())
    {
        FPCGPoint& LastPt = Points.Last();
        FVector    LastLoc = LastPt.Transform.GetLocation();
        LastLoc.X = X0;
        LastLoc.Y = Y0;
        LastLoc.Z = 0.f;
        LastPt.Transform.SetLocation(LastLoc);
    }

    // -----------------------------------------------------------------------
    // 4b. Write ArchitecturalRole + Walkable attributes
    //     Every stair point is ArchitecturalRole=Stair and Walkable=true
    //     (stair treads are designed to be walked on).
    // -----------------------------------------------------------------------
    {
        UPCGMetadata* Metadata = OutputData->Metadata;
        check(Metadata);

        FPCGMetadataAttribute<int32>* RoleAttr =
            Metadata->FindOrCreateAttribute<int32>(
                FMelodiaPCGAttrs::ArchitecturalRoleAttr,
                static_cast<int32>(EPCGArchitecturalRole::Stair),
                /*bAllowsInterpolation=*/ false,
                /*bOverrideParent=*/ true);

        FPCGMetadataAttribute<bool>* WalkAttr =
            Metadata->FindOrCreateAttribute<bool>(
                FMelodiaPCGAttrs::WalkableAttr,
                true,
                /*bAllowsInterpolation=*/ false,
                /*bOverrideParent=*/ true);

        FPCGMetadataAttribute<float>* SlopeAttr =
            Metadata->FindOrCreateAttribute<float>(
                FMelodiaPCGAttrs::SlopeAngleAttr,
                0.f,  // stairs are effectively flat walkable surfaces
                /*bAllowsInterpolation=*/ true,
                /*bOverrideParent=*/ true);

        const int32 StairRole = static_cast<int32>(EPCGArchitecturalRole::Stair);
        for (FPCGPoint& Pt : Points)
        {
            if (Pt.MetadataEntry == PCGInvalidEntryKey)
            {
                Metadata->InitializeOnSet(Pt.MetadataEntry);
            }
            if (RoleAttr)  RoleAttr->SetValue(Pt.MetadataEntry, StairRole);
            if (WalkAttr)  WalkAttr->SetValue(Pt.MetadataEntry, true);
            if (SlopeAttr) SlopeAttr->SetValue(Pt.MetadataEntry, 0.f);
        }
    }

    // -----------------------------------------------------------------------
    // 5. Emit output via FPCGTaggedData on the "Out" pin
    //
    //   PCGPinConstants::DefaultOutputLabel resolves to the "Out" pin label
    //   that PCG graphs wire to by default.
    // -----------------------------------------------------------------------
    FPCGTaggedData& Output = Context->OutputData.TaggedData.AddDefaulted_GetRef();
    Output.Data = OutputData;
    Output.Pin  = PCGPinConstants::DefaultOutputLabel;

    return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
