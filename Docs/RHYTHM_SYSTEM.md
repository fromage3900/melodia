# Melodia — Rhythm Combat System (technical reference)

How the "tap to the beat → scale damage" system works, verified link-by-link in the editor (2026-06-14). This is the single source of truth for the rhythm slice; update it when the system changes.

> Status: **functionally complete + verified correct end-to-end** on the *loose-coupling* model. Awaiting user PIE playtest (the STOP condition) + tuning decisions. Theatrythm tight-coupling is the next major feature (see §7).

---

## 1. The one-paragraph summary

During a battle in the template Gameplay map, the player taps **Space / Left-Mouse** in time with the beat. The tap is graded against the music clock (Perfect/Great/Good/Miss → a damage multiplier), the grade is pushed onto the live `BP_BattleController`, and the **next attack's damage is multiplied by it**, then the multiplier auto-resets to 1.0. A gold `* * * BEAT * * *` print fires once per beat as the timing cue, and each tap prints its rating + multiplier + combo (e.g. `PERFECT!   x1.5   C:3`).

---

## 2. The components (where everything lives)

| Asset | Role |
|---|---|
| `/Game/Blueprints/BP_MusicManager` | The beat clock. `PlayMusic(Sound, BPM)` sets `BeatLength=60/BPM` + `PlaybackStartTime`. `Event Tick` advances the beat and fires `OnBeatTick` (gold metronome print) per beat, `OnDownbeat` every 4th. `GetCurrentBeatPosition` = `(GetTimeSeconds − PlaybackStartTime) / BeatLength`. `GetBeatLength` returns the beat length in seconds. |
| `/Game/Blueprints/BP_RhythmInputValidator` | The grader. `ValidateInputTiming(beatPos, beatLen) → DamageMultiplierOut`. Computes timing error = `min(frac, 1−frac) × beatLen`, compares to window vars, outputs **Perfect 2.0 / Great 1.5 / Good 1.2 / Miss 0.5**. Windows: `PerfectWindow 0.1s`, `GreatWindow 0.2s`, `GoodWindow 0.3s` (instance-editable). |
| `/Game/Blueprints/Gameplay/BP_RhythmTestBattleManager` | The glue actor, **placed in the Gameplay map** ("RhythmTest_VerifyMe"). BeginPlay: adds `IMC_Rhythm`, enables input, spawns the MusicManager (`PlayMusic` 128 BPM) + Validator. On `IA_RhythmHit`: sample beat → grade → find the live `BP_BattleController` via `GetActorOfClass` → `RegisterRhythmHit` → `SetRhythmMultiplier` → print feedback. Helper fn `GetRatingWord(grade)→string`. |
| `/Game/TurnBasedJRPGTemplate/Blueprints/Battle/BP_BattleController` | The combat core, **placed in the Gameplay map**. Holds `CurrentRhythmMultiplier` (float, 1.0) + `RhythmCombo` (int). Fns: `SetRhythmMultiplier`, `RegisterRhythmHit`, `GetRhythmRating`. `CalculateDamage` multiplies by `CurrentRhythmMultiplier`; `DealDamage` resets it to 1.0 after. |
| `/Game/Melodia/Input/IA_RhythmHit`, `IMC_Rhythm` | Enhanced Input: Space + Left-Mouse, Pressed. |

---

## 3. The runtime chain (every link verified)

1. **Clock start** — `PlayMusic(_, 128)` → `BeatLength = 60/128 = 0.469s`, `PlaybackStartTime = now`. (No SoundTrack asset wired yet → no music; the gold beat print is the only cue. Adding a track is a content step — drop a SoundWave into the `PlayMusic` SoundTrack pin.)
2. **Clock runs** — `Event Tick` samples `GetCurrentBeatPosition` continuously; on each new integer beat fires `OnBeatTick` (gold `* * * BEAT * * *`, 0.22s).
3. **Tap** — `IA_RhythmHit` → `GetCurrentBeatPosition` + `GetBeatLength` → `ValidateInputTiming` → `DamageMultiplierOut` (2.0 / 1.5 / 1.2 / 0.5).
4. **Push to controller** — `GetActorOfClass(BP_BattleController_C)` → `RegisterRhythmHit(grade ≥ 0.7)` (builds `RhythmCombo`, or resets on miss) → `SetRhythmMultiplier(grade)` = `clamp(grade, 0.4, 1.5) + RhythmCombo × 0.05`.
5. **Feedback** — print `GetRatingWord(grade) + "   x" + grade + "   C:" + combo`.
6. **Damage** — the next `DealDamage` → `CalculateDamage` does `pure × damageMultiplier × CurrentRhythmMultiplier` → spawns `BP_DamageText` → `BP_UnitBase.TakeDamage`. Then `DealDamage` sets `CurrentRhythmMultiplier = 1.0` (one tap boosts exactly one attack; no bleed into enemy/other damage).

**Battle start** (verified): walk a character into a `BP_*EnemyExplorePawn` (or `BP_PermanentBattle`/`BP_InteractionBattle` trigger) → `BP_BattleController.StartBattle` runs **in-place** (IncreaseTurnCount → PlayBattleTheme → InitUnits → camera → Create BattleUI → Viewport → SetReadyUnits) — **no level transition**, so the placed rhythm manager and controller share the world.

---

## 4. Tuning knobs

| Knob | Where | Default | Notes |
|---|---|---|---|
| BPM | `BP_RhythmTestBattleManager` BeginPlay → `PlayMusic` BPM pin | 128 | Beat length = 60/BPM. |
| Timing windows | `BP_RhythmInputValidator` `PerfectWindow`/`GreatWindow`/`GoodWindow` | 0.1 / 0.2 / 0.3 s | ⚠️ See §5. |
| Tier multipliers | `ValidateInputTiming` Set DamageMultiplier nodes | 2.0 / 1.5 / 1.2 / 0.5 | Clamped to 0.4–1.5 in `SetRhythmMultiplier`. |
| Combo bonus | `BP_BattleController.SetRhythmMultiplier` | +0.05 / step | |
| Rating-word thresholds | `BP_RhythmTestBattleManager.GetRatingWord` | 1.75 / 1.35 / 0.85 | Aligned to the tier multipliers. |
| Hit-vs-miss threshold (combo) | tap path `GreaterEqual` B | 0.7 | grade ≥ 0.7 = hit. |

---

## 5. Known tuning issue — MISS is unreachable at 128 BPM

Beat length 0.469s → max possible timing error = half a beat ≈ **0.234s**, which is *less than* `GoodWindow` (0.3s). So every tap lands Good-or-better; a real MISS can't happen. To make the full Perfect→Miss range reachable at 128 BPM, tighten to approximately **Perfect 0.05 / Great 0.10 / Good 0.16**. This is a feel decision — left to playtest. (Edit the three window vars on `BP_RhythmInputValidator`.)

---

## 6. How to test (PIE)

1. Open / PIE the Gameplay map (`/Game/TurnBasedJRPGTemplate/Maps/Gameplay`).
2. Watch for the gold `* * * BEAT * * *` print (128 BPM) — your timing reference.
3. Walk into an enemy explore pawn to start a battle.
4. On the beat, tap **Space** or **Left-Click** → see `PERFECT!/GREAT/GOOD/MISS  x<mult>  C:<combo>`.
5. Execute the attack → the damage number should scale up after a good tap, and reset between attacks.

---

## 7. Next: Theatrythm tight-coupling (not yet built)

The loose model (tap anytime → next attack consumes) is functional. The full Theatrythm feel wants:
- A **note highway HUD** (notes scroll toward a hit-zone, driven by the MusicManager beat).
- The timing window **opens automatically when a skill is selected** (inside the `UseSkill` flow — `K2Node_CustomEvent_34` in the 673-node `BP_BattleController` event graph), grades, applies, then `DealDamage`, then resets.
- Note types beyond Tap (Hold, Slide); rating popups + combo cut-ins in the Melusina UI identity (Twinkle Star, ink-navy/aqua/gild).

This is a multi-part UI + async-input feature; build after the loose version is validated in PIE and the HUD visual direction is approved (avoids rework). See `SIGNATURE_MECHANICS.md` + `melodia-gameplay-loop-mandate` memory.

---

*Companion: `CONTENT_AUTHORING_GUIDE.md` (add music/SFX), `SIGNATURE_MECHANICS.md` (combat depth), `SIMULATED_UNIVERSE_DESIGN.md` (roguelike).*
