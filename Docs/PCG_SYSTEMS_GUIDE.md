# Melodia PCG Systems Guide

Single reference for procedural world generation in Melodia: inventory of existing graphs, review findings, PCGEx integration, and the baroque architecture system family.

> Companion: `PCG_SYSTEM_COHESION_2026-06-13.md` (shared parameters, mesh collections, test matrix).  
> Mesh palette: `Scripts/PCG/baroque_mesh_catalog.json`.  
> Builder: `Scripts/PCG/melodia_pcgex_builder.py` (run inside UE Python after enabling PCGEx).

---

## 1. Current inventory (disk-verified)

**Location:** `/Game/_PROJECT/PCG/` (target migration: `/Game/Melodia/PCG/` per FOUNDATION §2)

| Category | Graph | Test level | Stack |
|----------|-------|------------|-------|
| Baroque architecture | `PCG_BaroqueColonnade` | `L_PCGTest_Colonnade` | Vanilla PCG |
| Baroque architecture | `PCG_BaroqueRuins` | `L_PCGTest_Ruins` | Vanilla PCG |
| Baroque architecture | `PCG_Balustrade` | `L_PCGTest_Balustrade` | Vanilla PCG |
| Baroque architecture | `PCG_CathedralNave` | `L_PCGTest_Nave`, `L_PCGTest_Cathedral` | Vanilla PCG |
| Baroque architecture | `PCG_Cloister` | `L_PCGTest_Cloister` | Vanilla PCG |
| Baroque architecture | `PCG_OvergrownRuins` | (shares Ruins test) | Vanilla PCG |
| Baroque architecture | `PCG_GreyboxBlockout` | `L_PCGTest_Greybox` | Vanilla PCG |
| Escher / surreal | `PCG_PenroseShrine` | `L_PCGTest_Shrine` | Vanilla PCG |
| Escher / surreal | `PCG_EscherDecks` | `L_PCGTest_EscherDecks` | Vanilla PCG |
| Escher / surreal | `PCG_DreamWalls` | `L_PCGTest_DreamWalls` | Vanilla PCG |
| Escher / surreal | `PCG_BridgeArchipelago` | `L_PCGTest_Archipelago` | Vanilla PCG |
| Escher / surreal | `PCG_FloatingStairways` | `L_PCGTest_FloatingStairs` | Vanilla PCG |
| Nature / landscape | `PCG_MelodiaForest` | `L_PCGTest_Forest` | Vanilla PCG |
| Nature / landscape | `PCG_MelodiaForest_Landscape` | `L_PCGTest_ForestLandscape` | Vanilla PCG |
| Nature / landscape | `PCG_MeadowFalloff` | `L_PCGTest_Falloff` | Vanilla PCG |
| Nature / landscape | `PCG_ForestScatter_BS` (root) | `L_PCGTest_ForestScatter` | Vanilla PCG |
| Garden / paths | `PCG_TerraceGarden` | `L_PCGTest_TerraceGarden` | Vanilla PCG |
| Garden / paths | `PCG_WallGardenPath` | `L_PCGTest_WallGardenPath` | Vanilla PCG |
| Garden / paths | `PCG_SplinePath` | `L_PCGTest_SplinePath` | Vanilla PCG |

**Experiments** (`Graphs/_Experiments/PCGResearch/`): `PCG_M1_GrammarNave_BS`, `PCG_M1_GrammarNave_Inst_BS`, `PCG_FractalButtress_BS`, `PCG_CathedralBiome_BS`, `PCG_Forest_Biome_BS`, `PCG_APIProbe_BS`.

**Gap:** `L_PCGTest_GothicCorridors` exists with no matching graph — addressed by `PCG_GothicCorridorEx` below.

---

## 2. Expert review — existing vanilla graphs

### Strengths

- **Thematic cohesion:** All 18 production graphs share the Melusina baroque palette (`SM_wallhi_*`, MooaToon SDF/oil-paint materials).
- **One graph, one test level:** Fast iteration loop; matches FOUNDATION Phase 4 workflow.
- **Shape-grammar experiments:** `PCG_M1_GrammarNave_BS` and `PCG_FractalButtress_BS` align with FOUNDATION §5 (Foundation + Walls + Roof decomposition).
- **Zero broken mesh refs** after corruption recovery (FOUNDATION §1).

### Limitations (why PCGEx)

| Problem in vanilla graphs | PCGEx remedy |
|---------------------------|--------------|
| Column/wall rows placed by hand-tuned Transform chains | **Create Shapes** (Grid / Circle / Polygon) + **Resample Path** for exact module spacing |
| No explicit graph topology (adjacency, corridors) | **Clusters**, **Pathfinding**, **Flood Fill** for room graphs and nave aisles |
| Duplicated filter/transform chains per graph | **Uber Filter**, reusable **Filter / Heuristic sub-nodes** |
| Mesh lists duplicated in every Static Mesh Spawner | **Mesh Collections** (`PCGCol_Baroque_*`) with tags and weighted picks |
| Spline façades need offset cornices / balconies | **Offset Path**, **Copy To Paths**, **Path Hatch** |
| Walkability not validated in-graph | **Raycast Filter** + attribute checks (≤50 cm step, ≥200 cm path per FOUNDATION §5) |
| Escher systems lack structured connectivity | **Custom Graph Builder**, **Bridge** nodes between point sets |

### Architectural observations per graph

| Graph | Role today | PCGEx upgrade path |
|-------|------------|-------------------|
| `PCG_BaroqueColonnade` | Linear column runs | → `PCG_BaroqueColonnadeEx` (grid/circle shape + path resample) |
| `PCG_CathedralNave` | Longitudinal nave | → `PCG_BaroqueNaveVaultEx` (cluster ribs + aisle pathfinding) |
| `PCG_Cloister` | Courtyard ring | → `PCG_BaroqueAtriumEx` (polygon shape + inward-facing normals) |
| `PCG_Balustrade` | Railing scatter | → `PCG_BaroqueBalconyEx` (path offset + wallshort modules) |
| `PCG_BaroqueRuins` / `OvergrownRuins` | Breakup + nature | Keep nature half vanilla Biome; add PCGEx **Discard** / **Noise** for ruin masks |
| `PCG_GreyboxBlockout` | Layout probe | → feed **Create Shape Grid** seeds for all `*Ex` graphs |

---

## 3. PCGEx integration (project setup)

**Plugin:** `Plugins/PCGExtendedToolkit` v0.75.20 — enabled in `Melodia.uproject` as of 2026-06-15.

**Dependencies also enabled:** `GeometryScripting`, `PCGGeometryScriptInterop`.

**Docs:** [pcgex.gitbook.io](https://pcgex.gitbook.io/pcgex) · upstream [GitHub](https://github.com/Nebukam/PCGExtendedToolkit)

**After first open:** rebuild C++ modules, confirm PCGEx nodes appear under **PCGEx** category in the graph editor.

### Shared assets to create in editor

Create under `/Game/_PROJECT/PCG/Collections/`:

| Asset | Type | Purpose |
|-------|------|---------|
| `PCGCol_Baroque_Walls` | PCGEx Mesh Collection | Weighted `SM_wallhi_*` + mid/short/curved/window |
| `PCGCol_Baroque_Columns` | PCGEx Mesh Collection | Shaft modules (placeholder cubes until column meshes authored) |
| `PCGCol_Baroque_Cornice` | PCGEx Mesh Collection | Marble slab bands, ceiling squares |
| `PCGCol_Baroque_Roof` | PCGEx Mesh Collection | Hip roofs |
| `PCGCol_Baroque_Doors` | PCGEx Mesh Collection | Portal modules |
| `PCGCol_Baroque_Bridges` | PCGEx Mesh Collection | Venetian bridge, corridor spans |

Populate entries from `Scripts/PCG/baroque_mesh_catalog.json`. Assign `M_OilPainting_Gold_Baroque` or `M_SDF_TrueParallax` as material overrides where needed.

### Standard spawner pattern

On every **Static Mesh Spawner** node:

1. Add selector: **PCGEx Staged Mesh Selector**
2. Reference the appropriate `PCGCol_Baroque_*` collection
3. Enable **Template Descriptor** override for baroque MI when required

---

## 4. New baroque PCGEx systems (systematic family)

All new graphs live in `/Game/_PROJECT/PCG/Graphs/PCGEx/`. Naming: `PCG_<Feature>Ex` for top-level, `PCG_Sub_<Feature>` for reusable subgraphs. **No `_BS` suffix.**

### 4.1 Shared subgraphs (build first)

#### `PCG_Sub_BaroqueSpawn`

```
Input → Static Mesh Spawner (PCGEx Staged → PCGCol_Baroque_Walls) → Output
```

Parameters exposed on subgraph: Collection override, uniform scale, seed.

#### `PCG_Sub_BaroqueColumn`

```
Input → PCGEx Transform Points (scale Z = story height) → Spawner (PCGCol_Baroque_Columns) → Output
```

#### `PCG_Sub_BaroqueAlongPath`

```
Input (path points) → PCGEx Resample Path (distance = module_grid.wall_width) → PCGEx Orient → Sub_BaroqueSpawn → Output
```

### 4.2 Top-level systems

| Graph | Input | PCGEx node chain (summary) | Output |
|-------|-------|------------------------------|--------|
| **PCG_BaroqueColonnadeEx** | Surface or spline | Create Shapes (Grid: 1×N) → Break to paths → Resample Path (400 cm) → Sub_Column → Merge | Column arcade |
| **PCG_BaroqueFacadeEx** | Surface bounds | Create Shapes (Grid M×K) → Transform (face outward) → Tag rows → Sub_Spawn (wall tags by row) | Multi-story façade |
| **PCG_BaroqueRotundaEx** | Center point | Create Shapes (Circle, Fiblat) → Resample Path → Sub_Column → Sub_Cornice (offset path) | Circular colonnade |
| **PCG_BaroqueCorniceEx** | Path / spline | Spline To Path → Offset Path (+Z) → Resample Path → Spawner (PCGCol_Cornice) | Horizontal cornice band |
| **PCG_BaroquePilasterEx** | Façade grid points | Uber Filter (every 2nd column) → Transform (proud of wall) → Spawner (wallshort) | Vertical pilaster rhythm |
| **PCG_BaroqueBalconyEx** | Path at floor height | Offset Path (outward 120 cm) → Path Solidify (slab) → Spawner (wallshort + balustrade) | Balcony ledge row |
| **PCG_BaroqueNaveVaultEx** | Nave axis spline | Spline To Path → Path Hatch (rib spacing) → Cluster (path to cluster) → Spawner (cube/beam) | Vault rib lattice |
| **PCG_BaroqueAtriumEx** | Rectangular surface | Create Shapes (Polygon rectangle) → Sub_Column (perimeter) → Sub_Spawn (inner walls) → Flood Fill (courtyard void) | Cloister / atrium |
| **PCG_GothicCorridorEx** | Corridor spline | Spline To Path → Resample (400 cm) → Copy To Points (walls L/R via tensor offset) → Sub_Spawn | Fills `L_PCGTest_GothicCorridors` |
| **PCG_BaroqueEntryEx** | Portal point | Transform → Spawner (Doors) → Adjacent Sub_Spawn (wallhi arch surround) | Grand entry |

### 4.3 Graph parameters (cohesion)

All `*Ex` graphs expose these **PCG graph parameters** (pin overrides):

| Parameter | Default | Use |
|-----------|---------|-----|
| `ModuleWidth` | 400 | Column / wall module spacing (cm) |
| `StoryHeight` | 600 | Vertical module (cm) |
| `Seed` | 0 | Deterministic variation |
| `BaroqueCollection` | PCGCol_Baroque_Walls | Swappable collection |
| `WalkabilityCheck` | true | Enables raycast + step filter subgraph |

See cohesion doc for validation thresholds.

### 4.4 Mermaid — colonnade data flow

```mermaid
flowchart LR
  A[Input Surface] --> B[PCGEx Create Shapes Grid]
  B --> C[Break Clusters To Paths]
  C --> D[PCGEx Resample Path 400cm]
  D --> E[PCGEx Orient]
  E --> F[Sub_BaroqueColumn]
  F --> G[PCGEx Merge Points]
  G --> H[Output]
```

---

## 5. Test matrix (new)

| Graph | Test level | PCG volume setup |
|-------|------------|------------------|
| `PCG_BaroqueColonnadeEx` | `L_PCGTest_ColonnadeEx` | 20×8 m flat surface |
| `PCG_BaroqueFacadeEx` | `L_PCGTest_FacadeEx` | Vertical surface or thin box |
| `PCG_BaroqueRotundaEx` | `L_PCGTest_RotundaEx` | Single seed point |
| `PCG_BaroqueCorniceEx` | `L_PCGTest_CorniceEx` | Spline component in level |
| `PCG_BaroqueNaveVaultEx` | `L_PCGTest_NaveEx` | Long spline (reuse Nave test) |
| `PCG_GothicCorridorEx` | `L_PCGTest_GothicCorridors` | **Existing level** — assign new graph |
| `PCG_BaroqueAtriumEx` | `L_PCGTest_AtriumEx` | 30×30 m surface |

**Verify checklist (each graph):**

- [ ] Generate in editor (no empty output)
- [ ] Mesh refs resolve (0 broken)
- [ ] Module spacing visually consistent at 400 cm grid
- [ ] Walkability: no step > 50 cm on navigable floor tags
- [ ] Path width ≥ 200 cm where tagged `Path`
- [ ] Materials: baroque MI visible (not DefaultMaterial)

---

## 6. Automation — Python builder

Run from UE **Output Log → Python** or **Tools → Execute Python Script**:

```python
import importlib.util
spec = importlib.util.spec_from_file_location(
    "melodia_pcgex_builder",
    r"G:/Melodia/Scripts/PCG/melodia_pcgex_builder.py")
mod = importlib.util.module_from_spec(spec)
spec.loader.exec_module(mod)
mod.build_all()
```

This creates graph assets under `/Game/_PROJECT/PCG/Graphs/PCGEx/` with wired vanilla + PCGEx nodes. **Open each graph once** to assign mesh collection assets on spawners (collections are editor assets).

Optional: `mod.build_graph("PCG_BaroqueColonnadeEx")` for a single system.

---

## 7. Relationship to legacy graphs

- **Do not delete** existing vanilla graphs until each `*Ex` counterpart passes the test matrix.
- **Migration order:** ColonnadeEx → FacadeEx → GothicCorridorEx (unblocks orphaned test level) → NaveVaultEx → AtriumEx → RotundaEx → detail systems (Cornice, Pilaster, Balcony, Entry).
- **Escher graphs** (`PenroseShrine`, `BridgeArchipelago`, etc.) stay vanilla until Phase 5; PCGEx **Custom Graph** can be added later for gravity-zone connectivity.

---

## 8. Materials & aesthetic

Per `MATERIALS_LIBRARY_SPEC.md` §7:

- Façades: `M_SDF_TrueParallax`, `M_OilPainting_Gold_Baroque`
- Ornament: `M_SDF_Baroque`, `M_SDF_GildedStucco`, `M_SDF_GildedFiligree`
- Lavender / purple tonality, ornate baroque, surreal dream geometry — never photoreal

---

*Last updated: 2026-06-15. PCGEx enabled in project; graphs created via builder script require in-editor collection assignment and first generate pass.*
