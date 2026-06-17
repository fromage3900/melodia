# Melodia — Editor-ready state

Use this to confirm the project is ready for **content authoring in the editor** (skills, areas, weapons, portals, quests).

## Short answer

**Yes — with caveats.** The editor opens, PIE runs, and native systems bootstrap for full development mode through **mechanic Lv 30**. Songwriting skills, elemental keys, companion, and progression NPCs are wired in C++; assign art in child Blueprints (see `GAME_SYSTEMS_L30.md`). Phoenix battle **UI** skill selection is still a dev shortcut (`2` / Tab).

## Launch checklist

1. Build native code (MooaToon UE 5.7) — see `RECOVERY.md` §2.
2. Open via `Open Melodia Editor.bat` (not double-click `.uproject`).
3. Default map: `L_MelodiaRhythm` (`Config/DefaultEngine.ini`).
4. Default GameMode: `BP_MelodiaRhythmGameMode` → parent `MelodiaRhythmGameModeBase`.

### GameMode flags (important)

| Flag | Default | Purpose |
|------|---------|---------|
| `bMinimalDemoMode` | **false** | When **true**, strips map actors and skips Reverie/PCG bootstrap — smoke test only. Use PIE URL `?MinimalDemo` if needed. |
| `bUsePCGPlacement` | **false** | When **true**, rest/portal/encounters snap to PCG walkable index. Enable after you have a PCG volume + graph in the level. |
| `bRunLoopVerifier` | **false** | Self-check actor on BeginPlay — enable when debugging loop wiring. |

**If PIE still behaves like the old minimal demo:** open `BP_MelodiaRhythmGameMode` → Class Defaults → uncheck **Minimal Demo Mode** → Compile → Save.

## What works today (PIE)

| System | Status | How to extend |
|--------|--------|----------------|
| **Exploration** | ✅ WASD, camera, E interact, I inventory | Reparent hero to `MelodiaCharacterBase`; see `CHARACTER_SETUP.md` |
| **Encounters** | ✅ `AMelodiaEncounterTrigger` → Phoenix battle bootstrap | Place triggers or enable PCG spawner |
| **Battle** | ✅ Basic attack, flee, skill→rhythm highway, enemy turn, win/lose | Phoenix `BP_BattleController` + native loop library |
| **Quests** | ✅ `AMelodiaQuestManagerBase` / `BP_QuestManager` | Child Blueprints, `Notify*` hooks, HUD quest log |
| **Inventory / weapons** | ✅ `UMelodiaInventoryComponent` on pawn | Add items via `AddItem`; reward from quests |
| **Portals** | ✅ `AMelodiaPortal` (spawned or placed) | Set `TargetLevelName` or `TargetWorldLocation` |
| **Rest / save** | ✅ `AMelodiaRestPoint` | Bed save flow via native rest point |
| **PCG areas** | ✅ Custom nodes (Escher, Gravity Zone, Tessellation, Recursive Arch) | Rebuild editor; see `PCG_SYSTEMS_GUIDE.md` |
| **Mechanic level (demo Lv 1–30)** | ✅ Tiers, 30 presets, 30 songwriting skills, 7 keys, quests + save | `Docs/MECHANIC_PROGRESSION.md`, `Docs/GAME_SYSTEMS_L30.md` |
| **Songwriting skills → rhythm** | ✅ Unlocked by mechanic level; **2** fires active skill highway | Tab / Shift+2 cycle active skill |
| **Element keys / weakness** | ✅ Weakness wheel + equipped key bonus in battle | `MelodiaKeySystemLibrary` |
| **Companion (cockatoo)** | ✅ Native `AMelodiaCompanionActor`, unlock Lv8 | Child `BP_CockatooCompanion` for mesh/VFX |
| **Progression NPCs** | ✅ Three tier tutors auto-spawned | Child `BP_NPC_TierTutor` for art/dialogue |
| **Portfolio map** | ✅ Pipeline facet index + env art reel guide | `Docs/PORTFOLIO_PIPELINE.md` |
| **Jump + glide** | ✅ `UMelodiaGlideComponent` on Melusina | Space jump, hold to glide |
| **Pickable flowers** | ✅ `AMelodiaPickableFlower`, F to pick, quest | Blossom Gatherer |
| **HUD** | ✅ HSR-style native Slate HUD + custom fonts | `Docs/UI_FONTS.md` |
| **Music / rhythm clock** | ✅ `AMelodiaMusicManager` Quartz BPM | Assign tracks in `BP_MelodiaQuartzMusicManager` |

## Not finished yet (plan in `GAMEPLAY_LOOP_PLAN.md`)

- Phoenix battle **UI** for skill pick (keyboard `2` / Tab remain dev shortcuts).
- Full **Phoenix turn order** as single source of truth (native + template dual model).
- **Child Blueprints** for cockatoo mesh and NPC art (`BP_CockatooCompanion`, `BP_NPC_*`).
- Multi-enemy polish, packaging for game target (MooaToon precompiled).

These gaps do **not** block PIE through Lv 30 or extending native/Blueprint content.

## PIE smoke test (5 min)

1. Play `L_MelodiaRhythm`.
2. Explore with WASD; quest log top-left, minimap top-right.
3. Walk to song gate (encounter trigger) → battle starts.
4. **Space/1** basic, **4** flee, **2** skill (rhythm highway from active songwriting skill).
5. **Tab** or **Shift+2** cycle unlocked skills; check HUD prompt for skill name + element.
6. Win → reward prompt → back to explore; tier tutors and companion appear as you level (Lv8 companion).
7. **E** at rest point / portal / NPC if in range; **F** pick flowers.

## Content authoring entry points

| Goal | Start here |
|------|------------|
| New skills / spells | `/Game/TurnBasedJRPGTemplate/`, `/Game/Melodia/Data/`, `CONTENT_AUTHORING_GUIDE.md` |
| New quests | Child of `BP_QuestBase` or extend `AMelodiaQuestManagerBase` catalog |
| New areas | Level + PCG graph; set `bUsePCGPlacement` when walkable index exists |
| Weapons / items | `UMelodiaInventoryComponent`, quest rewards |
| Portals | Place `AMelodiaPortal` or configure spawned portal on GameMode |

## Engine note

Use **MooaToon precompiled UE 5.7** only. Stock Epic UE_5.7 fails (missing `VRM4U`).
