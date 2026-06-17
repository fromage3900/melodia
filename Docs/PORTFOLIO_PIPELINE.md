# Melodia — Portfolio Pipeline Map

One game, every facet of a 3D production pipeline — structured for **environment art** as the primary hire signal, with supporting systems that prove you can ship inside a team.

Use this doc when recording reel beats, building breakdown slides, or scoping your next authoring pass.

---

## Executive summary (what Melodia demonstrates)

| Pipeline facet | Melodia proof | Portfolio beat |
|----------------|---------------|----------------|
| **Environment art** | PCG baroque/Escher graphs, 30 location presets, L_MelodiaRhythm | Hero reel: 60–90s flythrough + PCG breakdown |
| **Level design** | Mechanic tiers Lv1–30, encounter triggers, portals, rest points | Progression map + gating diagram |
| **Technical art** | Custom PCG elements, walkable index, gravity/tessellation attrs | Node graph + attribute overlay video |
| **Lighting / mood** | Lumen, impressionist material spec (`LIVING_IMPRESSIONIST_PAINTING_ENGINE.md`) | Before/after lighting pass |
| **Character / animation** | Melusina ARP rig, mocap path, jump/glide locomotion | Locomotion + idle/walk mocap clip |
| **UI / UX** | HSR-style native HUD, Noto/Cherry Bomb fonts | HUD motion reel |
| **Gameplay programming** | C++ exploration, battle loop, quests, save | Architecture one-pager |
| **Audio / rhythm** | Quartz BPM clock, skill highway | Beat-sync combat clip |
| **Tools / procedural** | Reverie run manager, PCG encounter spawner | Seed sweep timelapse |

---

## Environment art (primary focus)

### What exists now

- **PCG library** — 20+ graphs documented in `PCG_SYSTEMS_GUIDE.md` (baroque, Escher, garden, forest).
- **Custom C++ PCG nodes** — Escher Staircase, Gravity Zone, Tessellation, Recursive Arch (`EditInlineNew` for editor menu).
- **30 unlockable location presets** — `MECHANIC_PROGRESSION.md`; each maps to a PCG graph soft path for authoring.
- **Decoration / encounter spawners** — PCG-driven placement via `AMelodiaReverieRunManager`.
- **Materials direction** — `MATERIALS_LIBRARY_SPEC.md`, MooaToon toon pipeline.

### Portfolio tasks (environment artist)

1. **Hero zone** — Pick one preset (e.g. Lv15 Moon Conservatory / Terrace Garden). Blockout → PCG graph → hand-polish hero shots.
2. **Flower scatter pass** — Replace placeholder `AMelodiaPickableFlower` meshes with authored blossom assets; keep F-pick interaction.
3. **Lighting study** — One golden-hour + one moonlit setup on `L_MelodiaRhythm`; capture 4K stills.
4. **Breakdown sheet** — Poly count, material instances, PCG node list, Nanite vs non-Nanite choice.
5. **Tier gallery** — Six images (Tier I–VI) showing visual complexity ramp tied to mechanic levels.

### Suggested reel structure (env lead)

1. Establishing wide (glide camera — **Space jump + hold glide**).
2. PCG generation timelapse (seed change).
3. Detail shots: materials, decals, foliage.
4. Interaction: pick flower (**F**), rest bed, portal.
5. Combat arena transition (song gate) — shows env supports gameplay.

---

## Animation & locomotion (professor / TD lens)

### Implemented

| Mechanic | Input | System |
|----------|-------|--------|
| **Jump** | Space / Shift | `AMelodiaCharacterBase` + `UCharacterMovementComponent` |
| **Glide** | Hold Space in air | `UMelodiaGlideComponent` — reduced gravity, forward accel, stamina |
| **Walk / camera** | WASD + mouse | Spring arm 520cm, lag enabled |

### Animation portfolio hooks

- Import mocap to `SK_Melusina_Prototype` (`RECOVERY.md` §5).
- **ABP_Melusina** states: Idle, Locomotion, JumpStart, FallLoop, GlideLoop (blend by `GlideComponent->bIsGliding`).
- Show **root motion off / in-place** comparison for glide (teaching moment).
- Foot IK on garden PCG terrain (future polish).

---

## Interaction design

| Action | Key | Actor / component |
|--------|-----|-------------------|
| Interact (portal, bed) | E | `IMelodiaInteractable` |
| Pick flower | F | `AMelodiaPickableFlower` |
| Inventory | I | `UMelodiaInventoryComponent` |
| Jump / glide | Space (hold in air) | `UMelodiaGlideComponent` |

Quest: **Blossom Gatherer** — pick 3 `ReverieBlossom` items.

---

## Other pipeline facets (supporting reels)

### Technical art / PCG
- Graph: `PCG_FloatingStairways` + custom Escher node.
- Show `GravityDir` attribute driving mesh orientation.

### Lighting
- Default: Lumen + virtual shadow maps (`DefaultEngine.ini`).
- Document one fill/rim/moon triple for Melusina bed area.

### UI
- Native Slate HUD — `UI_FONTS.md`, Kenney sparkles.
- Capture: exploration → battle → skill highway.

### Programming
- Modular C++: GameMode bootstrap, quest manager, mechanic progression subsystem.
- One diagram: Explore → Encounter → Phoenix battle → Rhythm skill → Victory.

### Audio
- Hook `BP_MelodiaQuartzMusicManager` BPM to battle (`CONTENT_AUTHORING_GUIDE.md`).

---

## Demo checklist (record this session)

- [ ] Launch via `Open Melodia Editor.bat`, PIE `L_MelodiaRhythm`.
- [ ] Glide from spawn overlook (Space jump, hold Space).
- [ ] Pick 3 flowers with **F** — quest completes.
- [ ] E at rest point — save + day advance.
- [ ] Song gate → battle → skill rhythm → win.
- [ ] Open PCG graph — show custom elements menu.
- [ ] Show mechanic level banner + tier quest in journal.

---

## Gap list (honest, for portfolio v2)

| Gap | Impact on portfolio | Suggested fix |
|-----|---------------------|---------------|
| Placeholder flower mesh | Env art not shown | Author `/Game/Melodia/Environment/Foliage/SM_ReverieBlossom` |
| Glide without anim blend | Animation reel weak | ABP glide state |
| No baked lighting variant | Lighting range narrow | One sublevel with baked mood |
| Phoenix UI not custom-skinned | UI reel partial | Reskin battle menu to match HUD |
| Packaging not demo-ready | No standalone exe | MooaToon package when stable |

---

## File index (quick reference)

| Topic | Doc / code |
|-------|------------|
| Editor setup | `EDITOR_READY.md`, `RECOVERY.md` |
| PCG | `PCG_SYSTEMS_GUIDE.md` |
| Progression / zones | `MECHANIC_PROGRESSION.md` |
| Rhythm / combat | `RHYTHM_SYSTEM.md`, `GAMEPLAY_LOOP_PLAN.md` |
| Flowers | `MelodiaPickableFlower.cpp` |
| Glide | `MelodiaGlideComponent.cpp` |
| Character | `MelodiaCharacterBase.cpp` |

---

*Melodia is designed as a living portfolio: each system is a chapter you can screenshot, record, and annotate for hiring managers in environment art, with adjacent disciplines visible in the same build.*
