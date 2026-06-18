# SDF / Toon58 look-dev (UE 5.8 eval)

Basic **Substrate Toon** materials for comparing stock UE 5.8 stylized shading against production **MooaToon SDF** (`Content/_PROJECT/04_Materials/MooaToon/`).

| Track | Engine | Toon path |
|-------|--------|-----------|
| Production | MooaToon UE 5.7 | `M_SDF_*`, `M_OilPainting_*` |
| UE 5.8 eval | Stock Epic 5.8 | `/Game/Melodia/Materials/SDF/Toon58/` |

See also: `Docs/UE58_EVAL.md`, `Docs/MATERIALS_LIBRARY_SPEC.md` §2.2.

---

## Fork / branch

| Item | Value |
|------|-------|
| Eval repo | [fromage3900/melodia-ue58](https://github.com/fromage3900/melodia-ue58) |
| Expected clone | `G:\Melodia-UE58` (sibling; optional) |
| **This workspace** | `G:\Melodia` on branch **`ue5.8-eval`** |
| Engine | Stock **UE 5.8** (`C:\Program Files\Epic Games\UE_5.8`) |

If `G:\Melodia-UE58` is not cloned, work on `G:\Melodia` `ue5.8-eval` — same eval track.

---

## Project settings (required once)

`Config/DefaultEngine.ini` enables Substrate for this branch:

```ini
[/Script/Engine.RendererSettings]
r.Substrate=True
r.Substrate.ProjectGBufferFormat=0   ; Blendable GBuffer (recommended for Toon NPR)
```

After changing Substrate settings: **restart the editor**.

In **Project Settings → Rendering**:

1. Enable **Substrate materials**
2. **Substrate GBuffer Format** → **Blendable** (value `0`)
3. Confirm **Lumen** stays on (default in Melodia)

---

## Assets created

### Toon profiles

| Asset | Path |
|-------|------|
| Default | `/Game/Melodia/Materials/SDF/Toon58/Profiles/TP_Toon58_Default` |
| Stucco (walls/rim) | `/Game/Melodia/Materials/SDF/Toon58/Profiles/TP_Toon58_Stucco` |
| Gold (accent) | `/Game/Melodia/Materials/SDF/Toon58/Profiles/TP_Toon58_Gold` |

Edit ramps in the Toon Profile asset (diffuse + specular LUTs). Assign per material or instance.

### Master material

| Asset | Role |
|-------|------|
| `M_Toon58_SDFMaster` | Substrate **Toon BSDF** on **Front Material** + faux-SDF world-aligned panel bands |

**Graph (summary):**

```
BaseTint, AccentTint, SDF_BandScale, SDF_BandStrength (parameters)
  → WorldPosition.XY × BandScale → Sin → Abs
  → Lerp(BaseTint, AccentTint) × BandStrength + BaseTint
  → Substrate Toon BSDF (DiffuseColor)
  → Front Material
```

Parameters:

| Name | Group | Default | Purpose |
|------|-------|---------|---------|
| `BaseTint` | Toon58 | Lavender stucco | Main surface color |
| `AccentTint` | Toon58 | Deeper lavender | Panel / band highlight |
| `SDF_BandScale` | SDF | 0.035 | World-space band frequency |
| `SDF_BandStrength` | SDF | 0.22 | Band contrast (0 = flat) |

### Look-dev instances

| Instance | Use | MooaToon analogue |
|----------|-----|-------------------|
| `MI_Toon58_Wall` | Façade / PCG walls | `M_SDF_TrueParallax` |
| `MI_Toon58_Floor` | Terrace / paving | `M_SDF_GildedStucco` (muted) |
| `MI_Toon58_Accent` | Gold trim, columns | `M_OilPainting_Gold_Baroque` |
| `MI_Toon58_Rim` | Dark outline / shadow trim | Rim proxy (no post outline yet) |

All under: `/Game/Melodia/Materials/SDF/Toon58/`

---

## Build script

**Script:** `Scripts/Materials/melodia_toon58_lookdev_builder.py`

Creates profiles, master, and four instances. Safe to re-run (deletes and rebuilds master/instances; reuses existing profiles).

### Run in editor

1. Open `Melodia.uproject` with **UE 5.8** (not MooaToon 5.7).
2. Restart editor if you just pulled Substrate config changes.
3. **Tools → Execute Python Script** → select the builder, **or** Output Log:

   ```
   py "G:/Melodia/Scripts/Materials/melodia_toon58_lookdev_builder.py"
   ```

4. Check log for `=== Melodia Toon58 look-dev build complete ===`.

### Command line (headless, optional)

```bat
"C:\Program Files\Epic Games\UE_5.8\Engine\Binaries\Win64\UnrealEditor-Cmd.exe" ^
  "G:\Melodia\Melodia.uproject" -unattended -stdout ^
  -run=pythonscript -script="G:\Melodia\Scripts\Materials\melodia_toon58_lookdev_builder.py"
```

Close the interactive editor first to avoid asset lock conflicts.

---

## Manual material setup (if script fails)

Use this when `MaterialExpressionSubstrateToonBSDF` is missing (wrong engine version).

1. **Content Browser** → `/Game/Melodia/Materials/SDF/Toon58/`
2. Create **Toon Profile** asset (`TP_Toon58_Default`).
3. Create **Material** `M_Toon58_SDFMaster`.
4. Material details: enable **Substrate** / use Substrate output (no legacy Base Color root).
5. Add **Substrate Toon BSDF** node.
6. Connect **Diffuse Color** ← `BaseTint` vector parameter (or constant).
7. Connect BSDF → **Front Material**.
8. On the BSDF or material: assign **Toon Profile**.
9. Compile & save.
10. Create **Material Instance** children; override `BaseTint`, `SDF_*`, and per-instance Toon Profile.

---

## How to test in editor

### Quick viewport swatch

1. Place a **Cube** or `SM_wallhi` mesh in any test level.
2. Assign `MI_Toon58_Wall` to slot 0.
3. Add a **Directional Light** + **Sky Light** (Lumen on).
4. Compare side-by-side with `M_SDF_TrueParallax` on a duplicate mesh.

### Portfolio / PCG levels

| Level | Action |
|-------|--------|
| `L_PCGTest_DreamWalls` | Open level → select PCG volume → after generate, override ISM material to `MI_Toon58_Wall` / `MI_Toon58_Accent` |
| `L_MelodiaPortfolioTerrace` | Swap terrace/wall material overrides in PCG component or placed kit meshes |
| `Melodia.BuildAllPCG` | Console: regenerate graphs, then assign Toon58 instances on hero ISMs |

### PCG catalog mapping (eval)

For A/B vs MooaToon, map `Scripts/PCG/baroque_mesh_catalog.json` materials:

| Catalog key | Production | Toon58 eval |
|-------------|------------|-------------|
| `facade_primary` | `M_SDF_TrueParallax` | `MI_Toon58_Wall` |
| `facade_gold` | `M_OilPainting_Gold_Baroque` | `MI_Toon58_Accent` |
| `ornament_sdf` | `M_SDF_Baroque` | `MI_Toon58_Accent` |
| `stucco` | `M_SDF_GildedStucco` | `MI_Toon58_Wall` |
| `filigree` | `M_SDF_GildedFiligree` | `MI_Toon58_Rim` |

### Eval checklist (fill in `UE58_EVAL.md`)

- [ ] Substrate Toon compiles without errors
- [ ] Wall/floor/accent read as baroque lavender + gold at portfolio camera distance
- [ ] PCG `PCG_DreamWalls` ISM count > 0 with Toon58 materials
- [ ] Side-by-side vs MooaToon gold/stucco on hero mesh

---

## Blockers / prerequisites

| Issue | Mitigation |
|-------|------------|
| `VRM4U` missing on stock UE 5.8 | **Disabled** in `Melodia.uproject` on `ue5.8-eval` |
| Headless `UnrealEditor-Cmd` slow / hangs | Run `py` script in the **open** editor instead |
| Substrate off until restart | Pull `DefaultEngine.ini` change → restart editor once |
| MooaToon SDF parity | This kit is **look-dev only** — bands, not raymarched SDF |

---

- **No MooaToon-style mesh outline** in this kit — `MI_Toon58_Rim` is a dark toon ramp, not post-process edge detection.
- **No raymarched SDF** — bands are procedural UV-world stripes; full `M_SDF_*` parity needs a later pass.
- Toon BSDF does **not** blend with Substrate Slab layers yet (Epic forum; experimental).
- Indirect light can soften cel steps; tune Toon Profile diffuse ramp shadow region for flatter look.
- **Restart required** after first `r.Substrate=True`.

---

## Files in this change

| File | Purpose |
|------|---------|
| `Scripts/Materials/melodia_toon58_lookdev_builder.py` | Editor Python builder |
| `Docs/SDF_TOON58_LOOKDEV.md` | This doc |
| `Config/DefaultEngine.ini` | `r.Substrate=True` for ue5.8-eval |

---

*Last updated: 2026-06-18*
