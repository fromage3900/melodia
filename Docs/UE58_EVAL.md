# Melodia — UE 5.8 evaluation fork

Separate track from **production** (`fromage3900/melodia` on MooaToon UE 5.7).

**GitHub:** [fromage3900/melodia-ue58](https://github.com/fromage3900/melodia-ue58)  
**Branch:** `ue5.8-eval` (default on the fork)

## Why this exists

Evaluate stock **Unreal Engine 5.8** without blocking MooaToon portfolio work:

| Feature | MooaToon 5.7 (production) | UE 5.8 (this fork) |
|---------|---------------------------|---------------------|
| Toon / stylized look | MooaToon SDF materials | In-engine toon shader |
| Vegetation | PCG mesh scatter + custom | Updated procedural vegetation |
| Engine | Precompiled fork, VRM4U | Stock Epic install |
| Risk | Known-good for Melodia | Migration / plugin churn |

**Decision rule:** stay on MooaToon until 5.8 toon + PCG smoke tests match portfolio quality on `PCG_DreamWalls` and hero materials.

## Prerequisites

1. Install **UE 5.8** from Epic Launcher (Windows).
2. Clone the eval repo (not the production repo for day-to-day 5.8 work):

   ```powershell
   git clone https://github.com/fromage3900/melodia-ue58.git G:\Melodia-UE58
   cd G:\Melodia-UE58
   git checkout ue5.8-eval
   ```

3. Right-click `Melodia.uproject` → **Switch Unreal Engine version** → **5.8** (regenerates project files).

## First open checklist

1. **Disable or replace MooaToon-only plugins** in `Melodia.uproject` if compile fails:
   - `VRM4U` (required on MooaToon 5.7; may be absent on stock 5.8)
   - Any MooaToon engine-path assumptions in docs/scripts
2. **PCGExtendedToolkit** — confirm a 5.8-compatible release; pin version in `Plugins/PCGExtendedToolkit/`.
3. Regenerate VS project and build:

   ```bat
   "C:\Program Files\Epic Games\UE_5.8\Engine\Build\BatchFiles\Build.bat" MelodiaMelusina_PRODEditor Win64 Development -Project="G:\Melodia-UE58\Melodia.uproject" -WaitMutex
   ```

   Adjust the engine path to your Epic install.

4. Open editor; run `Melodia.BuildAllPCG` and open `L_PCGTest_DreamWalls` + `L_MelodiaPortfolioTerrace`.

## Evaluation matrix (fill in as you test)

| Test | Pass? | Notes |
|------|-------|-------|
| Project opens on UE 5.8 | | |
| C++ module builds | | |
| MooaToon gold/stucco look vs 5.8 toon on hero mesh | | |
| `PCG_DreamWalls` generates (ISM > 0) | | |
| `PCG_PortfolioTerraceBezier` / LevelKit | | |
| PCGEx graphs compile + generate | | |
| Procedural vegetation (5.8) vs PCG scatter | | |
| Packaging Win64 | | |

## Syncing with production

Production PCG fixes land on `fromage3900/melodia` (`master`). Pull them into the eval fork:

```powershell
cd G:\Melodia-UE58
git remote add production https://github.com/fromage3900/melodia.git
git fetch production
git merge production/master   # resolve uproject / plugin conflicts on ue5.8-eval
```

Or use `Scripts/Setup/sync-ue58-fork.ps1` from either clone.

## Scope boundaries

- **Do not** replace MooaToon on production until eval matrix passes.
- **Do** keep 5.8-only experiments (toon material instances, vegetation graphs) on this fork.
- Large binary Content may differ between clones; use Git LFS or manual content sync if needed.

## Related docs

- `RECOVERY.md` — MooaToon 5.7 build/open (production only)
- `SDF_TOON58_LOOKDEV.md` — UE 5.8 Substrate Toon look-dev materials + builder script
- `PCG_SYSTEMS_GUIDE.md` — graph inventory (`PCG_DreamWalls`, Bezier portfolio kit)
- `PORTFOLIO_PIPELINE.md` — shot list / demo levels
