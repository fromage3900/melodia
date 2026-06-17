# Melodia Battle Architecture

**Decision:** Melodia C++ combat kernel is authoritative. Phoenix (`BP_BattleController`) is the **presenter** for units, animations, and **skill/command UI (Option B)**. You own replacing or restyling Phoenix widgets in Blueprint.

## Ownership

| Layer | Owns |
|-------|------|
| `UMelodiaBattleSession` | Encounter lifecycle, battle phase, command routing, win/lose/flee |
| `UMelodiaCombatStateComponent` + `UMelodiaBattleLoopLibrary` | SP, HP, toughness, elements, damage math |
| `UMelodiaRhythmExecutionComponent` | Note highway after skill selection |
| `UMelodiaJRPGPresenter` | Phoenix `StartBattle`, unit arrays, optional UI teardown |
| Phoenix Blueprints | Unit meshes, skill menus, damage text — wire into session hooks |

## Battle phases (`EMelodiaBattlePhase`)

```
None → AwaitingPlayerCommand ⇄ RhythmExecution
              ↓                      ↓
         EnemyTurn              (skill highway)
              ↓
    Victory / Defeat / Fled → None (explore)
```

Loop phase mapping (`AMelodiaRhythmGameModeBase`):

| Battle phase | `EMelodiaLoopPhase` |
|--------------|---------------------|
| AwaitingPlayerCommand, RhythmExecution, EnemyTurn | Battle |
| Victory | VictoryReward |
| None, Defeat, Fled | ExplorationReady |

## Phoenix UI (Option B)

- `bSuppressPhoenixBattleUI` on game mode defaults to **false** — Phoenix BattleUI / skill dialogue stay up until you replace them.
- When **true**, `UMelodiaJRPGPresenter` strips Phoenix widgets after init (Melodia-only HUD mode).

## Blueprint hooks (wire your Phoenix UI here)

On `BP_BattleController`, `BP_SkillUseDialogue`, or custom widgets:

| Call | When |
|------|------|
| `UMelodiaBattleSession::SubmitBasicCommand()` | Player picks Attack |
| `UMelodiaBattleSession::SubmitSkillCommand(SkillId)` | Player picks a songwriting skill from Phoenix menu |
| `UMelodiaBattleSession::SubmitUltimateCommand()` | Ultimate selected |
| `UMelodiaBattleSession::SubmitFleeCommand()` | Run / flee |
| `UMelodiaBattleSession::ConfirmVictoryReward()` | Reward claimed (auto after ~1.25s if skipped) |

Get session: `GetGameInstance → GetSubsystem(UMelodiaBattleSession)`.

**Skill flow (Option B):** Phoenix skill menu → `SubmitSkillCommand` → sets active skill → opens rhythm highway → damage on finish.

Dev keyboard (`1/2/4/Esc`) still works via `UMelodiaBattleInputComponent` and delegates to the same session methods.

## Encounter start

`AMelodiaEncounterTrigger::StartEncounter` → `UMelodiaBattleSession::BeginEncounter(FMelodiaEncounterDefinition)`.

Presenter runs `StartBattle`; kernel resets native combat state; game mode enters battle loop phase.

## HUD modes (`EMelodiaHUDMode`)

| Mode | When |
|------|------|
| Exploration | Explore / boot |
| BattleCompact | Player command phase (default compact bars) |
| BattleHighway | Rhythm execution active |
| Victory / Defeat | End states |

Set by session when phase changes. Replace Melodia native paint or hide it when your Phoenix UI is ready.

## Roadmap (remaining)

1. ~~**DataAssets**~~ — `UMelodiaSongSkillDataAsset`, `UMelodiaEncounterDataAsset` + `UMelodiaContentRegistrySubsystem` (demo fallback merged)
2. ~~**Party subsystem**~~ — `UMelodiaPartySubsystem` (Melusina + cockatoo slot, battle SP buff)
3. **Retire reflection** — stop mirroring `Rhythm*` properties on `BP_BattleController`; presenter syncs visuals only
4. **Loop verifier** — session subsystem present (partial); add PIE encounter session phase checks

## Content authoring (new)

| Asset | Path | Purpose |
|-------|------|---------|
| `UMelodiaSongSkillDataAsset` | `/Game/Melodia/Data/Skills/` | Override demo skill by matching `SkillId` |
| `UMelodiaEncounterDataAsset` | `/Game/Melodia/Data/Encounters/` | `DefaultSlime` encounter metadata |

Registry scans these paths on game instance init. No assets yet = full demo catalog still works.

## Phoenix skill menu (Option B)

```
Get Subsystem (Melodia Battle Session)
  → Get Unlocked Skills        // populate menu
  → Can Submit Skill Command   // grey out locked
  → Submit Skill Command       // on confirm
```

## File index

| File | Role |
|------|------|
| `MelodiaBattleTypes.h` | Phases, encounter def, HUD mode |
| `MelodiaBattleSession.*` | Kernel orchestrator (GameInstance subsystem) |
| `MelodiaJRPGPresenter.*` | Phoenix adapter |
| `MelodiaJRPGBridgeLibrary.*` | Unit HP sync + UI teardown (used by presenter) |
| `MelodiaBattleLoopLibrary.*` | Damage / turn rules (called by session) |
