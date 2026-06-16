# Requirements Document

## Introduction

The **Escher PCG Library** is a C++ and Blueprint PCG module library for Unreal Engine 5,
built inside the `MelodiaMelusina_PROD` project (`G:\Melodia`). It extends the existing
baroque/surreal PCG graph family — which already includes `PCG_PenroseShrine`,
`PCG_FloatingStairways`, `PCG_BridgeArchipelago`, `PCG_EscherDecks`, and `PCG_DreamWalls` —
with a systematic, reusable, composable set of advanced procedural generation systems
inspired by two aesthetic references:

- **M.C. Escher**: impossible geometries, Penrose/recursive staircases, interlocking
  tessellations, gravity-defying architecture, spatial paradoxes, self-similar recursion.
- **Infinity Nikki**: whimsical open-world, floating islands, magical gardens, organic
  flowing architecture, vine-draped ruins, luminescent flora.

The library runs on the UE5 PCG Framework, leverages PCGExtendedToolkit v0.75.20 already
installed at `Plugins/PCGExtendedToolkit`, and integrates into the existing
`MelodiaMelusina_PROD` C++ module with canonical project constants (`ModuleWidth=400 cm`,
`StoryHeight=600 cm`, golden-ratio shrink `0.618×`, Melusina baroque mesh palette,
MooaToon SDF/oil-paint materials).

The library is organized into five thematic families:

1. **Impossible Architecture** — Penrose staircases, self-connecting bridges, twisted corridors
2. **Recursive / Fractal Structures** — nested arches, fractal buttresses, tessellations
3. **Floating / Gravity-Defying Layouts** — inverted towers, suspended walkways, island clusters
4. **Organic Flowing Environments** — vine overgrowth, magical gardens, spiral terraces
5. **Stylized Fantasy Decoration** — lanterns, floral borders, particle anchors, prop scatter

Each family provides reusable PCG subgraphs that compose into top-level PCG graphs, backed by
C++ extension points (custom PCG nodes, Blueprint-callable math utilities), and validated by
dedicated test levels. This spec is **additive**: no existing PCG graph asset, test level,
or mesh collection is modified or deleted.


## Glossary

- **PCG_Library**: The complete set of PCG graphs, subgraphs, C++ nodes, Blueprint utilities,
  mesh collections, and test levels delivered by this feature.
- **PCG_Graph**: An Unreal Engine PCG graph asset (`PCG_` prefix) that procedurally generates
  point sets or spawns meshes.
- **PCG_Subgraph**: A reusable PCG graph asset (`PCG_Sub_` prefix) wired as a subgraph node
  inside top-level graphs.
- **Custom_PCG_Element**: A C++ class derived from `UPCGSettings` / `FPCGElement` inside the
  `MelodiaMelusina_PROD` module under `Source/MelodiaMelusina_PROD/PCG/`.
- **PCG_Volume**: A PCG Volume Actor placed in a level that drives a PCG_Graph via its input
  surface and parameters.
- **Graph_Parameter**: A named, type-safe pin override on a PCG_Graph settable per PCG_Volume
  instance without opening the graph.
- **MeshCollection**: A PCGEx Mesh Collection asset (`PCGCol_` prefix) holding a weighted list
  of Static Meshes selected by tags.
- **ModuleWidth**: Canonical wall/column spacing constant, 400 cm, shared across all graphs.
- **StoryHeight**: Canonical floor-to-cornice height, 600 cm, shared across all graphs.
- **GoldenRatio**: The value 0.618, used as the recursive scale factor for shrink systems.
- **PenroseLoop**: A staircase topology where the last step's top returns to the same world
  height as the first step's bottom, forming a closed loop.
- **GravityZone**: A PCG-tagged region where the up-direction is remapped (inverted, sideways)
  to support impossible-gravity architectural layouts.
- **SplinePath**: A Spline Component in a level actor used as PCG input for path-following,
  vine/tendril placement, and bridge generation.
- **PointAttribute**: A named metadata value attached to a PCG point (e.g. `ModuleIndex`,
  `ArchitecturalRole`, `Walkable`, `RecursionDepth`).
- **RecursionDepth**: A Graph_Parameter (int32, clamped 1–5) controlling nested iterations.
- **WalkabilityCheck**: The reusable `PCG_Sub_WalkabilityCheck` subgraph that validates floor
  points against step-height ≤ 50 cm and path-width ≥ 200 cm.
- **MeluPalette**: The Melusina baroque mesh palette: `SM_wallhi_*`, `SM_wallmid_*`,
  `SM_wallshort_*`, `SM_SpiralStair001`, `SM_venetianbridge`, `SM_PENROSE*`,
  `SM_surrealtower*`, with MooaToon SDF/oil-paint materials.
- **SeedParameter**: The `Seed` Graph_Parameter (int32, default 0) for deterministic variation.
- **EMelodiaTilePattern**: C++ enum with values `Triangular`, `Hexagonal`, `PenroseDiamond`.
- **EMelodiaStylePreset**: C++ enum with values `Baroque`, `EscherSurreal`, `NikkiWhimsical`,
  `NikkiRuins`.
- **Builder_Script**: `Scripts/PCG/escher_pcg_builder.py`, the Python UE automation script
  that creates EscherNikki graph assets in-editor.
- **TestLevel**: A dedicated UE5 level (`L_PCGTest_` prefix) pre-wired to one top-level graph.


## Requirements

### Requirement 1: C++ Custom PCG Element Infrastructure

**User Story:** As a C++ programmer on Melodia, I want four custom PCG nodes implementing
impossible-geometry primitives, so that graph designers can access math-heavy operations
that vanilla PCG and PCGEx nodes cannot express.

#### Acceptance Criteria

1. THE `MelodiaMelusina_PROD` module SHALL declare a `PCG` subdirectory under
   `Source/MelodiaMelusina_PROD/PCG/` containing all Custom_PCG_Element header and source files.

2. THE `MelodiaMelusina_PROD.Build.cs` SHALL add `"PCG"` and `"GeometryScripting"` to
   `PublicDependencyModuleNames`; IF any Custom_PCG_Element directly includes PCGEx headers,
   THEN `"PCGExtendedToolkit"` SHALL also be added to `PrivateDependencyModuleNames`.

3. THE `PCGEscherStaircaseSettings` Custom_PCG_Element SHALL accept `StepCount` (int32,
   default 16), `StepHeight` (float, default 25 cm), `StepWidth` (float, default 150 cm),
   `LoopRadius` (float, default 600 cm), and `Seed` (int32, default 0), and SHALL output a
   PCG point set forming a closed PenroseLoop whose final step's Z-position plus one
   `StepHeight` equals the first step's Z-position within 1 cm tolerance.

4. THE `PCGGravityZoneSettings` Custom_PCG_Element SHALL accept an input point set and a
   `GravityDir` FVector parameter, and SHALL write a `GravityDir` FVector attribute to every
   output point so downstream spawner nodes can orient meshes to the overridden gravity axis.

5. THE `PCGRecursiveArchSettings` Custom_PCG_Element SHALL accept `ArchWidth` (float, default
   400 cm), `ArchHeight` (float, default 600 cm), `RecursionDepth` (int32, 1–4), and
   `ScaleFactor` (float, default 0.618), and SHALL output point sets for each arch tier where
   each tier's width equals the parent tier's width multiplied by `ScaleFactor` within 2%
   tolerance.

6. THE `PCGTessellationSettings` Custom_PCG_Element SHALL accept a surface input and a
   `TileShape` enum of type `EMelodiaTilePattern`, and SHALL output a point set covering the
   surface with zero-gap tile placement where each tile point carries `TileType` (int32) and
   `TileIndex` (int32) attributes.

7. WHEN any Custom_PCG_Element receives an out-of-range parameter (e.g. `RecursionDepth`
   outside 1–4, `StepCount` less than 3), THEN THE Custom_PCG_Element SHALL clamp the value
   to the nearest valid bound and call `UE_LOG(LogPCG, Warning, ...)` describing the adjustment.

8. THE Custom_PCG_Element classes SHALL be thread-safe: their `Execute` functions SHALL NOT
   write to shared mutable state outside the `FPCGTaggedData` output they return, and SHALL
   NOT call `DrawDebug*` functions in non-editor builds.

