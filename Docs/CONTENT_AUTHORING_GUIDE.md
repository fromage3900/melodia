# Melodia — Content Authoring Guide

How to add your own **music, sound effects, quests, enemies, and NPCs** to Melodia. Written for the artist/designer (you), not just engineers. Paths assume the current project layout.

> Golden rule learned the hard way: **keep source files OUT of OneDrive.** UE's OneDrive cloud-filter causes import hangs and asset corruption. Keep raw `.wav/.ogg/.fbx/.psd` in a plain local folder like `G:\Melodia_Source\` and import from there.

---

## 1. Add your own MUSIC

**Supported source formats:** `.wav` (preferred), `.ogg`. (UE imports these to `USoundWave` assets.)

**Steps:**
1. Put the track in a local folder (not OneDrive), e.g. `G:\Melodia_Source\Music\`.
2. In the editor: Content Browser → navigate to `/Game/Melodia/Audio/Music/` → **Import** → pick the file. It becomes a `SoundWave`.
3. (Optional but recommended) Right-click the SoundWave → **Create Sound Cue** (or **MetaSound Source**) for volume/looping/concatenation control.
4. **Hook it to the beat system:** the rhythm engine needs the track's **BPM**. Open `BP_MusicManager` (`/Game/Blueprints/`) and call `PlayMusic(SoundTrack, BPM)` with your Sound + its real BPM. The Quartz clock drives all rhythm timing off that BPM, so **the BPM must match the track** or notes will drift.

**Tips:**
- Find a track's BPM in your DAW (or a BPM-detector site) before importing.
- Loopable battle tracks: set the Sound Cue/MetaSound to loop; mark loop points in the asset.
- For sheet-music / note display in UI, use the **Noto Music** font (`NotoMusic-Regular.ttf`) — it has proper musical-notation glyphs (notes, clefs, rests). Import it: Content Browser → Import → the `.ttf` → it becomes a Font asset usable in UMG `Text` blocks.

---

## 2. Add SOUND EFFECTS

**Steps:**
1. Local folder → `G:\Melodia_Source\SFX\`.
2. Import to `/Game/Melodia/Audio/SFX/` → `SoundWave`.
3. Right-click → **Create Sound Cue** (lets you add randomization/pitch variation — great for hit sounds so they don't feel repetitive).
4. Play it from any Blueprint with **Play Sound 2D** (UI/non-spatial) or **Play Sound at Location** (world).

**Where rhythm SFX hook in:** in `BP_RhythmInputValidator` (after `ValidateInputTiming` grades a hit) add a `Play Sound 2D` per rating — e.g. a bright chime on Critical, a duller note on Good, a thud on Miss. This is the #1 thing that makes rhythm feel "juicy."

---

## 3. Add QUESTS

Melodia already has a quest framework (`BP_QuestBase`, `BP_QuestGiver`) plus rhythm quest types (`BP_RhythmDuelQuest`, `BP_RhythmTempoChaseQuest`, `BP_RhythmFreestyleQuest`, `BP_RhythmEnduranceQuest`) in `/Game/Blueprints/Gameplay/`.

**To add a normal quest:**
1. Content Browser → right-click `BP_QuestBase` → **Create Child Blueprint Class** → name `BP_Quest_<YourName>`.
2. Open it, fill the dialogue arrays (state-aware text):
   - `DialogueWhenNew`, `DialogueWhenInProgress`, `DialogueWhenReadyToDeliver`, `DialogueWhenCompleted`, `DialogueWhenUnavailable`.
3. Set objective/reward fields.
4. Assign the quest to a `BP_QuestGiver` NPC (see §5) via its quest property.

**To add a rhythm quest** (a song-battle challenge): child one of the `BP_Rhythm*Quest` types instead and set its song/BPM/difficulty.

**Dialogue scripting:** Melodia uses **QuillScript** for visual-novel dialogue. Author the conversation in QuillScript, then have `BP_DialogueManager.StartDialogue()` play it on interaction.

---

## 4. Add ENEMIES (slimes, daemons, melody critters)

Melodia's combat sits on the TurnBasedJRPG template, which is **data-driven** — enemies are defined by stats + a mesh/sprite, not hand-coded.

**Steps (data-driven enemy):**
1. Find the enemy definition (Data Table or enemy `BP` under `/Game/TurnBasedJRPGTemplate/`). Enemies = stats (HP, ATK, SPD, element weaknesses) + visual + skill list.
2. Duplicate an existing enemy entry, rename to your type:
   - **Slimes** — low HP, basic attacks, common fodder (good for early run nodes).
   - **Daemons** — higher HP, status-inflicting skills, elite/mini-boss tier.
   - **Melody Critters** — gimmick enemies tied to the rhythm system (e.g. shift the beat/tempo, demand tighter timing).
3. Set **weaknesses** to the spell Elements (`E_SpellElement`: Forte/Tide/Gale/Stone/Radiant/Umbral/Arcane) so Songcraft choices matter.
4. Assign the enemy to encounters (battle actors like `BP_FixedEnemyBattleBase`, or the roguelike encounter pool — see SIMULATED_UNIVERSE_DESIGN.md).

**For the rhythm tie-in:** give Melody Critters a "tempo-shift" skill so fighting them changes the note pattern — this is where enemies feel musical.

---

## 5. Add NPCs

**Steps:**
1. Child `BP_QuestGiver` (or a plain interactable NPC BP) → `BP_NPC_<Name>`.
2. Add a skeletal mesh / paper sprite for the character.
3. Set its dialogue (QuillScript) and, if it gives a quest, assign the `BP_Quest_*` from §3.
4. Place it in a level; on player interaction it fires `OnQuestNPCInteracted` → `BP_DialogueManager.StartDialogue()`.

**Party members** (Melusina, Sir. Melodious, Viola, Dorian) are special NPCs/units defined as playable characters with an **instrument** (= combat style: Music Box, Violin, Drums, Harp, Trumpet) via `DA_Instrument` data assets.

---

## 6. Quick reference — where things live

| Content | Folder | Key Blueprint/Asset |
|---|---|---|
| Music | `/Game/Melodia/Audio/Music/` | `BP_MusicManager.PlayMusic(Sound, BPM)` |
| SFX | `/Game/Melodia/Audio/SFX/` | `Play Sound 2D` in BP_RhythmInputValidator |
| Quests | `/Game/Blueprints/Gameplay/` | `BP_QuestBase`, `BP_Rhythm*Quest` |
| Enemies | `/Game/TurnBasedJRPGTemplate/` | enemy data + `E_SpellElement` weaknesses |
| NPCs | `/Game/Blueprints/` | `BP_QuestGiver`, `BP_DialogueManager` |
| Spells | `/Game/Melodia/Data/` | `S_MelodiaSpell`, `BP_SongcraftManager.GenerateSpell` |
| Fonts | `/Game/Melodia/UI/Fonts/` | Twinkle Star (display), M PLUS Rounded 1c (body), Noto Music (notation) |

---

## 7. The golden workflow (avoid the pitfalls we hit)

1. Source files in `G:\Melodia_Source\` (NOT OneDrive).
2. Import into the matching `/Game/Melodia/...` folder.
3. Commit after each batch (`git add` + commit) so there's always a rollback.
4. For rigs/heavy FBX: uncheck "Import Morph Targets" and "Generate LODs" on first import if it hangs; add them back once the base imports.
5. Test in PIE early and often.

---

*Companion docs: `SIMULATED_UNIVERSE_DESIGN.md` (roguelike), `LIVING_IMPRESSIONIST_PAINTING_ENGINE.md` (materials), `PCG_SYSTEMS_GUIDE.md` (procedural levels).*
