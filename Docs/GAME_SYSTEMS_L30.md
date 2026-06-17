# Melodia Lv1–30 Game Systems (C++ Developer Review)

Native authoritative systems for the JRPG + rhythm hybrid demo band (Mechanic Levels 1–30). Blueprint (`BP_BattleController`) remains the Phoenix battle host; C++ owns rhythm damage, progression, elements, and highway execution.

## Architecture Overview

```
Exploration (AMelodiaRhythmGameModeBase)
  ├── UMelodiaMechanicProgressionSubsystem  (skills, keys, companion, presets)
  ├── AMelodiaCompanionActor                (cockatoo, Lv8+)
  ├── AMelodiaNPCBase ×3                    (tier tutors)
  └── AMelodiaQuestManagerBase              (starter + progression quests)

Battle (BP_BattleController + native components)
  ├── UMelodiaBattleInputComponent          (1=Basic JRPG, 2=Skill highway, Tab/Shift+2=cycle)
  ├── UMelodiaRhythmExecutionComponent      (note highway from FMelodiaSongSkillRecipe)
  ├── UMelodiaCombatStateComponent          (SP, toughness, elements, companion flag)
  └── UMelodiaBattleLoopLibrary             (damage, element multipliers, enemy turns)
```

## Core Types (`MelodiaSpellTypes.h`)

| Type | Purpose |
|------|---------|
| `EMelodiaSpellElement` | Forte, Tide, Gale, Stone, Radiant, Umbral, Arcane |
| `FMelodiaElementKeyDefinition` | Equippable key metadata (unlock level, element) |
| `FMelodiaSongSkillRecipe` | In `MelodiaSongSkillLibrary.h` — pitch pattern, instrument, element, SP |

## Songwriting Skills (`MelodiaSongSkillLibrary`)

- `BuildDemoSongSkills()` — 30 skills, one per Mechanic Level 1–30.
- Varying `EMelodiaInstrument`, `EMelodiaSpellElement`, and pitch/duration patterns.
- Unlocked into `FMelodiaMechanicProgressionState.UnlockedSkillIds` on level-up.
- `ActiveSkillId` cycled via Tab or Shift+2 in battle.

## Element Keys (`MelodiaKeySystemLibrary`)

**Weakness wheel (cycle):** Forte → Gale → Stone → Tide → Radiant → Umbral → Arcane → Forte

| Constant | Value |
|----------|-------|
| Weakness | ×1.5 |
| Resistance | ×0.75 |
| Matching key on weakness | ×1.25 additional |

Seven keys unlock at Lv **3, 6, 9, 12, 15, 18, 21**.

`GetEnemyElementForEncounterLevel(Level)` seeds enemy element in `ResetRhythmBattleEncounter`.

## Combat Rules (`MelodiaCoreRulesLibrary`)

- `FMelodiaGeneratedSpell.SpellElement` — derived from pitch average + instrument in `GenerateSpellFromSong`.
- `DeriveSpellElementFromSong()` — deterministic element from composition.
- `CalculateElementalDamageMultiplier()` — delegates to key system.
- `GetElementDisplayName()` — localized element names for HUD.

## Battle Flow

1. **Basic (1 / Space)** — classic JRPG hit via `ApplyRhythmBattleAction` (no highway, no element bonus).
2. **Skill (2)** — `BeginSkillExecution()` loads active skill from progression subsystem → rhythm highway → `ApplyRhythmExecutionResult` applies average grade × skill power × **element multiplier**.
3. **Element key** — `CombatState.EquippedKeyElement` synced from progression; +25% when key element matches skill element on a weakness hit.

## Companion (`AMelodiaCompanionActor`)

- Unlocks at **Mechanic Lv8** (`bCompanionUnlocked`).
- Follows player in exploration (`TickFollow`).
- Battle: `ApplyBattleStartBuff` grants **+1 SP** at encounter start.
- Optional `IMelodiaInteractable` pet interaction.
- Blueprint child: **`BP_CockatooCompanion`** (reparent from `AMelodiaCompanionActor`).

## Quest NPCs (`AMelodiaNPCBase`)

- Blueprintable base implementing `IMelodiaInteractable`.
- `QuestId` + `ActivateInteraction` → `AMelodiaQuestManagerBase::ActivateQuest`.
- Game mode spawns 3 tier tutors via `EnsureProgressionNPCs()`.
- Blueprint children: **`BP_NPC_TierTutor`**, **`BP_NPC_MoonTutor`**, etc.

## Progression (`UMelodiaMechanicProgressionSubsystem`)

State fields (also in `UMelodiaSaveGame`):

- `UnlockedSkillIds`, `UnlockedKeyIds`, `ActiveSkillId`
- `EquippedKeyElement`, `bCompanionUnlocked`

`UnlockContentUpToLevel()` runs on init, load, and level-up.

## Quest Chain (`RegisterProgressionQuestChain`)

| Quest | Level | Type |
|-------|-------|------|
| KeyOfForte | 3 | Equip Forte key |
| WeaknessWaltz | 3 | Land weakness hit |
| SongwritingMastery | 5 | Use skills ×5 |
| CockatooCompanion | 8 | Unlock companion |
| Tutor_TierII/IV/VI | 6/16/26 | Reach mechanic level |
| Collect_Key_* | 6–21 | Equip each harmonic key |

## Save / Load

`UMelodiaSaveGame` persists skills, keys, active skill, equipped element, companion flag alongside existing mechanic XP/level/presets.

## Editor Setup

### BP_CockatooCompanion

1. Content Browser → Add Blueprint Class → parent **`AMelodiaCompanionActor`**.
2. Path: `/Game/Melodia/Characters/Companions/BP_CockatooCompanion`.
3. Assign mesh/VFX on `VisualMesh`; tune `FollowDistance`, `BonusSkillPointsAtBattleStart`.
4. Set `CompanionClassPath` on `BP_MelodiaGameMode` (or `AMelodiaRhythmGameModeBase` defaults).

### BP_NPC_* (tier tutors)

1. Add Blueprint Class → parent **`AMelodiaNPCBase`**.
2. Set `QuestId` (e.g. `Tutor_TierII`), `DisplayName`, `DialogueLine`, `RequiredMechanicLevel`.
3. Place in level or rely on `EnsureProgressionNPCs()` spawn.
4. Set `ProgressionNPCClassPath` on game mode for custom art.

### BP_BattleController

- Keep existing Phoenix JRPG host; native `UMelodiaBattleInputComponent` + `UMelodiaRhythmExecutionComponent` are added at runtime by game mode.
- **Orchestrator:** `UMelodiaBattleSession` (see `Docs/BATTLE_ARCHITECTURE.md`).
- Phoenix skill UI (Option B): call `SubmitSkillCommand(SkillId)` from your menus.
- Rhythm damage authority stays in `UMelodiaBattleLoopLibrary`.

## File Index (this feature)

| File | Role |
|------|------|
| `MelodiaSpellTypes.h` | Element enum, key struct |
| `MelodiaSongSkillLibrary.*` | 30 demo skills |
| `MelodiaKeySystemLibrary.*` | Weakness wheel, keys, enemy seeding |
| `MelodiaCompanionActor.*` | Cockatoo companion |
| `MelodiaNPCBase.*` | Quest tutor NPC base |
| `MelodiaCoreRulesLibrary.*` | Spell element on generated spells |
| `MelodiaCombatStateComponent.h` | Enemy/key/companion battle fields |
| `MelodiaMechanicProgressionSubsystem.*` | Unlock + save integration |
| `MelodiaSaveGame.h` | Persistence |
| `MelodiaRhythmExecutionComponent.*` | Skill highway by SkillId |
| `MelodiaBattleInputComponent.*` | Input + skill cycle HUD |
| `MelodiaBattleLoopLibrary.*` | Element damage in combat |
| `MelodiaQuestManagerBase.*` | Progression quest chain |
| `MelodiaRhythmGameModeBase.*` | Companion + NPC bootstrap |
| `MelodiaMechanicProgressionLibrary.cpp` | Tier blurbs mention skills+keys |
