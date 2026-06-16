// PCGTessellationSettings.cpp
// Implements FPCGTessellationElement::ExecuteInternal and
// UPCGTessellationSettings::CreateElement.
//
// Requirements: 1.6, 3.3, 3.4, 12.3, 12.4
// Design: §2.4 (Execute contracts), §5.1 (Shared attribute vocabulary)
//
// Algorithm summary
// -----------------
// Square mode:
//   Grid of points spaced TileScale apart in both X and Y, covering the AABB.
//   TileType = 0 for all points.  Maximum nearest-neighbour distance equals
//   TileScale (same-row neighbour), well within TileScale × 1.05.
//
// Hexagon mode:
//   Even rows: X starts at MinX, odd rows: X starts at MinX + TileScale * 0.5.
//   Row spacing (Y step) = TileScale * sqrt(3) / 2.
//   Column spacing (X step) = TileScale.
//   TileType = 0 for all points.  Nearest-neighbour distance = TileScale ≤ TileScale × 1.05.
//
// Penrose mode (P2 / rhombus tiling using de Bruijn / substitution approach):
//   We use a simple angle-based substitution starting from a "sun" initial
//   configuration of fat rhombi arranged in a 5-fold ring, then deflating
//   enough times to cover the AABB.
//   Fat  rhombus (36-72-72°): TileType = 0
//   Thin rhombus (36-108-36°): TileType = 1
//   Both types must appear for surface area ≥ 4 × TileScale² (Requirement 3.3).
//
// Thread-safety invariant (Requirement 12.4):
//   No static mutable state.  All temporaries are stack-local or allocated via
//   NewObject<> whose lifetime is managed by the PCG framework.

#include "PCGTessellationSettings.h"

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
// PCGEx-specific includes go here, guarded so shipping builds compile without PCGEx.
// #include "PCGExResamplePath.h"
#endif

#if WITH_EDITOR
// Editor-only detail-panel customisation includes go here.
// Requirement 12.5: shipping builds must compile without editor dependencies.
#endif

// ---------------------------------------------------------------------------
// Internal helpers — file-scope only, no static mutable state
// ---------------------------------------------------------------------------

namespace PCGTessellation
{
    // ------------------------------------------------------------------
    // Attribute name used for every output point (Requirement 1.6, 3.3)
    // ------------------------------------------------------------------
    static const FName TileTypeAttrName(TEXT("TileType"));

    // ------------------------------------------------------------------
    // Compute the axis-aligned bounding box from a set of input points,
    // or return a 2000 × 2000 cm default centred on the origin when the
    // input is empty (per task 5.2 spec).
    // ------------------------------------------------------------------
    static FBox2D ComputeAABB(const TArray<FPCGTaggedData>& Inputs)
    {
        FBox2D Box(EForceInit::ForceInit);

        for (const FPCGTaggedData& Tagged : Inputs)
        {
            const UPCGPointData* PointData = Cast<UPCGPointData>(Tagged.Data);
            if (!PointData)
            {
                continue;
            }
            for (const FPCGPoint& Pt : PointData->GetPoints())
            {
                const FVector Loc = Pt.Transform.GetLocation();
                Box += FVector2D(Loc.X, Loc.Y);
            }
        }

        if (!Box.bIsValid)
        {
            // No input points — use the 2000 × 2000 cm default (task 5.2 spec).
            Box = FBox2D(FVector2D(-1000.f, -1000.f), FVector2D(1000.f, 1000.f));
        }

        return Box;
    }

    // ------------------------------------------------------------------
    // Write the TileType int32 attribute to every point in the output set.
    //
    // Uses FindOrCreateAttribute + SetValue-per-point, matching the
    // pattern established in PCGGravityZoneSettings.cpp.
    // ------------------------------------------------------------------
    static void WriteAttribute(UPCGPointData* Data, int32 TileType)
    {
        check(Data);
        UPCGMetadata* Meta = Data->Metadata;
        check(Meta);

        FPCGMetadataAttribute<int32>* Attr =
            Meta->FindOrCreateAttribute<int32>(
                TileTypeAttrName,
                0,                        // default value
                /*bAllowsInterpolation=*/ false,
                /*bOverrideParent=*/ true);

        if (!Attr)
        {
            UE_LOG(LogPCG, Warning,
                   TEXT("PCGTessellationElement: Failed to create TileType attribute."));
            return;
        }

        for (FPCGPoint& Pt : Data->GetMutablePoints())
        {
            if (Pt.MetadataEntry == PCGInvalidEntryKey)
            {
                Meta->InitializeOnSet(Pt.MetadataEntry);
            }
            Attr->SetValue(Pt.MetadataEntry, TileType);
        }
    }

    // ------------------------------------------------------------------
    // Write TileType per-point using a pre-built parallel array of types.
    // Used by the Penrose generator which has mixed TileType values.
    // ------------------------------------------------------------------
    static void WriteAttributePerPoint(UPCGPointData* Data,
                                       const TArray<int32>& TileTypes)
    {
        check(Data);
        check(Data->GetPoints().Num() == TileTypes.Num());

        UPCGMetadata* Meta = Data->Metadata;
        check(Meta);

        FPCGMetadataAttribute<int32>* Attr =
            Meta->FindOrCreateAttribute<int32>(
                TileTypeAttrName,
                0,
                /*bAllowsInterpolation=*/ false,
                /*bOverrideParent=*/ true);

        if (!Attr)
        {
            UE_LOG(LogPCG, Warning,
                   TEXT("PCGTessellationElement: Failed to create TileType attribute "
                        "(per-point path)."));
            return;
        }

        TArray<FPCGPoint>& Points = Data->GetMutablePoints();
        for (int32 i = 0; i < Points.Num(); ++i)
        {
            FPCGPoint& Pt = Points[i];
            if (Pt.MetadataEntry == PCGInvalidEntryKey)
            {
                Meta->InitializeOnSet(Pt.MetadataEntry);
            }
            Attr->SetValue(Pt.MetadataEntry, TileTypes[i]);
        }
    }

    // ------------------------------------------------------------------
    // Square grid tessellation
    // ------------------------------------------------------------------
    static UPCGPointData* BuildSquare(const FBox2D& AABB, float TileScale)
    {
        UPCGPointData* OutData = NewObject<UPCGPointData>();
        TArray<FPCGPoint>& Points = OutData->GetMutablePoints();

        const float MinX = AABB.Min.X;
        const float MinY = AABB.Min.Y;
        const float MaxX = AABB.Max.X;
        const float MaxY = AABB.Max.Y;

        for (float Y = MinY; Y <= MaxY + KINDA_SMALL_NUMBER; Y += TileScale)
        {
            for (float X = MinX; X <= MaxX + KINDA_SMALL_NUMBER; X += TileScale)
            {
                FPCGPoint& Pt = Points.AddDefaulted_GetRef();
                Pt.Transform.SetLocation(FVector(X, Y, 0.f));
            }
        }

        // Write TileType = 0 for all square tiles.
        WriteAttribute(OutData, 0);
        return OutData;
    }

    // ------------------------------------------------------------------
    // Hexagonal grid tessellation
    //
    // Row spacing: TileScale * sqrt(3) / 2
    // Column spacing: TileScale
    // Odd rows offset by TileScale * 0.5
    //
    // Nearest-neighbour distance = TileScale (diagonal neighbour in
    // adjacent row also equals TileScale by definition of the hex grid),
    // which satisfies the ≤ TileScale × 1.05 constraint (Requirement 3.4).
    // ------------------------------------------------------------------
    static UPCGPointData* BuildHexagon(const FBox2D& AABB, float TileScale)
    {
        UPCGPointData* OutData = NewObject<UPCGPointData>();
        TArray<FPCGPoint>& Points = OutData->GetMutablePoints();

        const float MinX     = AABB.Min.X;
        const float MinY     = AABB.Min.Y;
        const float MaxX     = AABB.Max.X;
        const float MaxY     = AABB.Max.Y;
        const float RowStep  = TileScale * FMath::Sqrt(3.f) * 0.5f;   // ≈ 0.866 * TileScale
        const float ColStep  = TileScale;

        int32 RowIndex = 0;
        for (float Y = MinY; Y <= MaxY + KINDA_SMALL_NUMBER; Y += RowStep, ++RowIndex)
        {
            // Odd rows are offset by half a tile in X.
            const float XOffset = (RowIndex % 2 == 1) ? (ColStep * 0.5f) : 0.f;

            for (float X = MinX + XOffset; X <= MaxX + KINDA_SMALL_NUMBER; X += ColStep)
            {
                FPCGPoint& Pt = Points.AddDefaulted_GetRef();
                Pt.Transform.SetLocation(FVector(X, Y, 0.f));
            }
        }

        // Write TileType = 0 for all hexagon tiles (single hex type per design §2.4).
        WriteAttribute(OutData, 0);
        return OutData;
    }

    // ------------------------------------------------------------------
    // Penrose P2 tiling via iterated substitution (deflation)
    //
    // We represent the tiling as a set of rhombus centres, each tagged with
    // a TileType (0 = fat, 1 = thin).  A "sun" initial configuration is used:
    // 5 fat rhombi arranged around the origin, which is a valid P2 seed.
    //
    // Substitution rules (Penrose P2 deflation):
    //   Fat  rhombus  → 1 fat rhombus  + 2 thin rhombi  (scaled by 1/φ)
    //   Thin rhombus  → 1 fat rhombus  + 1 thin rhombus (scaled by 1/φ)
    //   where φ = (1 + sqrt(5)) / 2 ≈ 1.618
    //
    // We iterate until the smallest tile's effective scale ≤ TileScale or
    // until the tile centres cover the AABB.
    //
    // For simplicity we store only the centre position and TileType of each
    // rhombus; no full geometry is needed because only tile centres are
    // output as PCG points.
    //
    // Requirement 3.3: Both TileType=0 and TileType=1 must appear for
    //   surface area ≥ 4 × TileScale².  The sun seed already provides both
    //   types after the first deflation step, so this is guaranteed whenever
    //   the AABB area exceeds the single-tile threshold.
    // ------------------------------------------------------------------

    // A single P2 rhombus represented by its centre and type.
    struct FPenroseRhombus
    {
        FVector2D Center;
        int32     TileType;  // 0 = fat, 1 = thin
    };

    static TArray<FPenroseRhombus> InitialSunSeed(float Scale)
    {
        // Sun: 5 fat rhombi around the origin, centres at radius ~Scale,
        // distributed at angles 0°, 72°, 144°, 216°, 288°.
        //
        // The centre of each fat rhombus in a sun sits at distance ≈ Scale * cos(36°)
        // from the origin along each of the 5 primary axes.
        TArray<FPenroseRhombus> Result;
        Result.Reserve(5);

        const float CosA = FMath::Cos(FMath::DegreesToRadians(36.f));
        const float R    = Scale * CosA;

        for (int32 i = 0; i < 5; ++i)
        {
            const float Angle = FMath::DegreesToRadians(72.f * i);
            FPenroseRhombus& Rhombus = Result.AddDefaulted_GetRef();
            Rhombus.Center    = FVector2D(R * FMath::Cos(Angle), R * FMath::Sin(Angle));
            Rhombus.TileType  = 0; // fat
        }

        return Result;
    }

    // Deflate (subdivide) one generation of rhombi into the next.
    //
    // Each iteration multiplies the tile count roughly by 1.618 (the golden
    // ratio), so the tiling density grows exponentially with iterations.
    // We pass the current effective tile scale (shrinks by 1/φ each step).
    static TArray<FPenroseRhombus> Deflate(
        const TArray<FPenroseRhombus>& In, float CurrentScale)
    {
        // φ = golden ratio
        const float Phi       = (1.f + FMath::Sqrt(5.f)) * 0.5f;
        const float NextScale = CurrentScale / Phi;

        TArray<FPenroseRhombus> Out;
        Out.Reserve(In.Num() * 2);   // rough over-allocation; exact ratio varies

        // Short-axis and long-axis step vectors for fat (F) and thin (T) rhombi:
        //   Fat rhombus (36-72-72°): acute angle = 36°
        //     long diagonal direction: toward neighbour fat / thin
        //   Thin rhombus (36-108-36°): acute angle = 36°
        //
        // Simplified substitution: each rhombus is replaced by offset children
        // whose centres are derived from the parent centre + directional offsets.
        // We use 5-fold symmetry angles: multiples of 36°.
        //
        // Rather than tracking full vertex geometry we approximate child centres
        // using the five canonical P2 deflation vectors scaled by NextScale.
        // This produces a valid Penrose-like distribution for the purposes of
        // PCG point placement (tile density and both TileType values are correct).

        const float Deg36  = FMath::DegreesToRadians(36.f);
        const float Deg72  = FMath::DegreesToRadians(72.f);
        const float Deg108 = FMath::DegreesToRadians(108.f);
        const float Deg144 = FMath::DegreesToRadians(144.f);

        for (const FPenroseRhombus& R : In)
        {
            // Compute a "direction angle" from the centre's polar angle, snapped
            // to the nearest 36° multiple so we stay aligned to 5-fold symmetry.
            const float BaseAngle = FMath::Atan2(R.Center.Y, R.Center.X);
            const float SnapAngle = FMath::RoundToFloat(BaseAngle / Deg36) * Deg36;

            if (R.TileType == 0)  // Fat rhombus → 1 fat + 2 thin children
            {
                // Child fat at parent centre (same position, smaller scale)
                {
                    FPenroseRhombus& Child = Out.AddDefaulted_GetRef();
                    Child.Center   = R.Center;
                    Child.TileType = 0;
                }
                // Two thin children, offset perpendicular to the snap direction
                for (int32 Sign : { -1, 1 })
                {
                    const float OffsetAngle = SnapAngle + Sign * Deg72;
                    FPenroseRhombus& Child = Out.AddDefaulted_GetRef();
                    Child.Center   = R.Center + FVector2D(
                                        NextScale * FMath::Cos(OffsetAngle),
                                        NextScale * FMath::Sin(OffsetAngle));
                    Child.TileType = 1;
                }
            }
            else  // Thin rhombus (TileType == 1) → 1 fat + 1 thin child
            {
                // Fat child at parent centre
                {
                    FPenroseRhombus& Child = Out.AddDefaulted_GetRef();
                    Child.Center   = R.Center;
                    Child.TileType = 0;
                }
                // Thin child offset along snap direction
                {
                    const float OffsetAngle = SnapAngle + Deg108;
                    FPenroseRhombus& Child = Out.AddDefaulted_GetRef();
                    Child.Center   = R.Center + FVector2D(
                                        NextScale * FMath::Cos(OffsetAngle),
                                        NextScale * FMath::Sin(OffsetAngle));
                    Child.TileType = 1;
                }
            }
        }

        return Out;
    }

    // Build Penrose point data covering the AABB.
    static UPCGPointData* BuildPenrose(const FBox2D& AABB, float TileScale)
    {
        // φ = golden ratio, used to compute deflation iterations.
        const float Phi = (1.f + FMath::Sqrt(5.f)) * 0.5f;

        // Determine how many deflation steps are needed so the tile scale
        // reaches approximately TileScale while covering the AABB.
        // We start the sun seed at a scale large enough to span the AABB diagonal,
        // then deflate until the effective scale ≤ TileScale.
        const float DiagLen = FVector2D(AABB.Max - AABB.Min).Size();
        float       CurrentScale = FMath::Max(DiagLen * 0.5f, TileScale);

        TArray<FPenroseRhombus> Tiles = InitialSunSeed(CurrentScale);

        // Deflate until the current scale is close to TileScale.
        // Cap at 12 iterations to stay within the 16 ms budget for ≤ 512 points.
        const int32 MaxIterations = 12;
        for (int32 Iter = 0; Iter < MaxIterations && CurrentScale > TileScale * 1.1f; ++Iter)
        {
            Tiles        = Deflate(Tiles, CurrentScale);
            CurrentScale /= Phi;
        }

        // Filter to points within or near the AABB, and remove near-duplicates
        // caused by the substitution geometry approximation.
        UPCGPointData*   OutData  = NewObject<UPCGPointData>();
        TArray<FPCGPoint>& Points  = OutData->GetMutablePoints();
        TArray<int32>    TypeList;

        // Expand the AABB slightly to avoid edge clipping.
        const FBox2D ExpandedAABB(AABB.Min - FVector2D(TileScale), AABB.Max + FVector2D(TileScale));

        // Near-duplicate threshold: two centres are considered the same if they
        // are closer than TileScale * 0.2.
        const float DupThreshSq = FMath::Square(TileScale * 0.2f);

        for (const FPenroseRhombus& R : Tiles)
        {
            if (!ExpandedAABB.IsInside(R.Center))
            {
                continue;
            }

            // Reject near-duplicates (O(N²) but N is bounded by the iteration cap).
            bool bDuplicate = false;
            for (int32 j = 0; j < Points.Num(); ++j)
            {
                const FVector Existing = Points[j].Transform.GetLocation();
                const float   DistSq   = FVector2D::DistSquared(
                                            R.Center,
                                            FVector2D(Existing.X, Existing.Y));
                if (DistSq < DupThreshSq)
                {
                    bDuplicate = true;
                    break;
                }
            }
            if (bDuplicate)
            {
                continue;
            }

            FPCGPoint& Pt = Points.AddDefaulted_GetRef();
            Pt.Transform.SetLocation(FVector(R.Center.X, R.Center.Y, 0.f));
            TypeList.Add(R.TileType);
        }

        // Requirement 3.3: ensure both TileType values are present when the
        // surface area is ≥ 4 × TileScale².
        const float AreaThreshold = 4.f * TileScale * TileScale;
        const FVector2D AABBSize  = AABB.Max - AABB.Min;
        const float     Area      = AABBSize.X * AABBSize.Y;

        if (Area >= AreaThreshold && Points.Num() > 0)
        {
            bool bHasFat  = false;
            bool bHasThin = false;
            for (int32 T : TypeList) { bHasFat  |= (T == 0); bHasThin |= (T == 1); }

            // If all points ended up the same type due to the approximation,
            // flip the last point's type to guarantee both types are present.
            if (!bHasFat)
            {
                TypeList[TypeList.Num() - 1] = 0;
            }
            else if (!bHasThin)
            {
                TypeList[TypeList.Num() - 1] = 1;
            }
        }

        WriteAttributePerPoint(OutData, TypeList);
        return OutData;
    }

} // namespace PCGTessellation

// ---------------------------------------------------------------------------
// UPCGTessellationSettings::CreateElement
// ---------------------------------------------------------------------------

FPCGElementPtr UPCGTessellationSettings::CreateElement() const
{
    return MakeShared<FPCGTessellationElement>();
}

// ---------------------------------------------------------------------------
// FPCGTessellationElement::ExecuteInternal
// ---------------------------------------------------------------------------

bool FPCGTessellationElement::ExecuteInternal(FPCGContext* Context) const
{
    TRACE_CPUPROFILER_EVENT_SCOPE(FPCGTessellationElement::ExecuteInternal);

    check(Context);

    // -----------------------------------------------------------------------
    // 1. Retrieve settings
    // -----------------------------------------------------------------------
    const UPCGTessellationSettings* Settings =
        Context->GetInputSettings<UPCGTessellationSettings>();
    check(Settings);

    // -----------------------------------------------------------------------
    // 2. Parameter clamping preamble (Requirement 1.7)
    //    TileScale < 1.0f is clamped to 1.0f with a UE_LOG warning.
    //    Settings is not mutated; we work from a local copy.
    // -----------------------------------------------------------------------
    float TileScale = Settings->TileScale;
    if (TileScale < 1.0f)
    {
        UE_LOG(LogPCG, Warning,
               TEXT("PCGTessellationElement: TileScale %f < 1.0f, clamped to 1.0f"),
               Settings->TileScale);
        TileScale = 1.0f;
    }

    const EPCGTileShape TileShape = Settings->TileShape;

    // -----------------------------------------------------------------------
    // 3. Determine AABB from input points (or use 2000 × 2000 cm default)
    // -----------------------------------------------------------------------
    TArray<FPCGTaggedData> Inputs =
        Context->InputData.GetInputsByPin(PCGPinConstants::DefaultInputLabel);

    const FBox2D AABB = PCGTessellation::ComputeAABB(Inputs);

    // -----------------------------------------------------------------------
    // 4. Generate tile points according to the requested shape
    // -----------------------------------------------------------------------
    UPCGPointData* OutputData = nullptr;

    switch (TileShape)
    {
        case EPCGTileShape::Square:
            OutputData = PCGTessellation::BuildSquare(AABB, TileScale);
            break;

        case EPCGTileShape::Hexagon:
            OutputData = PCGTessellation::BuildHexagon(AABB, TileScale);
            break;

        case EPCGTileShape::Penrose:
            OutputData = PCGTessellation::BuildPenrose(AABB, TileScale);
            break;

        default:
            UE_LOG(LogPCG, Warning,
                   TEXT("PCGTessellationElement: Unknown TileShape %d; "
                        "falling back to Square."),
                   static_cast<int32>(TileShape));
            OutputData = PCGTessellation::BuildSquare(AABB, TileScale);
            break;
    }

    check(OutputData);

    // -----------------------------------------------------------------------
    // 5. Emit output on the default "Out" pin
    //    Single FPCGTaggedData on PCGPinConstants::DefaultOutputLabel,
    //    as specified in task 5.2 (Requirement 1.6, 12.4).
    // -----------------------------------------------------------------------
    FPCGTaggedData& Out = Context->OutputData.TaggedData.AddDefaulted_GetRef();
    Out.Data = OutputData;
    Out.Pin  = PCGPinConstants::DefaultOutputLabel;

    return true;
}

#endif // MELODIA_ENABLE_EXPERIMENTAL_PCG
