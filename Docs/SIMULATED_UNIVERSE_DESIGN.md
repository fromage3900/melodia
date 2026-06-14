# Melodia — "The Endless Reverie" (Simulated-Universe-style Roguelike)

A run-based, escalating, roguelike mode modeled on Honkai: Star Rail's **Simulated Universe**, re-themed for **magical rhythm combat tied to the Songwriting system**. Research basis: HSR SU = choose a Path → traverse a node map of Domains → collect Blessings → beat a final Boss, with Path Resonance ultimates and meta-progression. ([HSR SU wiki](https://honkai-star-rail.fandom.com/wiki/Simulated_Universe), [Paths](https://honkai-star-rail.fandom.com/wiki/Simulated_Universe/Paths), [Prydwen guide](https://www.prydwen.gg/star-rail/guides/simulated-universe))

---

## Core fantasy
You enter a dream/reverie. Each run you pick a musical **Movement** (≈ HSR Path), travel a branching map of **Stages** (≈ Domains), and collect **Refrains** (≈ Blessings) that warp how your party and your composed songs behave. The deeper you go, the harder it gets, ending in a **Finale** boss. Your **Songbook** (spells you composed in the main game) is your build foundation; Refrains remix it.

---

## 1. Movements (≈ HSR Paths) — pick one per run
Each Movement grants a passive identity + biases which Refrains you're offered + unlocks a **Crescendo** (≈ Path Resonance ultimate). Start with 4–5, expand later.

| Movement | Fantasy | Passive identity | Crescendo (ultimate) |
|---|---|---|---|
| **Crescendo** | Building power | Damage ramps as your combo/chain grows | Unleash stored chain as one massive hit |
| **Harmony** | Support/heal | Perfect hits also heal the party | Full-party heal + shield on the beat |
| **Staccato** | Burst/speed | Shorter note windows, but big SP refund on Critical | Extra turn / act-again |
| **Dissonance** | Risk/DoT | Misses apply curses to enemies instead of failing | Detonate all curses across enemies |
| **Legato** | Sustain/flow | Hold-notes deal bonus damage and never break combo | Channel a sustained beam that scales with hold length |

Resonance escalation (HSR-style): collect 6 / 10 / 14 Refrains of your Movement to upgrade the Crescendo at thresholds.

---

## 2. Refrains (≈ HSR Blessings) — the buffs you collect
Short musical buffs, grouped by Movement, rarities 1–3 (★/★★/★★★). Earned from winning Stages, events, and Curios. They modify the **rhythm + Songcraft** layers specifically — that's the Melodia twist.

Examples:
- *Crescendo ★*: +8% damage per chain step (max 10).
- *Harmony ★★*: every 5th Perfect, cleanse one debuff.
- *Staccato ★*: Critical window +20ms (more forgiving).
- *Dissonance ★★★*: enemies at 3+ curses take double Songcraft spell damage.
- *Legato ★★*: Hold-notes count as 2 chain hits.
- *Songcraft-tie ★★★*: your first composed spell each battle is cast free (0 SP).

Refrains can be **enhanced** (HSR-style) at Rest stages by spending the run currency.

---

## 3. Stage map (≈ HSR Domains) — node types
A run is a branching map of stages you choose a path through (escalating difficulty per row). Re-themed Domain types:

| Stage (Melodia) | HSR equivalent | What happens |
|---|---|---|
| **Performance** | Combat | Standard rhythm battle vs slimes/daemons/melody critters |
| **Interlude** | Occurrence | Random event: choose-your-own (gain Refrain, gamble, lore) |
| **Rehearsal** | Transaction | Shop: spend currency on Refrains/Curios/heals |
| **Duet** | Encounter | Meet an ally/Curio; free Refrain or party boon |
| **Encore** | Elite | Hard fight (Melody Critter elite w/ tempo gimmicks) → better rewards |
| **Rest** | Respite | Heal, enhance Refrains, recompose a song |
| **Finale** | Boss | Run-ending boss; multi-phase song-battle |

Player picks the route → risk/reward (more Encores = harder but richer).

---

## 4. Songwriting tie-in (the heart of it)
This is what makes Melodia's roguelike unique vs generic SU:
- Your **Songbook** (spells composed via `BP_SongcraftManager.GenerateSpell` — note sequence + instrument → `S_MelodiaSpell`) is your loadout going into a run.
- **Refrains remix Songcraft**: e.g. "all Tide-element spells gain a heal," "spells cost 1 less SP," "first spell free."
- At **Rest** stages you can **recompose** — re-enter the note sequencer to retune a spell for the enemies ahead (exploit `E_SpellElement` weaknesses).
- **Melody Critter** enemies shift tempo/beat → your timing windows change → your composed phrases must adapt. Combat *is* performance.

---

## 5. Run loop (the build sequence to implement)
1. **Run state** (GameInstance subsystem or `BP_ReverieRunManager`): current Movement, collected Refrains[], currency, HP carryover, map position, seed.
2. **Map generator**: build a branching node graph (rows of stage choices). Reuse PCG/data for variety; seed-controlled.
3. **Stage handlers**: Performance → rhythm battle; Interlude/Rehearsal/Duet/Rest → UI events; Encore/Finale → scaled battles.
4. **Refrain system**: data-driven Refrain definitions (Data Table or Data Assets), offer 3-pick-1 after stages, apply effects as gameplay modifiers on party/Songcraft.
5. **Escalation**: scale enemy stats by row/depth; elite/boss tables.
6. **Meta-progression**: persistent unlocks between runs (new Movements, starting Refrains, Songbook slots) saved via the Save system (`SG_MelodiaSave`).

---

## 6. Minimal first playable (what the loop builds first)
To get a *magical-feeling* slice fast, in order:
1. `BP_ReverieRunManager` (run state: Movement, Refrains[], currency, seed).
2. Linear 5-stage run (skip branching for v1): 3 Performance → 1 Rehearsal → 1 Finale.
3. Refrain data (6–8 Refrains) + 3-pick-1 reward UI after each Performance.
4. Apply 2–3 Refrain effects to the rhythm/Songcraft layer (damage-per-chain, free-first-spell, wider window).
5. Carry Songbook in; allow recompose at the one Rest before Finale.
6. Win Finale → meta unlock → loop.

Then expand to branching map, all Movements, Curios, enhance system.

---

## 7. Naming (keep the musical identity)
Movements, Refrains, Stages, Crescendo, Reverie — all musical/dream language, in the Melusina palette (ink-navy/aqua/gild) with Twinkle Star + Noto Music notation glyphs on the reward cards.

---

*Sources: HSR Simulated Universe — [wiki](https://honkai-star-rail.fandom.com/wiki/Simulated_Universe), [Paths wiki](https://honkai-star-rail.fandom.com/wiki/Simulated_Universe/Paths), [Prydwen](https://www.prydwen.gg/star-rail/guides/simulated-universe), [Game8](https://game8.co/games/Honkai-Star-Rail/archives/409149). Companion: `CONTENT_AUTHORING_GUIDE.md`, loop memory `melodia-gameplay-loop-mandate`.*
