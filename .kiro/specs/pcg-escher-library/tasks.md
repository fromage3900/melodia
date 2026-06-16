# Implementation Plan: PCG Escher-Nikki Library

## Overview

This plan implements the Escher-Nikki PCG tier in four phases: (1) C++ custom PCG element
infrastructure, (2) Python builder script and mesh catalog extension, (3) PCG subgraph and
top-level graph assets created via the builder in-editor, and (4) test levels. Each phase
builds directly on the previous. All C++ lives in `Source/MelodiaMelusina_PROD/PCG/`. All
graph assets are created via `escher_nikki_builder.py` running inside the UE Editor Python
console. Test levels are created via Monolith MCP editor tooling.

---

## Tasks

- [~] 1. Update Build.cs and create C++ module directory
  - Add `"PCG"`, `"PCGEx"`, and `"GeometryScripting"` to `PublicDependencyModuleNames` in
    `Source/MelodiaMelusina_PROD/MelodiaMelusina_PROD.Build.cs` using `str_replace`.
  - Create the `Source/MelodiaMelusina_PROD/PCG/` directory (an empty `.gitkeep` is
    sufficient; actual files follow in tasks 2–5).
  - _Requirements: 1.1, 1.2_

- [ ] 2. Implement `PCGEscherStaircaseSettings` (header + source)
  - [x] 2.1 Write `PCGEscherStaircaseSettings.h`
    - Declare `UPCGEscherStaircaseSettings : public UPCGSettings` with UPROPERTY fields
      `StepCount` (int32, default 16, ClampMin=3, ClampMax=256), `StepHeight` (float,
      default 25 cm), `StepWidth` (float, default 150 cm), `LoopRadius` (float, default
      600 cm), `Seed` (int32, default 0).
    - Declare `FPCGEscherStaircaseElement : public FPCGElement` with `Execute()` override.
    - Declare `STAT_PCGEscherElement` cycle stat extern.
    - Use `#if WITH_PCGEX` guard for any PCGEx includes.
    - _Requirements: 1.1, 1.3, 1.8, 12.3_

  - [-] 2.2 Write `PCGEscherStaircaseSettings.cpp`
    - Implement `Execute()`: distribute N points on helix (radius `LoopRadius`, angular step
      = 2π/N, Z step = `StepHeight`). Project final point XY back to first point XY and Z
      back to Z=0 to close the loop within 1 cm tolerance.
    - Implement parameter clamping preamble (StepCount < 3 → clamp to 3, log
      `UE_LOG(LogPCG, Warning, ...)`).
    - Wrap execution body in `SCOPE_CYCLE_COUNTER(STAT_PCGEscherElement)`.
    - No static mutable state; all temporaries stack-local or context-allocator.
    - _Requirements: 1.1, 1.3, 1.7, 12.3, 12.4_

  - [ ]* 2.3 Write property test for Staircase Loop Closure (Property 1)
    - Use UE Automation Framework `FAutomationTest` to generate random
      `StepCount ∈ [3, 256]`, `StepHeight`, `LoopRadius`, call `Execute()`, assert
      `distance(point[0].XY, point[N-1].XY) ≤ 1 cm`.
    - **Property 1: Staircase Loop Closure**
    - **Validates: Requirements 1.3, 2.1**

- [ ] 3. Implement `PCGGravityZoneSettings` (header + source)
  - [x] 3.1 Write `PCGGravityZoneSettings.h`
    - Declare `UPCGGravityZoneSettings : public UPCGSettings` with UPROPERTY `GravityDir`
      (FVector, default (0,0,-1)).
    - Declare `FPCGGravityZoneElement : public FPCGElement` with `Execute()` override.
    - _Requirements: 1.1, 1.4, 1.8_

  - [-] 3.2 Write `PCGGravityZoneSettings.cpp`
    - Implement `Execute()`: copy all N input points to output; write `GravityDir` FVector
      attribute to each output point equal to `Settings->GravityDir`.
    - Handle empty input (N=0) → return empty output, no crash.
    - No static mutable state.
    - _Requirements: 1.4, 12.4_

  - [ ]* 3.3 Write property test for GravityDir Attribute Presence (Property 4)
    - Generate random input point sets (size N ≥ 1) and random `GravityDir` vectors, call
      `Execute()`, assert output has exactly N points and each point's `GravityDir`
      attribute equals the input FVector within floating-point epsilon.
    - **Property 4: GravityDir Attribute Presence**
    - **Validates: Requirements 1.4, 5.1, 5.5**

- [ ] 4. Implement `PCGRecursiveArchSettings` (header + source)
  - [x] 4.1 Write `PCGRecursiveArchSettings.h`
    - Declare `UPCGRecursiveArchSettings : public UPCGSettings` with UPROPERTYs:
      `ArchWidth` (float, default 400 cm, ClampMin=50), `ArchHeight` (float, default 600 cm,
      ClampMin=50), `RecursionDepth` (int32, default 2, ClampMin=1, ClampMax=4),
      `ScaleFactor` (float, default 0.618, ClampMin=0.3, ClampMax=1.0).
    - Declare `FPCGRecursiveArchElement : public FPCGElement` with `Execute()` override.
    - _Requirements: 1.1, 1.5, 1.8_

  - [-] 4.2 Write `PCGRecursiveArchSettings.cpp`
    - Implement `Execute()`: for depth 0..RecursionDepth-1, compute arch semi-circle points
      at width = `ArchWidth × ScaleFactor^d`. Emit separate tagged `FPCGTaggedData` per
      tier tagged `"Out_Tier0"`.."Out_TierN"`. Each tier's bounding box width must equal
      the parent tier's width × `ScaleFactor` within 2% tolerance.
    - Clamp `RecursionDepth` outside [1,4], `ScaleFactor` below 0.3 (call
      `FPCGContext::LogAndNotifyUser` for ScaleFactor clamp per Req 4.6).
    - No static mutable state.
    - _Requirements: 1.5, 1.7, 4.3, 4.6, 12.4_

  - [ ]* 4.3 Write property test for Recursive Arch Tier Width Ratio (Property 2)
    - Generate random `ArchWidth`, `RecursionDepth ∈ [1,4]`, `ScaleFactor ∈ [0.3, 1.0]`,
      call `Execute()`, verify that `bbox_width[i+1] / bbox_width[i]` equals `ScaleFactor`
      within 2% relative tolerance for all adjacent tier pairs.
    - **Property 2: Recursive Arch Tier Width Ratio**
    - **Validates: Requirements 1.5, 4.3**

  - [ ]* 4.4 Write property test for ScaleFactor Floor Clamp (Property 7)
    - Pass `ScaleFactor` values S < 0.3 to `PCGRecursiveArchSettings`, call `Execute()`,
      assert the resulting tier width ratio equals the ratio for ScaleFactor = 0.3, not S.
    - **Property 7: ScaleFactor Floor Clamp**
    - **Validates: Requirements 1.5, 4.6**

- [ ] 5. Implement `PCGTessellationSettings` (header + source)
  - [x] 5.1 Write `PCGTessellationSettings.h`
    - Declare `EPCGTileShape : uint8` enum (Square, Hexagon, Penrose) with
      `UMETA(DisplayName=...)` on each value.
    - Declare `UPCGTessellationSettings : public UPCGSettings` with UPROPERTYs:
      `TileShape` (EPCGTileShape, default Square), `TileScale` (float, default 200 cm,
      ClampMin=1).
    - Declare `FPCGTessellationElement : public FPCGElement` with `Execute()` override.
    - _Requirements: 1.1, 1.6, 1.8_

  - [-] 5.2 Write `PCGTessellationSettings.cpp`
    - Implement `Execute()`:
      - Square/Hex: compute grid positions covering input AABB with zero-gap spacing.
        Max neighbor distance ≤ `TileScale × 1.05`.
      - Penrose: P2 deflation to desired density. Both fat (TileType=0) and thin (TileType=1)
        rhombus types must appear for surfaces ≥ 4 × TileScale².
      - Write `TileType` (int32) attribute to every output point.
    - No static mutable state; algorithm O(N²) surface area within 16 ms budget.
    - _Requirements: 1.6, 3.3, 3.4, 12.3, 12.4_

  - [ ]* 5.3 Write property test for Zero-Gap Tessellation (Property 3)
    - Generate random `TileScale ∈ [50, 2000]` for Square and Hexagon shapes over a
      20 m × 20 m surface, call `Execute()`, verify max point-to-point neighbor distance
      ≤ `TileScale × 1.05`.
    - **Property 3: Zero-Gap Tessellation**
    - **Validates: Requirements 1.6, 3.4**

  - [ ]* 5.4 Write property test for Penrose TileType Completeness (Property 5)
    - Generate Penrose tessellation over surfaces of area ≥ 4 × TileScale², assert every
      point has `TileType` ∈ {0, 1} and both values appear at least once.
    - **Property 5: Penrose TileType Completeness**
    - **Validates: Requirements 1.6, 3.3**

  - [ ]* 5.5 Write property test for Parameter Clamping Safety (Property 6)
    - Pass out-of-range parameters to all four Custom_PCG_Elements (e.g. StepCount=2,
      RecursionDepth=0, RecursionDepth=5, TileScale=0), assert `Execute()` does not crash
      and either produces a non-empty valid point set or an empty set only for degenerate
      inputs after clamping.
    - **Property 6: Parameter Clamping Safety**
    - **Validates: Requirements 1.7**

- [~] 6. Trigger hot-reload compile and verify zero errors
  - Call `editor.live_compile` (fall back to `editor.trigger_build` + poll
    `editor.get_build_errors` with 60 s timeout) and assert the result is empty.
  - _Requirements: 1.1, 1.2, 1.8, 12.5_

- [~] 7. Checkpoint — Ensure C++ layer compiles clean
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 8. Extend `baroque_mesh_catalog.json` with `escher_nikki` key
  - [ ] 8.1 Read the existing `Scripts/PCG/baroque_mesh_catalog.json`
    - Verify the current `collections` key exists and note all existing collection names to
      ensure no key duplication.
    - _Requirements: 9.1, 9.3_

  - [~] 8.2 Write the updated catalog with the `escher_nikki` top-level key
    - Add four collection entries: `PCGCol_EscherNikki_Stairs`, `PCGCol_EscherNikki_Tiles`,
      `PCGCol_EscherNikki_Organic`, `PCGCol_EscherNikki_IslandRocks`.
    - Each entry follows the `{ tag, entries: [{ mesh, weight, tags, note? }] }` schema.
    - Placeholder entries carry `"note": "replace with dedicated mesh"`.
    - Mesh paths reuse existing project assets where available
      (`SM_SpiralStair001`, `SM_ceilingsquare`, `SM_MarbleSlabOutline`, etc.).
    - Run `python -c "import json; json.load(open('Scripts/PCG/baroque_mesh_catalog.json'))"` to
      verify valid JSON.
    - _Requirements: 9.1, 9.2, 9.3_

- [ ] 9. Write `Scripts/PCG/escher_nikki_builder.py`
  - [~] 9.1 Write module skeleton, imports, and helper import block
    - Add usage docstring in the same format as `melodia_pcgex_builder.py`.
    - Import helpers from `melodia_pcgex_builder.py` via `importlib.util` file-path pattern:
      `create_graph_asset`, `add_node`, `add_node_or_fallback`, `wire_chain`,
      `configure_spawner_from_collection`, `ensure_directory`, `save_and_reload`,
      `clear_user_nodes`, `mesh_entry`.
    - Define constants: `CATALOG_PATH`, `GRAPH_PACKAGE`, `SUBGRAPH_PACKAGE`.
    - Write `configure_spawner_from_collection_escher(spawner_settings, collection_key,
      catalog)` reading from `catalog["escher_nikki"][collection_key]`.
    - _Requirements: 10.3, 10.5_

  - [~] 9.2 Write the four subgraph builder functions
    - `build_sub_escher_railing(catalog) → str` — node chain per design §4.1.
    - `build_sub_escher_island_base(catalog) → str` — node chain per design §4.2.
    - `build_sub_escher_organic_overlay(catalog) → str` — node chain per design §4.3.
    - `build_sub_escher_terrain_top(catalog) → str` — node chain per design §4.4.
    - Each function exposes a `Seed` Blueprint_Parameter on the graph input node.
    - _Requirements: 8.1, 8.2, 8.3, 8.4, 8.5, 8.6_

  - [~] 9.3 Write the six top-level graph builder functions
    - `build_infinite_staircase_ex(catalog, sub_railing_path) → str` — per design §3.1;
      Blueprint_Parameters: StepCount, StepMeshCollection, RailingEnabled, Seed,
      MaterialOverride; sets `ArchitecturalRole="Stair"`, `Walkable=true`.
    - `build_tessellation_surface(catalog) → str` — per design §3.2; Blueprint_Parameters:
      TileShape, TileScale, AlternateCollection, RandomRotation, Seed; sets
      `ArchitecturalRole="Floor"`, `Walkable=true`.
    - `build_recursive_arch_ex(catalog) → str` — per design §3.3; Blueprint_Parameters:
      ArchWidth, ArchHeight, RecursionDepth, ScaleFactor, ArchCollection, Seed; sets
      `ArchitecturalRole="Arch"`, `RecursionTier`.
    - `build_gravity_defying_structure(catalog) → str` — per design §3.4; Blueprint_Parameters:
      GravityDir, StructureType, ModuleCollection, StoryCount, Seed; sets `GravityDir`,
      `ArchitecturalRole` per StructureType.
    - `build_floating_island_nikki(catalog, sub_island_base_path, sub_terrain_top_path,
      sub_organic_path) → str` — per design §3.5; Blueprint_Parameters: IslandRadius,
      RockCollection, TerrainCollection, CrownEnabled, CrownCollection,
      VerticalLayerSpread, Seed; sets `LayerRole` per layer.
    - `build_vine_tower_nikki(catalog, sub_organic_path) → str` — per design §3.6;
      Blueprint_Parameters: TowerHeight, TowerRadius, VineDensity, OrganicCollection,
      BaroqueCollection, Seed; sets `ArchitecturalRole="Tower"` / `"Organic"`.
    - All PCGEx nodes use `add_node_or_fallback` with vanilla fallback class names.
    - _Requirements: 2.1–2.6, 3.1–3.6, 4.1–4.5, 5.1–5.6, 6.1–6.6, 7.1–7.6, 9.4, 11.1–11.5, 12.1_

  - [~] 9.4 Write `build_all()` and `build_graph()` entry points
    - `build_all(force_rebuild=False) → list[str]`: assert PCGGraph available; PCGEx
      availability check (warn + continue); `ensure_directory` both packages; build 4
      subgraphs in Phase 1, then 6 top-level graphs in Phase 2 passing subgraph paths;
      return ordered list of 10 asset paths.
    - `build_graph(name: str) → str`: load catalog, build all subgraphs first, map name →
      lambda → call builder; raise `KeyError` for unknown names listing valid names.
    - Run `python -m py_compile Scripts/PCG/escher_nikki_builder.py` to verify syntax.
    - _Requirements: 10.1, 10.2, 10.4_

  - [ ]* 9.5 Write property test for Builder Round-Trip Consistency (Property 8)
    - Using pytest (or equivalent), for each graph name in the known EscherNikki name set,
      assert that `build_graph(name)` returns the same asset path string as the
      corresponding element in `build_all()`.
    - **Property 8: Builder Round-Trip Consistency**
    - **Validates: Requirements 10.6**

- [~] 10. Checkpoint — Python script syntax-checks and catalog validates
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 11. Create PCG subgraph assets in-editor via builder
  - [~] 11.1 Run subgraph builders in UE Editor Python console
    - Invoke `escher_nikki_builder.build_all()` using the standard `importlib.util` invocation
      pattern (see design §8.5). This creates the 4 `PCG_Sub_Escher_*` subgraph assets and
      the 6 top-level graph assets in a single call.
    - Verify all 4 subgraph assets exist via `project.get_asset_info` for each path under
      `/Game/_PROJECT/PCG/Graphs/EscherNikki/Subgraphs/`.
    - _Requirements: 8.1–8.6, 10.1_

  - [~] 11.2 Verify subgraph Blueprint_Parameters and Seed exposure
    - For each `PCG_Sub_Escher_*` subgraph, inspect that a `Seed` Blueprint_Parameter (int32,
      default 0) is present on the graph's input node via `project.get_asset_info` or an
      editor Python introspection call.
    - _Requirements: 8.5, 11.1, 11.2_

- [ ] 12. Create and verify top-level EscherNikki PCG graph assets
  - [~] 12.1 Verify all six top-level graph assets exist
    - After `build_all()` completes (task 11.1), call `project.get_asset_info` for each of
      the six graph paths under `/Game/_PROJECT/PCG/Graphs/EscherNikki/` and assert each
      returns a valid asset.
    - _Requirements: 2.1, 3.1, 4.1, 5.1, 6.1, 7.1, 10.1_

  - [~] 12.2 Verify Blueprint_Parameters on each graph
    - Inspect each graph asset to confirm the mandatory Blueprint_Parameters are present
      (at minimum `Seed` int32 and one mesh collection soft reference per graph) using
      editor Python introspection.
    - Confirm `DisplayName` meta on every parameter follows UE naming conventions
      (e.g. "Step Count" not "StepCount").
    - _Requirements: 2.2, 3.2, 4.2, 5.2, 6.2, 7.2, 11.1, 11.5_

  - [~] 12.3 Set `GenerationTrigger = GenerateOnLoad` on all six graphs
    - For each graph asset, write the `GenerationTrigger` property via editor Python to
      `EPCGGraphGenerationTrigger::GenerateOnLoad`.
    - _Requirements: 12.1_

  - [~] 12.4 Verify `ArchitecturalRole`, `Walkable`, and `Seed` attribute writes
    - Open each graph asset and confirm `PCGWriteAttributesSettings` nodes are present with
      the correct attribute name/type/value configuration per the shared attribute vocabulary
      (design §5.1).
    - _Requirements: 2.5, 3.5, 4.4, 5.5, 6.5, 7.5, 14.1_

  - [ ]* 12.5 Write property test for Shared Attribute Merge Compatibility (Property 10)
    - In a test level, place both a `PCG_InfiniteStaircaseEx` component and a baroque `*Ex`
      component, merge their outputs via `PCGExMergePointsSettings`, and assert every merged
      point has `ArchitecturalRole` (string) and `Walkable` (bool) with no type conflicts.
    - **Property 10: Shared Attribute Merge Compatibility**
    - **Validates: Requirements 14.1, 14.4**

- [~] 13. Checkpoint — All graph assets exist, parameters verified, GenerationTrigger set
  - Ensure all tests pass, ask the user if questions arise.

- [ ] 14. Create the six test levels
  - [~] 14.1 Create `L_PCGTest_InfiniteStaircase`
    - `editor.create_empty_map` at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_InfiniteStaircase`.
    - Spawn a PCG component, assign `PCG_InfiniteStaircaseEx`, set `StepCount=16`.
    - Place a flat 20 m × 20 m surface mesh and trigger generation; assert output count ≥ 16.
    - _Requirements: 2.4, 13.1, 13.7_

  - [~] 14.2 Create `L_PCGTest_TessellationSurface`
    - Create at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_TessellationSurface`.
    - Assign `PCG_TessellationSurface`, set `TileShape=Hexagon`, `TileScale=200 cm` over
      a 20 m × 20 m surface.
    - Assert no point-to-point spacing exceeds `TileScale × 1.05`.
    - _Requirements: 3.4, 13.2, 13.7_

  - [~] 14.3 Create `L_PCGTest_RecursiveArch`
    - Create at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_RecursiveArch`.
    - Assign `PCG_RecursiveArchEx`, set `RecursionDepth=3` with default dimensions.
    - Assert exactly 3 tier point sets are produced.
    - _Requirements: 4.3, 13.3, 13.7_

  - [~] 14.4 Create `L_PCGTest_GravityDefying`
    - Create at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_GravityDefying`.
    - Assign `PCG_GravityDefyingStructure`, set `GravityDir=(0,0,1)`, `StructureType=Tower`.
    - Assert all spawned mesh transforms have local up-axis aligned to world-down within 1°.
    - _Requirements: 5.3, 13.4, 13.7_

  - [~] 14.5 Create `L_PCGTest_FloatingIsland`
    - Create at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_FloatingIsland`.
    - Assign `PCG_FloatingIslandNikki`, set `IslandRadius=1200 cm`, `CrownEnabled=true`.
    - Assert at least one instance exists in each of the three composition layers
      (RockBase, TerrainTop, Crown).
    - _Requirements: 6.4, 13.5, 13.7_

  - [~] 14.6 Create `L_PCGTest_VineTower`
    - Create at `/Game/_PROJECT/PCG/TestLevels/L_PCGTest_VineTower`.
    - Assign `PCG_VineTowerNikki`, set `VineDensity=0.5`.
    - Assert both baroque-tier and organic-tier outputs are non-empty.
    - _Requirements: 7.3, 7.4, 13.6, 13.7_

  - [ ]* 14.7 Write property test for VineDensity Extremes (Property 9)
    - Configure `PCG_VineTowerNikki` with `VineDensity=0.0`: assert zero organic instances.
    - Configure with `VineDensity=1.0`: assert organic instance count ≥ baroque module point
      count.
    - **Property 9: VineDensity Extremes**
    - **Validates: Requirements 7.3, 7.4**

- [~] 15. Run walkability check integration for navigable graphs
  - For `PCG_InfiniteStaircaseEx` and `PCG_TessellationSurface` (both write `Walkable=true`),
    confirm via editor Python that `PCG_Sub_WalkabilityCheck` is wired after the
    PCGWriteAttributesSettings node and that navigable-tagged points satisfy ≤ 50 cm max step
    height and ≥ 200 cm min path width per design §5.2.
  - _Requirements: 14.2_

- [~] 16. Final checkpoint — Ensure all tests pass
  - Run `editor.get_build_errors` and assert empty.
  - Verify all 10 graph assets and 6 test level assets exist via `project.get_asset_info`.
  - Run round-trip consistency check: `build_graph(n) == build_all()[i]` for each of the
    6 graph names.
  - Ensure all tests pass, ask the user if questions arise.

---

## Notes

- Tasks marked with `*` are optional and can be skipped for faster MVP delivery.
- Each task references specific requirements for full traceability.
- All PCGEx nodes use `add_node_or_fallback` so graphs degrade gracefully when PCGEx is absent.
- The `#if WITH_PCGEX` guard pattern (design §2.5) ensures shipping builds compile cleanly.
- Subgraphs must always be created before the top-level graphs that reference them; `build_all()`
  enforces this order automatically.
- Placeholder mesh catalog entries are searchable via `"note"` key to track outstanding art
  deliverables.
- Property tests validate universal correctness guarantees; unit tests cover specific boundary
  values and edge cases.

## Task Dependency Graph

```json
{
  "waves": [
    { "id": 0, "tasks": ["2.1", "3.1", "4.1", "5.1"] },
    { "id": 1, "tasks": ["2.2", "3.2", "4.2", "5.2", "8.1"] },
    { "id": 2, "tasks": ["2.3", "3.3", "4.3", "4.4", "5.3", "5.4", "5.5", "8.2", "9.1"] },
    { "id": 3, "tasks": ["9.2", "9.3"] },
    { "id": 4, "tasks": ["9.4"] },
    { "id": 5, "tasks": ["9.5", "11.1"] },
    { "id": 6, "tasks": ["11.2", "12.1"] },
    { "id": 7, "tasks": ["12.2", "12.3", "12.4"] },
    { "id": 8, "tasks": ["12.5", "14.1", "14.2", "14.3", "14.4", "14.5", "14.6"] },
    { "id": 9, "tasks": ["14.7"] }
  ]
}
```
