# Melodia — Core Gameplay Loop: Review & Plan to a Working Game

Continues codex cycle `019ece28-14d2-7c22-af65-f253fce63741` (the native "rhythm vertical slice" in `Source/MelodiaMelusina_PROD/`).

**Target loop:** explore → encounter slime → enter battle → skill tutorial → cast skill → rhythm bar HUD appears → execute skill as a rhythm minigame driven by the *songwriting skill* → damage calculation → enemy turn → loop until end.

---

## 1. What the codex cycle already built (C++ inventory)

| Module | Role | State |
|--------|------|-------|
| `UMelodiaCoreRulesLibrary` | Deterministic grading, damage, AV/SP economy, **songcraft `GenerateSpellFromSong`** | Solid, unit-tested (`MelodiaCoreRulesTests.cpp`) |
| `UMelodiaBattleLoopLibrary` | Battle glue: basic/skill/ultimate, toughness/break, follow-up, victory, quest notify | Works on a reflection-driven model; **see issues §3** |
| `UMelodiaCombatStateComponent` | Reactive state: ultimate gauge, SP, toughness/break, enemy turn-delay | Solid |
| `AMelodiaMusicManager` | Quartz beat clock (`OnBeatTick`, `GetBeatPhase`, `GetTimeToNextBeatMs`) | Solid |
| `AMelodiaRhythmGameModeBase` | Bootstraps clock + HUD + trigger + input + quest + verifier | Solid; spawns at hardcoded coords |
| `AMelodiaEncounterTrigger` | Sphere overlap → `StartEncounter` → battle controller | Works |
| `UMelodiaBattleInputComponent` | Binds Attack/Skill/Ultimate → loop library | Works |
| `UMelodiaRhythmHUDWidget` | Native HUD with ~25 BlueprintNativeEvent hooks | Works (after §2 fix) |
| `AMelodiaQuestManagerBase` | `NotifyBattleWon` hook | Works |
| `AMelodiaLoopVerifier` | PIE self-check of the loop | Works |

**Assessment:** the *scaffolding is genuinely impressive* — the explore→battle→victory→explore phase machine exists and is verifiable. The gap is that the **battle is one-sided and not yet driven by songwriting**, and there are correctness issues that block a clean build.

---

## 2. Build blocker found & fixed (this cycle)

`MelodiaBattleLoopLibrary.cpp` called `Widget.SetEnemyTurnDelay(...)` in 3 places, but the method **did not exist** on `UMelodiaRhythmHUDWidget` → the module would fail to compile.

**Fixed:** added `SetEnemyTurnDelay` (BlueprintNativeEvent + `_Implementation`) and its state fields (`LastEnemyTurnDelayStacks`, `LastEnemyTurnDelayApplied`, `EnemyTurnDelayUpdateCount`) to the widget.

> **Action:** rebuild `MelodiaMelusina_PROD` and confirm 0 errors before any further work. Also verify Quartz links (`AudioMixer` is in `Build.cs`; `UQuartzClockHandle` resolves).

---

## 3. Gap analysis vs the target loop

| Loop beat | Status | Gap |
|-----------|--------|-----|
| **Explore** | ✅ GameMode restores control to Melusina pawn | Needs a real playable explore map (currently hardcoded coords in template Gameplay map) |
| **Encounter slime** | ⚠️ `AMelodiaEncounterTrigger` exists | No slime enemy actor/visual; trigger spawns a battle controller, not a themed slime encounter |
| **Enter battle** | ✅ In-place battle start (no level load) | OK |
| **Skill tutorial** | ❌ Not present | Need a first-run gated tutorial overlay before first skill cast |
| **Cast skill** | ⚠️ Skill action exists | Uses a flat `1.75` scalar — **not connected to songcraft** |
| **Rhythm bar HUD appears** | ⚠️ HUD always present (loose) | No *note highway that opens when a skill is selected* (Theatrythm tight-coupling, see `RHYTHM_SYSTEM.md` §7) |
| **Rhythm game from songwriting skill** | ❌ Missing link | `GenerateSpellFromSong` output (NotePitches/Durations/HitCount/Power) is **never fed into the rhythm execution**. The song should *become* the note pattern you play. |
| **Damage calculation** | ✅ `CalculateRhythmDamage` + loop library | Wire it to per-note grades × spell power instead of one grade |
| **Enemy turn** | ❌ Cosmetic only | Enemy never damages the player; no AV turn order actually scheduling enemy actions; no player HP/defeat |
| **Loop until end** | ⚠️ Victory loop works | No *defeat* path; no multi-enemy / multi-turn termination beyond single enemy HP |

### Architectural risk: dual source of truth
The native `MelodiaBattleLoopLibrary` reads/writes **loose `Rhythm*` properties by reflection** onto the template `BP_BattleController` *and* mirrors them in `UMelodiaCombatStateComponent`. The template's own `CalculateDamage`/`DealDamage`/turn order is a **separate** combat brain. Two brains = drift, double-counting, and the enemy-turn gap.

**Decision required (recommended):** make the **native reactive model the single source of truth for the rhythm slice**. Treat `BP_BattleController` as a *presentation/turn host* (camera, unit actors, turn order UI), and route all damage/economy through `MelodiaBattleLoopLibrary` + `MelodiaCombatStateComponent`. Stop writing gameplay-critical numbers into template properties except where the template renders them.

---

## 4. Plan — phased to a playable vertical slice

Ordering favors *one fully playable loop* over breadth. Each phase ends in a PIE-verifiable checkpoint (extend `AMelodiaLoopVerifier`).

### Phase 0 — Stabilize the build (½ day)
- [x] Fix missing `SetEnemyTurnDelay` (done this cycle).
- [ ] Rebuild module, run `MelodiaCoreRulesTests` automation suite (green).
- [ ] PIE the template Gameplay map; confirm `AMelodiaLoopVerifier` reports the loop wiring passes and the beat print fires.

### Phase 1 — Make the enemy turn real (the biggest missing half) (2–3 days)
Goal: combat is two-sided and can be lost.
- [ ] Add player vitals to `UMelodiaCombatStateComponent` (`PartyHP`, `PartyMaxHP`) and an `EMelodiaLoopPhase::EnemyTurn`.
- [ ] In `MelodiaBattleLoopLibrary`, after a player action resolves (and enemy HP > 0), schedule an **enemy turn**: consume `EnemyTurnDelayStacks` (break delays/skips the enemy), else apply `EnemyIntentPower` as damage to `PartyHP`.
- [ ] Drive ordering with the existing `CalculateAVCost`/`AddAVCost` (AV = 10000/SPD) so turn order is deterministic, not "player spams".
- [ ] Add a **defeat** resolution (`PartyHP <= 0`) → `EMelodiaLoopPhase` defeat → retry/return.
- [ ] HUD: add party HP + an enemy intent telegraph (reuse `SetReactiveBattleState`).
- **Checkpoint:** player can win OR lose a 1v1 vs a slime; enemy actually attacks on its turn.

### Phase 2 — Wire songwriting → the skill rhythm minigame (3–5 days) ⭐ the signature
Goal: the skill you cast is a *song*, and playing that song well is the damage.
- [ ] Create `DA_Song` data (NotePitches[], NoteDurations[], Instrument, MaterialSlots) — start with 1–2 authored songs per instrument.
- [ ] On **skill select**, call `GenerateSpellFromSong` → `FMelodiaGeneratedSpell` (Power, HitCount, SPCost, SecondaryChance).
- [ ] Build a **note timeline** from the song: map `NoteDurations` against `AMelodiaMusicManager` beats to schedule `HitCount` note hit-windows (the rhythm bar).
- [ ] New `UMelodiaRhythmExecutionComponent` (native): opens when a skill is selected, listens to `OnBeatTickEvent`, grades each note via `GradeInputFromBeatPosition`, accumulates per-note grades.
- [ ] Final skill damage = `Spell.Power × Σ CalculateRhythmDamage(base, gradeₙ) × InstrumentPowerScalar`, with `SecondaryChance` rolling a bonus effect. Replace the flat `1.75` skill scalar.
- [ ] HUD: note-highway widget — notes scroll to a hit zone (extend `UMelodiaRhythmHUDWidget`; this is the `RHYTHM_SYSTEM.md` §7 tight-coupling feature). Add note states Tap first; Hold/Slide later.
- **Checkpoint:** selecting a skill opens the note bar; playing the song's pattern well scales damage; SP is spent per `Spell.SPCost`.

### Phase 3 — Skill tutorial & onboarding (1–2 days)
- [ ] First-encounter `bTutorialSeen` gate (save to a `USaveGame` or GameInstance flag).
- [ ] Tutorial overlay sequence on first battle: "Pick a skill → notes will appear → tap Space on the beat." Pause/slow the first note bar (tie to `RHYTHM_SYSTEM.md` "Largo" wider windows).
- [ ] HUD `ShowActionPrompt` already exists — script the step text through it.
- **Checkpoint:** a new player is guided through their first skill cast.

### Phase 4 — The slime encounter & explore space (2–3 days)
- [ ] Author a small playable explore area (reuse a PCG baroque zone from `PCG_SYSTEMS_GUIDE.md`, or a simple blockout) instead of hardcoded coords.
- [ ] `BP_Slime` enemy actor: visual + wandering, overlaps `AMelodiaEncounterTrigger`, sets enemy stats (HP/SPD/intent) on the battle controller.
- [ ] Encounter→battle camera/transition polish (in-place is fine).
- **Checkpoint:** walk up to a visible slime, battle starts, win/lose, return to explore, slime is consumed/respawns.

### Phase 5 — Loop closure, juice, tuning (2–3 days)
- [ ] Tune rhythm windows (RHYTHM_SYSTEM §5 — MISS unreachable at 128 BPM; tighten to ~50/100/160 ms which `FMelodiaRhythmWindows` already defaults to).
- [ ] Victory reward → exploration handoff polish; multiple encounters in a row.
- [ ] SFX per grade (Perfect/Great/Good/Miss), beat-synced HUD pulse, damage flash (hooks exist).
- [ ] Hook one real music track into `AMelodiaMusicManager.DefaultSoundTrack` at the correct BPM.
- **Checkpoint:** full target loop is playable start-to-finish with feel.

---

## 5. Concrete C++ work items (high-value, low-risk first)

1. **`UMelodiaRhythmExecutionComponent`** (Phase 2) — the missing bridge between `GenerateSpellFromSong` and the beat clock. This is the keystone; everything signature depends on it.
2. **Enemy-turn + player-HP in `MelodiaBattleLoopLibrary`/`CombatStateComponent`** (Phase 1) — makes the game losable.
3. **Collapse the dual source of truth** — add a `bUseNativeRhythmModel` switch and route template `DealDamage` to call into the loop library, not parallel math.
4. **`DA_Song` + a small song library** — content the rhythm minigame consumes.
5. **Note-highway widget** — extend `UMelodiaRhythmHUDWidget` with scrolling note state.

---

## 6. Risks / decisions for the user

- **Template vs native combat brain** (§3) — recommend native-authoritative. Confirm before Phase 1.
- **Loose tap vs note highway** — the target loop ("rhythm bar HUD shows up → execute skill rhythm game") implies the **tight-coupled note highway**. That's Phase 2 and the largest single feature. Confirm we build it now rather than shipping the loose tap first.
- **Songwriting depth at slice stage** — start with *authored* `DA_Song`s (deterministic via `MakeCompositionHash`); the full drag-notes-on-a-staff composer UI (`SIGNATURE_MECHANICS.md` #3) is post-slice.
- **Explore space** — reuse PCG baroque zone or a quick blockout? Affects Phase 4 scope.

---

## 7. Definition of "working game" (vertical slice acceptance)

Player spawns in an explore area → sees a slime → walks into it → battle starts → (first time) tutorial explains skills → selects a songcraft skill → note bar appears → plays the song's rhythm pattern → damage scales with timing → enemy takes its turn and can hurt/defeat the player → repeat until the slime is defeated → reward → return to explore. Losable, winnable, and the rhythm is *the song*.

---

*Created this cycle. Companions: `RHYTHM_SYSTEM.md` (timing internals), `SIGNATURE_MECHANICS.md` (depth roadmap), `SIMULATED_UNIVERSE_DESIGN.md` (roguelike framing), `PCG_SYSTEMS_GUIDE.md` (explore environments).*
