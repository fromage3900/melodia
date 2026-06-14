# Melodia — Signature Mechanics (where music IS the mechanic)

Brainstormed 2026-06-14. Goal: make Melodia feel like nothing else — every system speaks "music." Ordered by priority. ⭐ = top picks (most distinctive + synergistic + feasible).

## Top 4 (build these first — they define the game)
1. **⭐ Harmony / duet beats** — when two party members act in the same measure, nailing *both* their beats in sync triggers a Harmony: a combined attack with bonus effects. Makes party composition + rhythm matter together. (Theatrythm "feature" moment.)
2. **⭐ Leitmotifs** — a composed song becomes a reusable motif that grows stronger the more it's mastered (familiarity bonus). A character's signature spell literally becomes their theme music. Ties directly to Songcraft `GenerateSpell` + a per-spell mastery counter.
3. **⭐ Sheet-music spellcrafting UI** — compose spells by dragging notes onto a real musical staff (uses the **Noto Music** font for glyphs). Diegetic, signature UI; output feeds `GenerateSpell` (note sequence → S_MelodiaSpell).
4. **⭐ Run soundtrack builds with blessings** — each Reverie Refrain adds an instrument layer to the music; by the final boss the track is fully orchestrated. The run becomes *audible*. Pairs with the existing audio-reactive materials (MPC_MusicClock).

## Combat depth
- **Tempo as a battle state** — Melody Critters speed up / slow the song mid-fight, changing timing windows (slow = wider windows but enemies hit harder; fast = tighter but more SP). A conductor tug-of-war.
- **Key signature / modulation** — battle has a musical key; in-key spells get bonuses, off-key penalties (musical "elemental weakness"). Modulating the key is an action (risk/reward).
- **Rest notes (silence)** — deliberately NOT hitting a beat defends + charges a Crescendo meter. Restraint as power.
- **Call-and-response** — boss plays a phrase; echo it back (Simon-says) to parry/counter. Great telegraph design.

## Songwriting depth
- **Counter-melody layering** — stack two party members' songs into one bigger combined spell (chords).
- **Dissonance → resolution** — build tension, then resolve for a payoff hit (tension/release damage curve).
- **Improvisation/Freestyle** — hit notes in a scale to generate random-but-in-key spells (ties to BP_RhythmFreestyleQuest).

## World & exploration (Melusina / water / dream theme)
- **Sing to the world** — melodies resonate crystals, open paths, calm critters, shatter barriers by pitch. Music as the "key item."
- **Recruit Melody Critters by song** — charm enemies into collectible allies/summons.
- **Tidal puzzles** — Melusina raises/lowers water with song; environments shift with the musical "tide."
- **Echoes** — areas hold musical echoes of the past; replay for lore/puzzles (fits the Reverie/dream theme).

## Roguelike (Endless Reverie)
- **Curio instruments** — found instruments with quirky rules (cracked music box: randomizes notes but doubles power).
- **Dissonant routes** — harder "off-key" map paths = more danger, better Refrains.

## Juice / accessibility (leverages built tech)
- **World pulses to the beat** — audio-reactive materials make the environment bloom/throb on rhythm (already built foundation).
- **Difficulty as tempo** — "Largo" widens windows (story players); "Presto" for experts. On-theme accessibility.

---

**Sequencing:** the Top 4 layer onto the template-battle pivot. Harmony + Leitmotifs extend the rhythm→`ApplyRhythmModifier`→`GenerateSpell` chain; the sheet-music UI is a UMG widget feeding `GenerateSpell`; the run-soundtrack hooks the Reverie Refrain system + MPC_MusicClock. Build after the core rhythm↔songcraft tie works in PIE.

*Companion: SIMULATED_UNIVERSE_DESIGN.md, melodia-gameplay-loop-mandate memory.*
