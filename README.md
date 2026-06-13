# Melodia Melusina — Game Project

A rhythm-combat, procedurally-generated world built in Unreal Engine 5.7.2 with MooaToon stylized shading. **Music IS the magic.**

## Quick start

```bash
# Clone and open
git clone <repo-url>
cd Melodia
# Open Melodia.uproject in UE 5.7.2
```

**Prerequisites:**
- Unreal Engine 5.7.2 (MooaToon precompiled engine)
- MooaToon plugin (included)
- Git LFS (for `.uasset` / `.umap` files)

## Project structure

See `FOUNDATION.md` for the complete architecture, naming conventions, and core-system specs.

**Current state (as of 2026-06-13):**
- ~2,238 migrated assets (materials, meshes, PCG, gameplay)
- Folder hierarchy at `/Game/_PROJECT/` and scattered legacy paths — **reorganization into `/Game/Melodia/` tree is Phase 2** (in-editor via `rename_asset`)
- Rhythm combat system scaffolded (BPs migrated, hooks need re-applying)
- Songcraft system scaffolded (5 instrument trees + manager)
- 18 PCG graphs themed to Melusina palette, 0 broken mesh refs
- Clean Git + LFS from 2026-06-13 onward

## What's next

1. Open `Melodia.uproject` in UE, verify it loads
2. Check Project Settings (GameMode/default maps may need repointing)
3. Re-apply rhythm hooks to combat BPs (§4A in FOUNDATION.md)
4. Reorganize folders into `/Game/Melodia/` hierarchy (in-editor, Phase 2)
5. Build first vertical slice: one beat-combat encounter + songcraft spell

## Core systems

**Rhythm Combat:** One-clock beat tracking (Quartz), AV turn economy (10000/SPD), SP pool (5 shared), damage scaling by input grade (0.4×–1.5×), Crescendo→Ultimate.

**Songcraft:** Compose sheet music + inscribe materials → deterministic spell generation. 5 instrument skill trees, phase 1 target is 4-beat compose UI, 3 slots, 3 rarity tiers.

See `FOUNDATION.md` §4 for canonical specs.

## Aesthetic inspiration

- **MC Escher:** impossible geometry, recursive tessellation, non-Euclidean spaces
- **Infinity Nikki:** walkable beauty, scale consistency, silhouette clarity, material richness without geometry
- **Japanese stylized rendering:** SDF face shadows, toon shading, anime character lighting (see RESEARCH_shaders_procedural.md)

## GitHub resources (study refs, not drop-in assets)

Curated procedural/shader research in `Docs/RESEARCH_shaders_procedural.md`:
- Inigo Quilez (SDFs, raymarching), Shadertoy, weiyinfu/Fractal, L-Systems
- Shape grammars, golden-ratio recursion, constraint-based walkability
- Japanese rendering talks + BUKKBEEK stylized VFX approach
- **Integration discipline:** one technique at a time on test assets, after foundation is locked

## Root cause lesson

This project was rebuilt from a catastrophic corruption (mass `_BS` asset rename/delete + 89GB restore desync). Git + LFS from day one is non-negotiable. See `FOUNDATION.md` §7 for the postmortem.

## License

(Specify your license here)

---

Built with 🎵 rhythm, 🎨 procedural art, and 💜 Melusina's surreal aesthetic.
