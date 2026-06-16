# Requirements Document

## Introduction

The PCG Escher–Nikki Library is a C++ and Blueprint PCG module library for Unreal Engine 5 within the Melodia Melusina project (`G:\Melodia`). It extends the existing baroque/surreal PCG graph family — which already includes `PCG_PenroseShrine`, `PCG_FloatingStairways`, `PCG_BridgeArchipelago`, and `PCG_DreamWalls` — with a systematic, reusable, composable set of advanced procedural generation systems inspired by two aesthetic references: **Infinity Nikki** (whimsical open-world, floating islands, magical gardens, organic flowing architecture) and **M.C. Escher** (impossible geometries, Penrose/recursive staircases, interlocking tessellations, spatial paradoxes).

The library runs on the UE5 PCG Framework (not Houdini), leverages PCGExtendedToolkit v0.75.20 already installed at `Plugins/PCGExtendedToolkit`, and integrates into the existing `MelodiaMelusina_PROD` C++ module and project conventions (`ModuleWidth=400cm`, `StoryHeight=600cm`, golden-ratio shrink `0.618×`, Melusina baroque mesh palette, MooaToon SDF/oil-paint materials).

The library is organized into five thematic families: **Impossible Architecture**, **Recursive / Fractal Structures**, **Floating / Gravity-Defying Layouts**, **Organic Flowing Environments**, and **Stylized Fantasy Decoration**. Each family contains reusable PCG subgraphs that compose into top-level PCG graphs, backed by C++ extension points (custom PCG nodes, data types, Blueprint-callable utilities), and validated by a dedicated test level.


## Glossary

- **PCG_Library**: The complete set of PCG graphs, subgraphs, C++ nodes, Blueprint utilities, mesh collections, and test levels delivered by this feature.
- **PCG_Graph**: An Unreal Engine PCG graph asset (`PCG_` prefix) that procedurally generates point sets or spawns meshes.
- **PCG_Subgraph**: A reusable PCG graph asset (`PCG_Sub_` prefix) wired as a subgraph node inside top-level graphs.
- **PCG_Node**: A single node inside a PCG graph, either a built-in UE5 PCG node, a PCGEx node, or a custom C++ node from this library.
- **PCG_Volume**: A PCG Volume Actor placed in a level that drives a PCG_Graph via its input surface and parameters.
- **Graph_Parameter**: A named, type-safe pin override on a PCG_Graph that can be set per PCG_Volume instance without opening the graph.
- **MeshCollection**: A PCGEx Mesh Collection asset (`PCGCol_` prefix) that holds a weighted list of Static Meshes selected by tags.
- **ModuleWidth**: Canonical wall/column spacing constant, 400 cm, shared across all architecture PCG graphs.
- **StoryHeight**: Canonical floor-to-cornice height, 600 cm, shared across all architecture PCG graphs.
- **GoldenRatio**: The value 0.618, used as the recursive scale factor for Escher shrink systems.
- **PenroseLoop**: A staircase topology where the last step returns to the same world height as the first step, creating a closed loop.
- **GravityZone**: A PCG-tagged region where up-direction is remapped (e.g., inverted, sideways) to support floating or inverted architectural layouts.
- **SplinePath**: A Spline Component in a level actor, used as PCG input for path-following, vine/tendril placement, and road/bridge generation.
- **PointAttribute**: A named metadata value attached to a PCG point (e.g., `ModuleIndex`, `StoryIndex`, `ArchitecturalRole`, `Walkable`, `RecursionDepth`).
- **EscherFamily**: The five existing surreal graphs (`PCG_PenroseShrine`, `PCG_EscherDecks`, `PCG_DreamWalls`, `PCG_BridgeArchipelago`, `PCG_FloatingStairways`) plus all new `PCG_*` and `PCG_Sub_*` graphs added by this library.
- **NikkiFamily**: The nature/garden/landscape graphs (`PCG_MelodiaForest`, `PCG_TerraceGarden`, `PCG_WallGardenPath`, etc.) plus all new organic/floating/decoration graphs added by this library.
- **PCGEx**: The PCGExtendedToolkit plugin (v0.75.20) installed at `Plugins/PCGExtendedToolkit`, providing nodes such as Create Shapes, Resample Path, Offset Path, Pathfinding, Clusters, Flood Fill, Tensors, and Uber Filter.
- **C++_PCGNode**: A custom C++ class derived from `UPCGSettings` / `UPCGNode` inside the `MelodiaMelusina_PROD` module, Blueprint-callable and usable inside PCG graphs.
- **TestLevel**: A dedicated UE5 level (`L_PCGTest_` prefix) that contains a PCG_Volume pre-wired to one top-level graph for isolated verification.
- **RecursionDepth**: A Graph_Parameter that controls how many nested iterations a recursive/fractal graph executes (integer, clamped 1–5).
- **WalkabilityCheck**: The reusable `PCG_Sub_WalkabilityCheck` subgraph that validates floor points against step-height ≤ 50 cm and path-width ≥ 200 cm constraints.
- **MeluPalette**: The Melusina baroque mesh and material palette: `SM_wallhi_*`, `SM_wallmid_*`, `SM_wallshort_*`, `SM_SpiralStair001`, `SM_venetianbridge`, `SM_PENROSE`, `SM_surrealtower*`, MooaToon SDF/oil-paint materials.
- **SeedParameter**: The `Seed` Graph_Parameter (int32, default 0) used for deterministic variation across all PCG graphs in the library.


## Requirements

### Requirement 1: Impossible Architecture Family

**User Story:** As a level designer on Melodia Melusina, I want a set of PCG graphs that generate impossible and non-Euclidean architectural forms, so that I can place Escher-style spatial paradoxes — looping staircases, self-connecting bridges, and Penrose-loop corridors — in game levels without hand-placing every module.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a top-level graph `PCG_PenroseStaircaseEx` that accepts a center-point input and a `LoopRadius` Graph_Parameter (float, 200–2000 cm) and generates a closed staircase ring where the last step's top surface aligns within 1 cm of the first step's bottom surface in world space.

2. WHEN `PCG_PenroseStaircaseEx` is executed with any `LoopRadius` value in the valid range and any integer `StepCount` value between 4 and 64, THE PCG_Library SHALL produce exactly `StepCount` stair modules placed at equal angular intervals around the loop, each rotated so its tread faces the traversal direction.

3. THE PCG_Library SHALL provide a top-level graph `PCG_EscherBridgeLoopEx` that accepts two spline endpoints and generates a bridge path that curves back and connects to its own origin at the same elevation, with all span modules using `PCGCol_Baroque_Bridges` meshes.

4. WHEN `PCG_EscherBridgeLoopEx` receives a SplinePath input with fewer than 2 control points, THE PCG_Library SHALL emit zero output points and set a PCG warning attribute `BridgeError=true` on the output dataset rather than crashing or generating degenerate geometry.

5. THE PCG_Library SHALL provide a top-level graph `PCG_ImpossibleCorridor` that generates a corridor segment whose exit portal is rotated 90° relative to its entry portal, using the `Tensor` field system from PCGEx to re-orient spawn normals.

6. THE PCG_Library SHALL provide a reusable subgraph `PCG_Sub_PenroseLoop` that computes per-point world offsets and rotations for a closed-loop stair ring, accepting `StepCount` (int32), `LoopRadius` (float), and `TiltAngleDeg` (float, −45 to 45) as Graph_Parameters, and outputting a point set with `ModuleIndex`, `LoopAngleDeg`, and `StepHeightOffset` attributes set on every point.

7. WHILE a PCG_Volume referencing `PCG_PenroseStaircaseEx` is selected in the Editor, THE PCG_Library SHALL expose `LoopRadius`, `StepCount`, `TiltAngleDeg`, and `Seed` as visible, editable Graph_Parameters in the Details panel without requiring the graph to be opened.

8. THE PCG_Library SHALL provide a top-level graph `PCG_GravityFlipPlatformEx` that generates a platform cluster where half the platforms are inverted (floor facing downward) using PCGEx Transform Points with a per-point `GravityZone` attribute toggle.


### Requirement 2: Recursive and Fractal Structures

**User Story:** As a level designer, I want PCG graphs that generate self-similar and recursively nested structures — nested arches, fractal buttresses, spiral towers, and tessellating tile fields — so that environments carry Escher-style mathematical depth at any scale.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a top-level graph `PCG_RecursiveArchEx` that generates a nested arch sequence: a primary arch containing `RecursionDepth` additional shrinking arches, each scaled by `GoldenRatio` (0.618) relative to its parent, with `RecursionDepth` clamped to the range 1–5, and SHALL validate both that the input `RecursionDepth` is within range and that each output ring's bounding radius equals the previous ring's bounding radius multiplied by 0.618 within 1% tolerance.

2. WHEN `PCG_RecursiveArchEx` is executed with `RecursionDepth=N` where N is in {1, 2, 3, 4, 5}, THE PCG_Library SHALL produce exactly N arch module rings in the output point set, where each ring's bounding radius equals the previous ring's bounding radius multiplied by 0.618 within 1% tolerance. IF `RecursionDepth` is outside the range 1–5, THEN THE PCG_Library SHALL clamp the value to the nearest valid bound and write a `PCGDiagnostic` attribute describing the adjustment, rather than executing with the out-of-range value.

3. THE PCG_Library SHALL provide a top-level graph `PCG_FractalTessellationEx` that tiles a planar surface with interlocking geometric shapes (triangles, hexagons, or Penrose tiles), selectable via a `TilePattern` Graph_Parameter of enumeration type `EMelodiaTilePattern` with values `Triangular`, `Hexagonal`, and `PenroseDiamond`.

4. WHEN `PCG_FractalTessellationEx` is executed with `TilePattern=PenroseDiamond`, THE PCG_Library SHALL generate two alternating diamond mesh types (`ThinDiamond` and `ThickDiamond`) placed so that no two diamonds of the same type share a full edge, validated by a per-point `TileType` attribute on all output points.

5. THE PCG_Library SHALL provide a top-level graph `PCG_SpiralTowerEx` that generates a spiral tower along a vertical axis, with each story ring rotated by `TwistDegPerStory` degrees (Graph_Parameter, float, 0–45) relative to the story below, and `TowerStories` stories total (Graph_Parameter, int32, 2–20).

6. THE PCG_Library SHALL provide a reusable subgraph `PCG_Sub_GoldenShrink` that accepts an input point set and outputs a new point set at `GoldenRatio` scale centered on each input point, applying recursive depth tags (`RecursionDepth` attribute) up to the value of the `MaxDepth` Graph_Parameter.

7. WHEN `PCG_Sub_GoldenShrink` is executed with `MaxDepth=D`, THE PCG_Library SHALL produce a point set whose total point count equals the sum of `InputCount × 1 + InputCount × depth` iterations, with each output point carrying a `RecursionDepth` integer attribute between 0 and D inclusive.

8. THE PCG_Library SHALL provide a top-level graph `PCG_FractalButtressEx` that upgrades the existing experimental `PCG_FractalButtress_BS` graph, generating gothic buttress pairs along a wall spline with nested bracket repetitions at `RecursionDepth` levels using `PCGCol_Baroque_Walls` meshes.


### Requirement 3: Floating and Gravity-Defying Layouts

**User Story:** As a level designer, I want PCG graphs that generate floating islands, suspended walkways, and inverted architecture, so that I can build the gravity-defying dreamscapes central to Melodia Melusina's surreal aesthetic.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a top-level graph `PCG_FloatingIslandClusterEx` that accepts a seed point and generates a cluster of 3–12 floating island platforms (count controlled by `IslandCount` Graph_Parameter, int32) distributed within a spherical radius (`ClusterRadius` Graph_Parameter, float, 500–5000 cm), each platform offset vertically by a noise-driven value with amplitude controlled by `VerticalVariance` (float, 0–1000 cm).

2. WHEN `PCG_FloatingIslandClusterEx` is executed with any `IslandCount` in the valid range, THE PCG_Library SHALL produce at least `IslandCount` output points, each with a `PlatformRadius` attribute (float) and an `IslandIndex` attribute (int32) set on every point.

3. THE PCG_Library SHALL provide a top-level graph `PCG_SuspendedWalkwayEx` that places walkway span modules along a SplinePath input, with each span optionally connected by chain/rope anchor points (`ChainAnchors` bool Graph_Parameter), using `PCGCol_Baroque_Bridges` meshes.

4. WHEN `PCG_SuspendedWalkwayEx` receives a SplinePath longer than `ModuleWidth` (400 cm), THE PCG_Library SHALL produce at least one walkway span module per 400 cm of spline arc length, and every walkway span point SHALL carry both a `Walkable` attribute set to `true`.

5. THE PCG_Library SHALL provide a top-level graph `PCG_InvertedArchitectureEx` that generates a mirrored architectural layout where floors face downward and columns hang from a ceiling plane, with the graph provision and `GravityZone=Inverted` attribute assignment occurring together as one atomic operation so that all points in the output carry the `GravityZone=Inverted` attribute simultaneously.

6. THE PCG_Library SHALL provide a reusable subgraph `PCG_Sub_FloatNoise` that accepts input points and applies a vertical noise displacement using PCGEx noise nodes, with `NoiseScale` (float), `NoiseAmplitude` (float, cm), and `Seed` as Graph_Parameters, preserving all existing point attributes.

7. WHEN `PCG_Sub_FloatNoise` is applied to an input point set of N points, THE PCG_Library SHALL output exactly N points with each point's Z coordinate displaced by a value within the range `[−NoiseAmplitude, +NoiseAmplitude]`.

8. THE PCG_Library SHALL provide a top-level graph `PCG_ArchipelagoEx` that upgrades the existing `PCG_BridgeArchipelago` vanilla graph, connecting floating island clusters with bridge spans using PCGEx Pathfinding to compute minimum-cost spanning connections between island center points.


### Requirement 4: Organic Flowing Environments

**User Story:** As a level designer, I want PCG graphs for vine-covered ruins, magical spiral gardens, and tendril-draped architecture, so that I can produce the lush, organic Infinity Nikki aesthetic alongside Melusina's baroque surrealism.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a top-level graph `PCG_VineOvergrowthEx` that places vine and tendril meshes along surfaces and architectural edges, driven by a SplinePath or surface normal input, with `OvergrowthDensity` (float, 0–1) and `VineLength` (float, cm) as Graph_Parameters.

2. WHEN `PCG_VineOvergrowthEx` executes with `OvergrowthDensity=0`, THE PCG_Library SHALL produce zero vine mesh points in the output.

3. WHEN `PCG_VineOvergrowthEx` executes with `OvergrowthDensity=1`, THE PCG_Library SHALL produce vine mesh points spaced no greater than 100 cm apart along valid surface normal sample paths, prioritizing strict spacing over complete coverage so that any surface region where 100 cm spacing cannot be maintained has its vine points skipped.

4. THE PCG_Library SHALL provide a top-level graph `PCG_MagicalGardenEx` that scatters flora (flowers, ferns, luminescent mushrooms) across a landscape surface, using PCGEx Sampling for slope and curvature-aware placement, with `FloraVariety` int32 Graph_Parameter (1–5 controlling the number of distinct flora mesh variants drawn from `PCGCol_Nikki_Flora`).

5. THE PCG_Library SHALL provide a top-level graph `PCG_SpiralGardenTerraceEx` that generates a multi-tier spiral terrace structure with planted edges, combining `PCG_SpiralTowerEx` geometry for the terrace rings and `PCG_VineOvergrowthEx` decoration for the edges, via subgraph composition.

6. THE PCG_Library SHALL provide a reusable subgraph `PCG_Sub_SplineFlora` that distributes flora props along a SplinePath at a spacing defined by `FloraSpacing` (float, cm, Graph_Parameter), with random yaw rotation per point and a `FloraRole` string attribute set to one of `Flower`, `Fern`, `Mushroom`, or `Grass`.

7. WHEN `PCG_Sub_SplineFlora` receives a SplinePath of arc length L cm and `FloraSpacing` of S cm, THE PCG_Library SHALL produce between `floor(L/S) − 1` and `ceil(L/S) + 1` flora points along the spline.

8. THE PCG_Library SHALL provide a top-level graph `PCG_TendrilArchEx` that drapes tendril meshes from arch keystone points downward, using PCGEx Offset Path to generate hanging chain point sets from arch-top positions, with `TendrilDensity` and `TendrilDropLength` Graph_Parameters.

9. THE PCG_Library SHALL provide a MeshCollection asset `PCGCol_Nikki_Flora` containing at least 5 flora mesh slots (flowers, ferns, luminescent mushrooms, ground cover, tall grass) tagged with `FloraRole` values matching the attribute written by `PCG_Sub_SplineFlora`.


### Requirement 5: Stylized Fantasy Decoration

**User Story:** As a level designer, I want PCG graphs that scatter lights, props, and decorative elements along splines and surfaces in the Infinity Nikki fantasy style, so that I can dress environments with glowing lanterns, flower clusters, and magical particle anchors without placing each item by hand.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a top-level graph `PCG_LanternSplineEx` that places lantern and light prop meshes along a SplinePath at a spacing defined by `LanternSpacing` (float, cm, Graph_Parameter), with each lantern point carrying a `LightColor` vector attribute and `LightRadius` float attribute for downstream light-placement systems.

2. WHEN `PCG_LanternSplineEx` receives a SplinePath input, THE PCG_Library SHALL set `LightColor` and `LightRadius` on every lantern output point, with default values of `(1.0, 0.9, 0.6)` and `300.0` cm respectively, overridable via Graph_Parameters.

3. THE PCG_Library SHALL provide a top-level graph `PCG_FloralBorderEx` that places alternating flower and foliage meshes along path or wall edges, using `PCGCol_Nikki_Flora` meshes, with `BorderWidth` (float, cm) and `PatternPeriod` (int32, 1–8 meshes per repeat) as Graph_Parameters.

4. THE PCG_Library SHALL provide a top-level graph `PCG_MagicParticleAnchorEx` that places Niagara emitter anchor points on surface normals within a PCG volume, using slope angle filtering to concentrate anchors on near-horizontal surfaces, with `AnchorDensity` (float, 0–1) and `MaxSlopeAngleDeg` (float, 0–45) as Graph_Parameters.

5. WHEN `PCG_MagicParticleAnchorEx` executes, THE PCG_Library SHALL produce zero anchor points on surfaces whose world-space slope angle exceeds `MaxSlopeAngleDeg`.

6. THE PCG_Library SHALL provide a top-level graph `PCG_PropScatterEx` that scatters decorative props (pedestals, crystals, urns, signage) within a PCG volume, using PCGEx Uber Filter to enforce minimum separation distance `MinPropSpacing` (float, cm, Graph_Parameter) between any two spawned props.

7. WHEN `PCG_PropScatterEx` executes with `MinPropSpacing=D`, THE PCG_Library SHALL produce an output point set where the Euclidean distance between any two points is greater than or equal to D cm.

8. THE PCG_Library SHALL provide a MeshCollection asset `PCGCol_Nikki_Decoration` containing at least 6 decoration mesh slots (lanterns, crystals, pedestals, urns, mushroom clusters, flower arches) tagged with `DecoRole` values `Light`, `Crystal`, `Vessel`, `Flora`, and `Structure`.


### Requirement 6: Reusability and Composability

**User Story:** As a technical artist, I want every graph in the library to follow a strict composability contract — shared parameters, shared attribute vocabulary, and wirable subgraph inputs — so that I can chain graphs together or swap subgraphs without rewiring the whole network.

#### Acceptance Criteria

1. THE PCG_Library SHALL expose the following Graph_Parameters on every top-level graph: `ModuleWidth` (float, default 400), `StoryHeight` (float, default 600), `Seed` (int32, default 0), and `MeshCollection` (PCGEx Mesh Collection reference, default the family-appropriate `PCGCol_*` asset).

2. WHEN the `Seed` Graph_Parameter is set to the same value across two separate PCG_Volume instances referencing the same graph, THE PCG_Library SHALL produce identical output point sets (position, rotation, scale, and all attributes) for both instances.

3. THE PCG_Library SHALL define and consistently write the shared PointAttribute vocabulary on all output points: `ModuleIndex` (int32), `ArchitecturalRole` (string), `Walkable` (bool), `RecursionDepth` (int32), `GridX` (int32), `GridY` (int32), `ShapeId` (int32), `GravityZone` (string, `Normal` or `Inverted`), `FloraRole` (string), and `DecoRole` (string), with missing values defaulting to zero or empty string.

4. THE PCG_Library SHALL ensure that every PCG_Subgraph (`PCG_Sub_*`) exposes both an input pin and an output pin that accept and pass through the full PCG point dataset, so subgraphs can be daisy-chained in series without data loss.

5. WHEN a top-level graph references a PCG_Subgraph via a Subgraph node, THE PCG_Library SHALL allow the `MeshCollection` Graph_Parameter from the top-level graph to override the default collection inside the subgraph without requiring the subgraph to be opened or modified.

6. THE PCG_Library SHALL provide a reusable subgraph `PCG_Sub_AttributeDefaults` that sets all shared PointAttribute vocabulary fields to their default values on any input point set that is missing those attributes, ensuring downstream filters never receive undefined attributes.

7. THE PCG_Library SHALL ensure that the WalkabilityCheck subgraph (`PCG_Sub_WalkabilityCheck`) is wired as a terminal validation step on every top-level graph that produces points tagged `Walkable=true`, enforcing step height ≤ 50 cm and path width ≥ 200 cm.

8. WHEN `PCG_Sub_WalkabilityCheck` detects a floor point that violates the step-height or path-width constraints, THE PCG_Library SHALL set `Walkable=false` on that point and keep it in the output dataset, rather than removing it, so that downstream systems can inspect and reason about non-walkable geometry. THE PCG_Library SHALL consistently apply this handling for all constraint violations (step-height, path-width, or both) on every affected point.


### Requirement 7: C++ Extension Points

**User Story:** As a C++ programmer on the project, I want custom PCG nodes and Blueprint-callable utility functions implemented in the `MelodiaMelusina_PROD` module, so that graph designers can access math-heavy operations (golden-ratio recursion, Penrose loop transforms, gravity-zone transforms) that vanilla PCG and PCGEx nodes cannot express.

#### Acceptance Criteria

1. THE PCG_Library SHALL implement a C++ class `UPCGMelodiaPenroseLoopSettings` (derived from `UPCGSettings`) inside the `MelodiaMelusina_PROD` module, exposed to the PCG graph editor as a node under the `Melodia|PCG` category, that transforms an input point set into a closed-loop staircase ring given `StepCount`, `LoopRadius`, and `TiltAngleDeg` parameters.

2. THE PCG_Library SHALL implement a C++ class `UPCGMelodiaGoldenShrinkSettings` (derived from `UPCGSettings`) that applies iterative golden-ratio (0.618) scale reduction to an input point set up to `MaxDepth` recursion levels, writing a `RecursionDepth` int32 attribute on every output point.

3. THE PCG_Library SHALL implement a C++ class `UPCGMelodiaGravityZoneSettings` (derived from `UPCGSettings`) that reads a `GravityZone` string attribute on input points and applies the corresponding world-space transform inversion (180° X-axis rotation for `Inverted`, identity for `Normal`) to point orientations.

4. THE PCG_Library SHALL implement a C++ class `UPCGMelodiaTessellationSettings` (derived from `UPCGSettings`) that generates a planar tiling pattern of type `EMelodiaTilePattern` across a bounding rectangle, outputting one point per tile with `TileType` (string) and `TileIndex` (int32) attributes.

5. THE PCG_Library SHALL implement a Blueprint Function Library class `UMelodiaPCGMathLibrary` in the `MelodiaMelusina_PROD` module, exposing the following `BlueprintPure` functions: `ComputePenroseStepTransform(StepIndex, StepCount, LoopRadius, TiltAngleDeg) → FTransform`, `ApplyGoldenShrink(Scale, Depth) → float`, `ComputeIslandDistribution(Seed, Count, Radius) → TArray<FVector>`, and `IsTilePatternValid(TilePattern, GridX, GridY) → bool`.

6. WHEN `UMelodiaPCGMathLibrary::ComputePenroseStepTransform` is called with `StepIndex=StepCount−1`, THE C++_PCGNode SHALL return a transform whose Z-axis position plus one step height equals the Z-axis position of `StepIndex=0`, within 1 cm tolerance, confirming the loop closes.

7. THE PCG_Library SHALL add `"PCG"` to the `PublicDependencyModuleNames` in `MelodiaMelusina_PROD.Build.cs` so that all C++ PCG node classes compile without requiring manual developer setup.

8. THE PCG_Library SHALL add `"PCGExtendedToolkit"` (or the canonical PCGEx module name) to `PrivateDependencyModuleNames` in `MelodiaMelusina_PROD.Build.cs` only if any C++ class directly includes PCGEx headers. IF no C++ class in the library includes PCGEx headers, THEN THE PCG_Library SHALL remove or omit that dependency from the build file to maintain strict compilation coupling minimization.


### Requirement 8: Aesthetic Fidelity

**User Story:** As the art director, I want all PCG outputs to match Melusina's MooaToon SDF/oil-paint aesthetic — Infinity Nikki's whimsy fused with Escher's precision — so that procedurally placed meshes are visually indistinguishable in style from hand-crafted set-dressing.

#### Acceptance Criteria

1. THE PCG_Library SHALL use only meshes from the MeluPalette (`SM_wallhi_*`, `SM_wallmid_*`, `SM_wallshort_*`, `SM_SpiralStair001`, `SM_venetianbridge`, `SM_PENROSE*`, `SM_surrealtower*`) or `PCGCol_Nikki_*` collections for architectural and flora spawning, and SHALL NOT embed raw mesh references directly in Static Mesh Spawner nodes.

2. WHEN a PCG_Graph in the library executes and produces a Static Mesh spawner node, THE PCG_Library SHALL apply an explicit material override tracked via either a PCGEx Staged Mesh Selector collection entry (with a pre-assigned material slot) or a Template Descriptor override on the spawner node, so that no spawned instance uses `DefaultMaterial`. The material assignment method (collection or override flag) SHALL be explicitly documented in the graph's spawner node comment.

3. THE PCG_Library SHALL expose a `StylePreset` Graph_Parameter of enumeration type `EMelodiaStylePreset` with values `Baroque`, `EscherSurreal`, `NikkiWhimsical`, and `NikkiRuins`, and use it to select the appropriate MeshCollection and material override on each graph.

4. WHEN `StylePreset=EscherSurreal`, THE PCG_Library SHALL apply a golden-ratio scale modulation to all module placements, such that modules at `RecursionDepth=1` are 0.618× the size of modules at `RecursionDepth=0`, and SHALL use desaturated, high-contrast materials consistent with Escher's monochromatic engraving palette.

5. WHEN `StylePreset=NikkiWhimsical`, THE PCG_Library SHALL use pastel-tinted MooaToon material variants and flora-rich mesh collections, with no inverted-gravity or impossible-geometry transforms applied.

6. THE PCG_Library SHALL ensure all spawned Escher-family mesh instances have a bounding box of at least 50 cm on every axis, regardless of current view distance or camera position, unless the point carries a `DecoRole=Flora` attribute. This constraint is enforced at generation time on all output points.

7. THE PCG_Library SHALL NOT use the `DefaultMaterial` material on any spawned mesh instance; every spawner SHALL specify an explicit material override or reference a MeshCollection entry with a pre-assigned material.


### Requirement 9: Performance and Scalability

**User Story:** As a developer, I want the PCG library to generate outputs efficiently enough that iterating on layouts in the Unreal Editor does not stall, and that real-time regeneration during gameplay (for roguelike zones) stays within frame budget.

#### Acceptance Criteria

1. WHEN any top-level graph in the PCG_Library is executed inside the UE5 Editor on a PCG_Volume whose input surface fits within a 100 m × 100 m bounding box, THE PCG_Library SHALL complete graph execution and produce a final point set in under 500 milliseconds on a mid-range developer workstation (Intel i7-class or equivalent, PCG CPU execution, no GPU offload required).

2. THE PCG_Library SHALL support PCG graph execution in Unreal's threaded CPU worker mode; no custom C++ node SHALL use non-thread-safe APIs (e.g., direct `UWorld` mutations, `DrawDebug*` calls in non-editor builds) inside `Execute()` or `ExecuteWithContext()`.

3. THE PCG_Library SHALL limit the maximum number of output points from any single top-level graph execution to 10,000 points, enforced by a `MaxOutputPoints` Graph_Parameter (int32, default 2000, hard clamped to 10,000) applied as a downsample step at the graph's output.

4. WHEN `RecursionDepth` is set to 5 on any recursive graph, THE PCG_Library SHALL complete execution in under 2 seconds, achieved by capping the exponential point growth using the `MaxOutputPoints` limit before each recursive iteration.

5. THE PCG_Library SHALL use PCGEx Mesh Collections exclusively (not inline mesh arrays) so that cook-time deduplication of mesh references reduces packaged content size.

6. THE PCG_Library SHALL tag all graph outputs intended for Hierarchical Instanced Static Mesh (HISM) component generation with a `UseHISM=true` point attribute, enabling the PCG HISM spawner to batch identical meshes and reduce draw calls.

7. WHEN the roguelike run manager (via `BP_ReverieRunManager`) triggers a zone generation with a new seed, THE PCG_Library SHALL support seed-parameter injection via PCG_Volume actor tags so that runtime zone seeding does not require a PCG graph asset re-cook.


### Requirement 10: Editor Tooling and Workflow

**User Story:** As a level designer, I want the PCG library to integrate cleanly into the UE5 Editor workflow — with one test level per graph, a Python builder script for batch creation, and clear diagnostics — so that iteration cycles are fast and self-documenting.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a dedicated TestLevel (`L_PCGTest_*`) for every top-level graph in the library, pre-configured with a PCG_Volume whose Graph reference and graph parameters are set to default values, so a designer can open the level and immediately see a valid generated output.

2. THE PCG_Library SHALL extend the existing `Scripts/PCG/melodia_pcgex_builder.py` Python script to include all new top-level graphs, callable via `mod.build_graph("PCG_<GraphName>")` or `mod.build_all()` to batch-create graph assets under `/Game/_PROJECT/PCG/Graphs/PCGEx/`.

3. WHEN a PCG_Graph in the library executes and produces zero output points for any reason — whether due to invalid parameters, empty input, or unsatisfied constraints — THE PCG_Library SHALL write a `PCGDiagnostic` string attribute to the output dataset describing the failure reason (e.g., `"StepCount must be >= 4"`, `"SplinePath has no control points"`, `"No valid surface normals found"`). IF the zero-output is caused by an invalid Graph_Parameter, THEN THE PCG_Library SHALL additionally attempt to spawn a default static mesh cube (100×100×100 cm) at the PCG_Volume origin as a visual placeholder so designers can see that the volume is active.

4. THE PCG_Library SHALL follow the naming convention: top-level graphs as `PCG_<Feature>Ex`, subgraphs as `PCG_Sub_<Feature>`, test levels as `L_PCGTest_<Feature>`, collections as `PCGCol_<Family>_<Category>`, with no `_BS`, `_BSS`, or `SceneImport_` prefixes.

5. THE PCG_Library SHALL store all new PCG graph assets under `/Game/_PROJECT/PCG/Graphs/PCGEx/` (current path) with planned migration to `/Game/Melodia/PCG/` per `FOUNDATION.md §2`, and all new test levels under `/Game/_PROJECT/PCG/TestLevels/`.

6. THE PCG_Library SHALL include a verification checklist comment block at the top of each PCG graph asset's description field listing: `[ ] Generates without empty output`, `[ ] No broken mesh refs`, `[ ] Module spacing consistent at ModuleWidth`, `[ ] Walkability subgraph wired (if Walkable)`, `[ ] Materials: no DefaultMaterial`.

7. WHERE a designer selects a PCG_Volume in the Editor and that volume references a library graph, THE PCG_Library SHALL expose all Graph_Parameters defined in Requirement 6.1 as visible and editable in the Actor's Details panel through standard PCG graph parameter overrides.

8. THE PCG_Library SHALL provide a Blueprint Actor class `BP_PCGLibraryTestHarness` that places one PCG_Volume per registered graph in the library at spaced positions in a flat test level, allowing a single actor placement to preview all graphs simultaneously.


### Requirement 11: Property-Based Correctness Properties

**User Story:** As a developer, I want the deterministic and mathematical properties of the PCG library's C++ nodes to be automatically tested, so that refactors and parameter changes cannot silently break the geometric invariants the library depends on.

#### Acceptance Criteria

1. THE PCG_Library SHALL provide a C++ test class `FMelodiaPCGLoopTests` (using Unreal Automation Framework, `IMPLEMENT_SIMPLE_AUTOMATION_TEST`) that runs property-based tests against `UMelodiaPCGMathLibrary` functions with randomly sampled inputs within valid parameter ranges.

2. FOR ALL valid `StepCount` values in {4, 8, 12, 16, 24, 32, 48, 64} and `LoopRadius` values in {200, 400, 800, 1600, 3200} cm, THE Penrose loop round-trip property SHALL hold: the Z-coordinate of `ComputePenroseStepTransform(StepCount−1, StepCount, LoopRadius, TiltAngleDeg)` plus one step height equals the Z-coordinate of `ComputePenroseStepTransform(0, StepCount, LoopRadius, TiltAngleDeg)` within 1 cm tolerance.

3. FOR ALL valid inputs `Scale ∈ [0.01, 100.0]` and `Depth ∈ {0, 1, 2, 3, 4, 5}`, THE golden shrink idempotence property SHALL hold: `ApplyGoldenShrink(ApplyGoldenShrink(Scale, Depth), 0) == ApplyGoldenShrink(Scale, Depth)` (applying zero additional depth leaves the value unchanged).

4. FOR ALL valid inputs `Scale` and `Depth ∈ {1, 2, 3, 4, 5}`, THE golden shrink monotonicity property SHALL hold: `ApplyGoldenShrink(Scale, Depth) < ApplyGoldenShrink(Scale, Depth−1)` (each recursion level produces a strictly smaller scale).

5. FOR ALL valid `Seed ∈ [0, 2^31−1]`, `Count ∈ {3, 4, 5, 6, 7, 8, 9, 10, 11, 12}`, `Radius ∈ {500, 1000, 2000, 5000}`, THE island distribution count property SHALL hold: `ComputeIslandDistribution(Seed, Count, Radius).Num() == Count`.

6. FOR ALL `Seed` values, THE determinism property SHALL hold: two calls to `ComputeIslandDistribution(Seed, Count, Radius)` with identical arguments SHALL return arrays with identical element positions in the same order.

7. FOR ALL valid `GridX`, `GridY` integers and `TilePattern=PenroseDiamond`, THE tessellation alternation property SHALL hold: `IsTilePatternValid(PenroseDiamond, GridX, GridY) != IsTilePatternValid(PenroseDiamond, GridX+1, GridY)` OR `IsTilePatternValid(PenroseDiamond, GridX, GridY) != IsTilePatternValid(PenroseDiamond, GridX, GridY+1)`, confirming that no two adjacent tiles share the same type.

8. FOR ALL valid `StepCount ∈ {4..64}` and `LoopRadius > 0`, THE angular spacing invariant SHALL hold: the angle between any two consecutive step transforms computed by `ComputePenroseStepTransform` equals `360.0 / StepCount` degrees within 0.001-degree tolerance.


### Requirement 12: Integration with Existing Project Systems

**User Story:** As a developer, I want the PCG library to integrate with the existing roguelike zone seeding system, the audio-reactive material system, and the existing baroque PCG graph family, so that the new graphs participate in the same ecosystem without duplicating systems or breaking conventions.

#### Acceptance Criteria

1. THE PCG_Library SHALL be additive to the existing 18 baroque/surreal/nature PCG graphs and SHALL NOT modify, delete, or rename any existing PCG graph asset, TestLevel, or MeshCollection asset.

2. THE PCG_Library SHALL use the same `ModuleWidth=400`, `StoryHeight=600`, and `Seed` Graph_Parameter names and types already established in `PCG_SYSTEM_COHESION_2026-06-13.md` so that level blueprints can drive all graphs (old and new) with the same parameter-injection pattern.

3. WHEN the roguelike run manager (`BP_ReverieRunManager`) sets or updates the actor tag `PCGSeed` on a PCG_Volume at any point — whether during zone generation or at any other time a new seed value is provided — THE PCG_Library SHALL read the new seed value from the tag and override the graph's `Seed` parameter at runtime using UE5's PCG graph parameter override mechanism, triggering a graph re-execution.

4. THE PCG_Library SHALL emit an `AudioReactive=true` PointAttribute on any output point that corresponds to a mesh with an audio-reactive material slot, enabling the `MPC_MusicClock.BeatPulse`-driven material system to locate those instances for parameter animation.

5. THE PCG_Library SHALL ensure all new graphs follow the folder and naming conventions in `FOUNDATION.md §3` and `PCG_SYSTEM_COHESION_2026-06-13.md §5`: no `_BS`/`_BSS`/`SceneImport_` suffixes, no spaces, no diacritics, `PCG_` prefix for graphs, `PCGCol_` for collections, `L_PCGTest_` for test levels.

6. THE PCG_Library SHALL add all new `PCGCol_Nikki_*` MeshCollection entries to the catalog file `Scripts/PCG/baroque_mesh_catalog.json` in the same format as existing `PCGCol_Baroque_*` entries. THE PCG_Library SHALL be considered functionally complete only when the catalog file has been successfully updated, as the Python builder script depends on the catalog to populate collections automatically.

