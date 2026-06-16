# PCG System Cohesion — 2026-06-13

Cross-graph conventions so baroque PCG systems read as one Melusina world. Referenced from `FOUNDATION.md` §6 Phase 4 and `PCG_SYSTEMS_GUIDE.md`.

---

## 1. Module grid (canonical)

All architecture PCG graphs — vanilla and PCGEx — align to this grid unless a graph explicitly documents an exception (Escher shrink systems use 0.618× golden ratio).

| Dimension | Value (cm) | Notes |
|-----------|------------|-------|
| Wall module width | **400** | Matches typical `SM_wallhi` segment length |
| Story height | **600** | Floor-to-cornice single story |
| Column center spacing | **400** | Colonnade inter-column axis |
| Minimum path width | **200** | FOUNDATION walkability |
| Maximum step height | **50** | FOUNDATION walkability |
| Cornice offset (Z) | **580** | Just below story cap |
| Balcony offset (XY) | **120** | Proud of façade plane |
| Pilaster every N modules | **2** | Every second grid column |

**PCG graph parameters:** expose as `ModuleWidth`, `StoryHeight`, `Seed` on all `PCG_*Ex` graphs.

---

## 2. Mesh collections (single source)

**Catalog file:** `Scripts/PCG/baroque_mesh_catalog.json`  
**Editor assets:** `/Game/_PROJECT/PCG/Collections/PCGCol_Baroque_*`

| Collection | Tags used in graphs | Graphs |
|------------|---------------------|--------|
| `PCGCol_Baroque_Walls` | `base`, `mid`, `short`, `curved`, `window`, `corner` | FacadeEx, Sub_Spawn, GothicCorridorEx, Ruins |
| `PCGCol_Baroque_Columns` | `shaft` | ColonnadeEx, RotundaEx, AtriumEx, NaveEx |
| `PCGCol_Baroque_Cornice` | `band`, `ceiling_tile` | CorniceEx, RotundaEx, NaveEx |
| `PCGCol_Baroque_Roof` | `hip` | AtriumEx, Greybox (future) |
| `PCGCol_Baroque_Doors` | `portal` | EntryEx, GothicCorridorEx |
| `PCGCol_Baroque_Bridges` | `span`, `corridor` | BridgeArchipelago (future PCGEx pass) |

**Rule:** Never embed raw mesh lists in Static Mesh Spawner nodes — always reference a collection.

---

## 3. Point attributes (shared vocabulary)

Write these attributes consistently so filters and downstream graphs compose:

| Attribute | Type | Set by | Meaning |
|-----------|------|--------|---------|
| `ModuleIndex` | int32 | Resample Path | Index along a path |
| `StoryIndex` | int32 | FacadeEx grid | Floor level (0 = ground) |
| `ArchitecturalRole` | string | Various | `Column`, `Wall`, `Cornice`, `Door`, `Path`, `Void` |
| `Walkable` | bool | Walkability subgraph | Navigable floor |
| `GridX`, `GridY` | int32 | Create Shapes Grid | Façade coordinates |
| `ShapeId` | int32 | Create Shapes | Per-shape identity |

---

## 4. Walkability validation subgraph

Reuse `PCG_Sub_WalkabilityCheck` on any graph tagged navigable:

```
Input (floor points, Walkable=true)
  → PCGEx Raycast Filter (down 100cm, hit floor)
  → PCGEx Uber Filter (step height ≤ 50cm vs neighbor)
  → PCGEx Discard (path width < 200cm between Walkable clusters)
  → Output ( culled bad points OR flag attribute )
```

Non-navigable décor (cornice, balustrade, vault ribs) **must not** set `Walkable=true`.

---

## 5. Naming & paths

| Item | Convention |
|------|------------|
| Production graph | `PCG_<Name>` or `PCG_<Name>Ex` |
| Subgraph | `PCG_Sub_<Name>` |
| Test level | `L_PCGTest_<Name>` |
| Collection | `PCGCol_Baroque_<Category>` |
| Folder (current) | `/Game/_PROJECT/PCG/Graphs/PCGEx/` |
| Folder (target) | `/Game/Melodia/PCG/` |

**Forbidden:** `_BS`, `_BSS`, `SceneImport_`, spaces, diacritics in new assets.

---

## 6. Cohesion between vanilla ↔ PCGEx

| Vanilla graph | PCGEx counterpart | Shared collection | Migration status |
|---------------|-------------------|-------------------|------------------|
| `PCG_BaroqueColonnade` | `PCG_BaroqueColonnadeEx` | Columns + Walls | Builder ready |
| `PCG_CathedralNave` | `PCG_BaroqueNaveVaultEx` | Columns + Cornice | Builder ready |
| `PCG_Cloister` | `PCG_BaroqueAtriumEx` | Columns + Walls + Roof | Builder ready |
| `PCG_Balustrade` | `PCG_BaroqueBalconyEx` | Walls (short) | Builder ready |
| *(missing)* | `PCG_GothicCorridorEx` | Walls + Doors | Builder ready |
| `PCG_GreyboxBlockout` | feeds all `*Ex` seeds | n/a | Manual link |

Both families may coexist in one level during Phase 4 — use **separate PCG volumes** or **merge** via PCGEx Merge Points at the end.

---

## 7. Seed & determinism

- **Default seed:** graph parameter `Seed=0`; increment per zone in level blueprint for variety.
- **Roguelike tie-in** (`SIMULATED_UNIVERSE_DESIGN.md`): zone seed = hash(run_seed, zone_id) — PCG volumes should read seed from actor parameter when placed in run maps.

---

## 8. Test level orphan resolution

| Level | Issue | Resolution |
|-------|-------|------------|
| `L_PCGTest_GothicCorridors` | No graph asset | Assign `PCG_GothicCorridorEx` |
| `L_PCGTest_ZoneDraft` | Zone draft only | Wire to `PCG_GreyboxBlockout` + FacadeEx combo |

---

## 9. Phase 4 acceptance (cohesion gate)

Phase 4 complete when:

1. All `PCGCol_Baroque_*` collections exist and match JSON catalog  
2. At least **ColonnadeEx + GothicCorridorEx + FacadeEx** pass test matrix  
3. Walkability subgraph validates on one navigable graph  
4. Zero broken mesh refs on new graphs  
5. One vertical-slice zone uses a single `*Ex` graph (Phase 5)

---

*Snapshot date: 2026-06-13 (cohesion rules). PCGEx graphs added 2026-06-15.*
