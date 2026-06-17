# Mechanic level progression (demo Lv 1–30)

Player **Mechanic Level** gates unlockable **tiers** and **location presets** for Reverie/PCG authoring.

## Tiers (6 bands × 5 levels)

| Tier | Levels | Name |
|------|--------|------|
| I | 1–5 | Novice Star |
| II | 6–10 | Moon Apprentice |
| III | 11–15 | Comet Adept |
| IV | 16–20 | Aurora Virtuoso |
| V | 21–25 | Nebula Maestro |
| VI | 26–30 | Celestial Legend |

## Location presets

- **30 presets** (`Lv01_StarlitAtrium` … `Lv30_MaestroSanctum`) — one unlocked per mechanic level.
- Each preset includes: display name, build notes, suggested PCG graph soft path, encounter counts, difficulty.
- Assign or swap PCG graphs in `UMelodiaMechanicProgressionSubsystem::LocationPresetCatalog` or via data asset later.

## XP sources

| Event | XP |
|-------|-----|
| Battle victory | 45 |
| Quest complete | 35 |

Formula for next level: `60 + CurrentLevel × 25`.

## Quest integration

Demo quests in `RegisterMechanicDemoQuests()`:

- **Rising Melody** → Lv 5  
- **Moon Apprentice** → Lv 10  
- **Comet Adept** → Lv 15  
- **Aurora Virtuoso** → Lv 20  
- **Nebula Maestro** → Lv 25  
- **Demo Maestro** → Lv 30  
- **Unlock: Escher Entry** / **Maestro Sanctum** — preset unlock objectives  

## Editor / Blueprint

- Subsystem: `UMelodiaMechanicProgressionSubsystem` (GameInstance).
- Apply presets to run: `ApplyUnlockedPresetsToReverieRunManager`.
- `AMelodiaReverieRunManager::AreaTemplates` is filled on bootstrap from unlocked presets.

## Save

Rest at Melusina's bed persists `MechanicLevel`, `MechanicXP`, and `UnlockedLocationPresetIds` in `UMelodiaSaveGame`.

## Automation test

`Melodia.MechanicProgression.DemoLevel30` — validates 30 presets, 6 tiers, XP level-up, Reverie config conversion.
