# Melodia editor recovery (no MCP)

Use this after a hung editor, failed automated import, or stuck Live Coding. **Do not use Unreal MCP / VibeUE bulk import** until manual steps below succeed.

## 1. Clean shutdown

1. Save work in any other apps, then close Unreal Editor normally if possible.
2. If the editor is frozen, end the process:
   ```powershell
   taskkill /F /IM UnrealEditor.exe
   ```
3. Wait until `tasklist /FI "IMAGENAME eq UnrealEditor.exe"` reports no tasks.
4. Optional: delete `Saved/Logs/Melodia.log` only if you need a fresh log (not required).

## 2. Build native code (before opening editor)

From a **Developer PowerShell for VS 2022** or cmd:

```bat
"G:\MooaToon-main\MooaToon-main\MooaToon-Engine-Precompiled\Windows\Engine\Build\BatchFiles\Build.bat" MelodiaMelusina_PRODEditor Win64 Development -Project="G:\Melodia\Melodia.uproject" -WaitMutex
```

- Engine: **MooaToon precompiled** (UE 5.7). Do not use stock Epic UE_5.7 for this project.

## 3. Start editor (manual only)

**Use `Open Melodia Editor.bat` in the project root** — not a double-click on `Melodia.uproject`.

Double-clicking `.uproject` goes through Epic's VersionSelector and can spawn **two editor instances** plus a stuck Live Coding "processing source file changes" dialog.

1. Run `G:\Melodia\Open Melodia Editor.bat` (single instance, Live Coding off, `-Unattended` to suppress startup modals).
2. Do **not** run MCP import tools on startup.
3. Disable **Live Coding** for heavy skeletal imports: *Editor Preferences → General → Live Coding → uncheck Enable Live Coding* (re-enable after imports).

### Startup hang fixes (applied in repo)

If the editor opens but stays **Not Responding** before the viewport appears:

| Fix | Where |
|-----|--------|
| **UnrealMCP disabled** | `Melodia.uproject` — re-enable only after editor is stable |
| **No restore-tabs prompt** | `RestoreOpenAssetTabsOnRestart=False` in saved + default `EditorPerProjectUserSettings.ini` and launch script `-ini:` overrides |
| **No startup content scan** | `bDetectChangesOnStartup=False` (same files + launch script) |
| **Skip EOS SDK blocking** | `Config/DefaultEngine.ini` → `[OnlineSubsystem] DefaultPlatformService=NULL`, `[OnlineSubsystemNull] bEnabled=true` |

The launcher script also passes `-Unattended` (safe for editor recovery; suppresses modal dialogs that can block the UI thread while hidden).

## 4. ARP skeletal mesh import (manual)

**Source FBX (already in repo tree):**

`G:\Melodia\Content\Import\Melusina\WorkingMelusinaScene.fbx`

**Destination content path:** `/Game/Melodia/Characters/Melusina/`

1. In Content Browser, go to `Content/Melodia/Characters/Melusina/` (create folders if missing).
2. **Import** → select `WorkingMelusinaScene.fbx`.
3. Import options (Auto-Rig Pro / humanoid):
   - **Skeleton:** create new skeleton (e.g. `SK_Melusina_Prototype`).
   - **Import Mesh:** on.
   - **Import Animations:** **off** for this step (rig-only).
   - **Transform:** leave at origin unless ARP export notes say otherwise.
4. After import, open the skeletal mesh, confirm materials/textures from `WorkingMelusinaScene.fbm/` if prompted.
5. Create **Animation Blueprint** `ABP_Melusina` parented to the new skeleton (empty state machine is fine for now).

## 5. Mocap animations (one at a time)

**Source folder:** `G:\Mocap\` (local mocap FBX files, not in git).

Import **one FBX per session**; wait for shader compile / asset registry to settle (~30–60s) before the next file.

1. Right-click skeleton or `/Game/Melodia/Characters/Melusina/` → **Import**.
2. Select a single `.fbx` from `G:\Mocap\`.
3. Options:
   - **Skeleton:** existing `SK_Melusina_Prototype` skeleton (must match ARP bone names).
   - **Import Mesh:** off if the file is animation-only.
   - **Import Animations:** on.
4. Name sequences clearly (e.g. `AM_Melusina_Idle`, `AM_Melusina_Walk`).
5. Repeat for each file; **never** batch-import the whole folder via MCP while recovering.

Reference inventory script (optional): `Scripts/ImportMelusinaMocap.ps1` (lists `G:\Mocap` files; run without MCP).

## 6. Reparent Melusina Blueprint to native base

Goal: exploration pawn uses cosmetics, inventory, and input components from C++.

1. Open existing Melusina hero Blueprint (e.g. `BP_Melusina` or `BP_Melusina_Hero` under `/Game/Melodia/Characters/Melusina/`).
2. **File → Reparent Blueprint** (or Class Settings → Parent Class).
3. Choose **`MelodiaCharacterBase`** (`/Script/MelodiaMelusina_PROD.MelodiaCharacterBase`).
4. Compile and fix warnings:
   - Assign **Skeletal Mesh** → `SK_Melusina_Prototype`.
   - Assign **Anim Class** → `ABP_Melusina`.
   - Capsule: use **Fit Aggressively** or match mesh bounds if collision is wrong.
5. On the character defaults, leave enabled where desired:
   - `bAutoApplyCosmetics`
   - `bSeedStarterInventory`
   - `bAutoBindExplorationInput`
6. Set as default pawn in your exploration GameMode if not already wired.

## 7. What caused the hang (avoid next time)

- **UnrealMCP / VibeUE** plugin init (HTTP server on port 8088) blocking early startup.
- Hidden **Restore open asset tabs?** modal (`RestoreOpenAssetTabsOnRestart=AlwaysPrompt`).
- **EOS SDK** config fetch on frame 2 (mitigated via NULL online subsystem in `DefaultEngine.ini`).
- Automated **Unreal MCP** skeletal import while Live Coding / asset registry was busy.
- Large FBX + simultaneous mocap imports.

Prefer: **kill hung editor → apply fixes above → build → launch via `.bat` → manual single imports → reparent Blueprint → then** re-enable UnrealMCP/automation.

If the editor still hangs after these fixes, try once with `-nullrhi` on the command line to see whether the block is UI/logic vs. GPU/RHI init.

## 8. Quick checklist

- [ ] No `UnrealEditor.exe` running
- [ ] `MelodiaMelusina_PRODEditor` build succeeded
- [ ] `SK_Melusina_Prototype` imported from `Content/Import/Melusina/`
- [ ] Mocap from `G:\Mocap` imported one-by-one
- [ ] Hero Blueprint parent = `MelodiaCharacterBase`
- [ ] PIE smoke test (locomotion + input)

