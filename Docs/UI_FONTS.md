# Melodia HUD fonts

Runtime Slate HUD loads TTF files directly from `Content/Melodia/UI/Fonts/` (no `.uasset` required for native paint).

| File | Role |
|------|------|
| `LifeSavers-Regular.ttf` | Body / quest lines |
| `LifeSavers-Bold.ttf` | Stats, SP, command labels |
| `CherryBombOne-Regular.ttf` | Titles, banners, damage popups |
| `TwinkleStar-Regular.ttf` | Accent labels (ULT, minimap) |
| `NotoMusic-Regular.ttf` | Rhythm / note highway hints |

## Optional editor import (UMG / Blueprint widgets)

If you want the same fonts in UMG `TextBlock` widgets:

1. Content Browser → **Import** each `.ttf` into `Content/Melodia/UI/Fonts/`.
2. Create a **Font** asset (Right-click → User Interface → Font) referencing the imported face.
3. Assign in `WBP_RhythmHUD` text widgets.

Native `UMelodiaRhythmHUDWidget` painting uses `MelodiaHUDFonts` automatically after `ApplyCuteCombatTheme()`.

## Sprites (Kenney CC0)

Decor PNGs live in `Content/Melodia/UI/Sprites/Kenney/` (star, spark, magic, window frame).  
Source repo: https://github.com/Calinou/kenney-particle-pack (CC0 1.0).

Native HUD loads these at runtime via `MelodiaHUDDecor` (no texture import required).
