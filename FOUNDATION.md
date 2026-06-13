# MELODIA MELUSINA — PROJECT FOUNDATION
*Clean-slate project at `G:\Melodia`. Single source of truth. Created 2026-06-13 after migrating off the corrupted `MelodiaMelusina_PROD` (mass `_BS` rename/delete damage — see note §7). This file replaces the 25-doc sprawl; design specs are summarized here and the originals carried in `/Docs`.*

---

## 1. Migration status (disk-verified 2026-06-13)
**In the new project already (~2,238 assets):**
- ✅ Materials: 04_Materials (1,316), MATERIALLAYERS (24), UPDATEDINSTANCES (18) + MooaToon core (MLB_ToonBaseBlend, ML_ToonBaseInput, MF_*, T_*_Linear)
- ✅ Meshes: MelusinasHouse (354/356), houseassets (337)
- ✅ PCG: 25 (the 18 graphs — already repointed to Melusina palette + 0 broken refs — plus test levels)
- ✅ assets (72), VFX (18), Art (20), Tools (20), Systems (3), Data, Structures, Blueprints(2)

**LEFTOVER — DONE 2026-06-13 (filesystem copy, since vibeue bridge was offline):**
- ✅ `_PROJECT/Songcraft` (5): DA_Tree_{Drums,Harp,MusicBox,Trumpet,Violin}
- ✅ Core system BPs: BP_MusicManager, BP_RhythmInputValidator, BP_RhythmHUD, WBP_RhythmHUD, BP_RhythmTestBattleManager, BP_SongcraftManager, BP_InstrumentData, BP_InstrumentTreeData, BP_Melusina
- ✅ Gameplay BPs: rhythm skills (Crescendo/Fortissimo/Harmony/Resonance/Tempo/Diminuendo/Discordant/Pianissimo), quest-givers (BeatKeeper/HarmonyMaster/Maestro), Dialogue, Companion, Forest set, SaveBed, SurrealCathedral, rhythm quests
- ⚠️ 2 corrupt MelusinasHouse meshes skipped (rebuild if needed)
- ⚠️ Copied BPs may show broken refs on first open (deps like IA_Jump, BP_BattleBase, BP_ItemBase were DELETED in the corruption — no survivor). They're carried as scaffolding; rebuild from §4 spec, don't expect them to compile clean immediately.
- NOTE: copy = functional equivalent of migrate for a one-way move (deps already present); .uasset resolves on first open.

**Combat template (TurnBased JRPG) — restored CLEAN 2026-06-13:**
- ✅ Copied the **pristine, pre-damage** template from `G:\ueprojects\TurnBasedjRPGTemplate` (409 assets, 0 corruption) → `G:\Melodia\Content\TurnBasedJRPGTemplate` (correct /Game paths).
- All core BPs intact: BP_BattleBase, BP_BattleController, BP_JRPGPlayerController, BP_PlayerUnitBase, BP_ItemBase, BP_TargetIcon, BP_UnitBase, BP_BattleSkillBase, BP_BattleUI.
- ⚠️ This is the **vanilla** template — the rhythm hooks you'd added to it in the old project (damage × CurrentRhythmMultiplier in CalculateDamage; ShowRhythmUI/ApplyRhythmModifier on BP_BattleSkillBase) are NOT here and must be **re-applied from §4A spec**. Small, documented work — and cleaner than salvaging the corrupted custom versions (which were throwing load errors).
- ⚠️ Keep TurnBasedJRPGTemplate at its current path — its GameMode/PlayerController are referenced in project settings (.ini) by path; do NOT fold it into /Game/Melodia.
- ❌ NOT chosen: rebuilding combat from scratch on GitHub HSR repos — months of work, and those are study refs (often Unity/C++), not drop-in UE systems. HSR stays a *design* reference layered on this template (AV/SP/Ultimate already in MASTER_PLAN).

---

## 2. Clean folder hierarchy (target — reorganize in new project via rename_asset)
```
/Game/Melodia/
  Core/
    Rhythm/        BP_MusicManager, BP_RhythmInputValidator, BP_RhythmHUD
    Songcraft/     BP_SongcraftManager, BP_Instrument*, DA_Tree_*, DA_Song/Spell/Material
    Battle/        BP_BattleManager (AV turn economy), BP_SkillBase  (rebuild clean)
    Characters/    BP_Melusina (+ player/companion)
  Materials/       M_* masters → Toon/ SDF/ Water/ Baroque/ OilPaint/ FX/
    Layers/        ML_* · MLB_* · MF_* · MPC_MusicClock
    Instances/     MI_*
    Textures/
  Meshes/          SM_* → Architecture/ (wallhi, towers, bridges) · Nature/ · Props/
  PCG/             PCG_* graphs · TestLevels/
  UI/              WBP_* (HUD, dialogue, songcraft composer, menus)
  Art/             VFX/ FX/ decals
  Audio/
  Levels/
  Docs/            design specs carried from old project
  _Reference/      imported GitHub study material (NEVER shipped; clearly quarantined)
```
**MooaToon stays a plugin (`/MooaToon/…`)** — never copied into /Game. This alone prevents the whole class of "deleted /Game copy" breakage that started this mess.

## 3. Naming / labeling conventions (enforced)
`M_` material · `MI_` instance · `ML_` layer · `MLB_` layer-blend · `MF_` material-function · `MPC_` param-collection · `T_` texture · `SM_` static mesh · `BP_` blueprint · `WBP_` widget · `DA_` data-asset · `PCG_` pcg graph · `L_` level · `NS_` niagara · `S_`/`SC_` sound.
Rules: **no `_BS`/`_BSS`/`SceneImport_`, no spaces, no diacritics** (`SM_BézierCurve`→`SM_BezierCurve`). One asset = one canonical name. Folder = role, not author.

---

## 4. CORE SYSTEMS — lock in (canonical spec)

### 4A. Rhythm Combat (from RHYTHM_SYSTEM.md) — "one clock, beat rewards never punishes"
- **BP_MusicManager** — Quartz clock; `StartBattleMusic(Sound,BPM)`, `GetBeatPhase()0–1`, `GetTimeToNextBeatMs()`; dispatchers `OnBeatTick/OnBar/OnSongSection`. Drives MPC_MusicClock.BeatPulse (materials already react).
- **BP_RhythmInputValidator** — `GradeInputNow()→{Perfect ±90ms, Good ±160ms, Miss}`; calibration tap → LatencyOffsetMs.
- **BP_RhythmHUD / WBP_RhythmHUD** — PulseRing, JudgmentText, CrescendoMeter.
- **Turn economy (AV):** AV cost = 10000/SPD; lowest-AV acts, resets; `OnTurnOrderChanged`.
- **SP:** pool 5 shared; basic +1, skill −1.
- **Damage:** `Base × (0.4 + {Good 0.6 / Perfect 1.1})` → Miss 0.4×, Good 1.0×, Perfect 1.5×.
- **Crescendo→Ultimate:** +5/Perfect, never drained; full → fire anytime (interrupt AV, extra turn).
- **Instruments:** MusicBox 1.0× / Violin 0.9× DoT / Drums 1.15× AoE / Harp 1.0× heal / Trumpet 1.3× nuke.

### 4B. Songcraft (from SONGCRAFT_MAGIC_SYSTEM.md) — "music IS the magic"
- Compose sheet music (notes+durations+beat pattern) + inscribe materials → deterministic spell (seed = hash(notes+materials)).
- Data: **`DA_Song`** (NoteSequence, NoteDurations, TimeSig, Instrument, MaterialSlots, CompositionHash), **`DA_SpellInstance`** (name, dmg, hits, SPcost, effects, secondary), **`DA_Material`** (rarity, ModifierMap, SynergyPairs).
- Note pitch→effect type, duration→speed, beat pattern→RNG; material rarity→secondary/mutation.
- 5 instrument skill trees gate note range / material slots / complexity.
- Phase 1 target: 4-beat compose UI, 3 slots, 3 rarity tiers, deterministic generation, cast in battle.

> These two specs are the spine. Everything else (PCG world, art) serves them. Lock the data-asset structs + manager BPs first; build UI second.

---

## 5. GitHub resources — honest integration
The curated repos (UE5-Procedural-Building, ProceduralMeshDemos, shape-grammar papers, Escher tools) are **C++/study references, not drop-in assets** (their own note: "Greybox ≠ asset; greybox is a workflow"). Integration = **extract patterns, not import projects**:
- Shape-grammar decomposition (Building=Foundation+Walls+Roof) → PCG graph structure.
- Recursive golden-ratio scaling (0.618×, depth 3) → PenroseShrine / nested chambers.
- Constraint-based walkability (floor exists, ≤50cm step, ≥200cm path) → PCG validation.
- Escher gravity-zones + impossible bridges → BridgeArchipelago / DreamWalls.
Action: keep links in `/Docs/GREYBOX_RESOURCES_CURATED.md`; if any actual asset is downloaded, it goes in `/Game/Melodia/_Reference/` (never referenced by shipping content). Do NOT bulk-import marketplace C++ into the clean project.

---

## 6. Methodical execution plan
- **Phase 1 — finish migration**: ✅ DONE 2026-06-13 (filesystem copy). All essential content now in `G:\Melodia\Content`.
- **Phase 2 — reorganize** (new project open): move migrated assets into the §2 hierarchy via `rename_asset` (redirectors keep refs alive); fix up redirectors; strip stray `_BS`/SceneImport leftovers.
- **Phase 3 — lock core systems**: open the new project; verify Rhythm + Songcraft manager BPs compile; rebuild the corrupt Battle/Skill BPs clean from §4 spec; create the `DA_Song/Spell/Material` structs if missing.
- **Phase 4 — PCG + levels**: graphs already themed (§ PCG_SYSTEM_COHESION); add one clean test level; Generate-verify (in-editor, not via bridge).
- **Phase 5 — vertical slice**: one playable beat-combat encounter using one instrument + one songcraft spell, on a small PCG zone. First milestone of the rebuild.

## 7. Why we're here (do not repeat)
A past automated bulk op mass-renamed thousands of assets to `_BS` (some `BP_`-prefixed into Blueprints/Gameplay) and deleted others, corrupting filename↔assetname links and breaking references project-wide. Recovery was partial (materials restored via MooaToon twins; PCG repointed). Lesson baked into this foundation: **plugin content stays in plugins; never bulk-rename/delete across the project; keep this clean tree + conventions; commit to version control with `.uasset` actually tracked (Git LFS) from day one.**
