# Melusina character setup (placeholder + final rig)

Use this guide while the custom ARP rig is being simplified. The gameplay loop does **not** depend on Melusina's mesh — only exploration pawn + components.

## Project safety checklist

| Area | Status |
|------|--------|
| C++ rhythm / JRPG loop | Built and wired (`MelodiaRhythmGameModeBase`, Phoenix bridge) |
| Default map / game mode | `L_MelodiaRhythm` + `BP_MelodiaRhythmGameMode` in `DefaultEngine.ini` |
| Exploration pawn | `BP_Melusina` (fallback: native `AMelodiaCharacterBase`) |
| VRM4U | Enabled in `Melodia.uproject` (requires `IKRig` + `OSC`) |
| Custom FBX import | **Manual only** — do not bulk-import via UnrealMCP (hangs editor) |
| Shipping build | Blocked on precompiled MooaToon (Editor/PIE OK) |

## When you return to the editor

1. Close any stuck `dotnet` / UBT processes if a command-line build was interrupted.
2. Open `Melodia.uproject` — allow plugin reload for **VRM4U**, **IKRig**, **OSC**.
3. Press **Play** on `L_MelodiaRhythm` to confirm the loop before importing assets.

## VRM placeholder (recommended)

**Best CC0 pick:** VRoid **AvatarSample_B** (β Ver AvatarSample_1) — bob cut, anime proportions, no usage restrictions.

- FAQ + download: https://vroid.pixiv.help/hc/en-us/articles/360012381793
- Alternate CC0 pack (VRM 0.x): https://opengameart.org/content/vroid-studio-cc0-models (`avatarsample_d.zip`, `sendagaya_shino.zip`, etc.)

VRM4U imports **VRM 0.x and 1.0**. Older 0.x samples work without conversion.

### Import steps (VRM4U)

1. Download a `.vrm` file to `Content/Import/Placeholder/` (keep out of git if large).
2. In Content Browser: create folder `/Game/Melodia/Characters/Placeholder/`.
3. Drag the `.vrm` into the folder (or **Import** → select `.vrm`).
4. VRM4U creates a skeletal mesh + materials under that path.
5. Open `BP_Melusina` (or duplicate as `BP_Melusina_Hero`):
   - **Reparent** to `MelodiaCharacterBase` if not already.
   - Assign imported skeletal mesh to the **Mesh** component.
   - Optional: set **Placeholder Skeletal Mesh** on the class defaults (runtime fallback via `EnsureDisplayMesh()`).
6. Create a simple **AnimBP** from the VRM skeleton (locomotion blend space later).
7. Set **Game Mode → Default Pawn** to your hero BP if you created a new child.

### Capsule / scale

VRM avatars are often ~1.6 m. `AMelodiaCharacterBase` uses capsule half-height **88**, radius **34**. After import, use **Set Character Properties** in BP or adjust mesh relative location so feet sit on the ground.

## Custom rig (when ready)

Local copy (safe path, not OneDrive):

```
Content/Import/Melusina/WorkingMelusinaScene.fbx
```

Mocap: `G:\Mocap\*.fbx`

**Import manually** in Skeletal Mesh Import (one file at a time):

1. Import rig → `/Game/Melodia/Characters/Melusina/SK_Melusina`
2. Create ABP from skeleton
3. Reparent / configure `BP_Melusina_Hero`
4. Import animations one-by-one with 2–3 s between imports

See `Scripts/ImportMelusinaMocap.ps1` for file inventory and ordering.

## Native character base

`AMelodiaCharacterBase` provides:

- `UMelodiaCosmeticsComponent`
- `UMelodiaExplorationInputComponent` (E interact, I inventory)
- `UMelodiaInventoryComponent`
- Walk speed 450, orient rotation to movement
- `EnsureDisplayMesh()` — loads `PlaceholderSkeletalMesh` if mesh slot empty
- `ApplyMelusinaPresentation()` — cosmetics + HUD sparkle theme

Game mode calls `ApplyMelusinaPresentation()` after battle return when pawn is `AMelodiaCharacterBase`.

## What to avoid

- UnrealMCP `import_skeletal_mesh` on the full Melusina FBX (editor freeze)
- Importing entire mocap folders in one batch
- Enabling Game packaging target on precompiled MooaToon
