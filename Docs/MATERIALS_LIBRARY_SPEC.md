# Melodia Material Library Specification

**Master Living Painting — Cohesive Material Ecosystem for Rhythm Combat + Procedural Art**

*Recreated 2026-06-13. Single source of truth for all material organization, compilation standards, and rendering verification. This spec defines how 200+ materials work together to deliver Melodia's visual identity: MooaToon stylized shading + procedural SDF mathematics + audio-reactive dynamics.*

---

## 1. Library Architecture

**Goal:** One cohesive library where every material serves the game's core systems (Rhythm, Songcraft, PCG, UI) and aesthetic (Melusina surreal, Japanese stylized, Escher impossible geometry).

### 1.1 Folder Hierarchy (Target: `/Game/Melodia/Materials/`)

```
/Game/Melodia/Materials/
├── Toon/                 → Base toon shading (MooaToon foundation)
│   ├── Character/        → Clothing, skin, fabric
│   ├── Environment/      → Stone, wood, metal, leather, glass, crystal
│   └── Water/            → Transparent, mystical, toon water
├── SDF/                  → Procedural signed-distance fields (Inigo Quilez)
│   ├── Baroque/          → Gilded, rose windows, ornament, stucco
│   ├── Escher/           → Klein bottle, Möbius strip, Penrose, impossible geometry
│   ├── Ornament/         → Recursive, decorative detail layers
│   └── Architecture/     → Gothic, cathedrals, vaults, grid patterns
├── OilPaint/             → Impressionist, painterly (stylized, low-poly aesthetic)
├── FX/                   → Visual effects, particles, decals, glitter, volumetric ink
│   ├── Particles/        → Niagara particle materials
│   ├── Asteroids/        → GPU effects (field, micro, high-detail)
│   ├── Glitter/          → Iridescent sparkling, world-aligned
│   ├── Decals/           → Damage, grime
│   └── VolumetricInk/    → Accumulation, atmospheric
├── Advanced/             → Complex/experimental MooaToon materials
│   ├── Aura/             → Energy shells, bio-luminescence
│   ├── Vortex/           → Swirl, spoke, radial, pipeline, master
│   ├── Metamorph/        → Crystal fracturing, membrane (organic tissue), fabric flow
│   ├── Cosmic/           → Quantum foam, nebula, starlight dust, celestial
│   └── Esoteric/         → Void anti-light, shimmer, metal living alloy, erosion decay
├── Procedural/           → Procedural generation + reactive (landscape, audio, etc.)
│   ├── Landscape/        → Mountain, island, cloud, height-blend, layer-blend
│   ├── AudioReactive/    → Music clock reactivity, sound wave pulse
│   ├── Cosmo/            → Cosmic master
│   ├── Glitter/          → Master glitter + volumetric ink hybrid
│   └── Impasto/          → Textured brush stroke effect
├── Layers/               → Material layer stack system
│   ├── ML_*/             → Material layer assets (ToonBaseInput, Vein/Cracks, SDF, etc.)
│   ├── MLB_*/            → Material layer blends (ToonBaseBlend, etc.)
│   ├── MF_*/             → Material functions (reusable components)
│   └── MPC_*/            → Material parameter collections (MPC_MusicClock for beat reactivity)
├── Instances/            → Material instances (MI_*)
│   ├── Toon/             → Character, environment instances
│   ├── SDF/              → Procedural instances with custom parameters
│   └── Specialized/      → Custom instances for specific meshes/levels
├── Textures/             → Source textures (T_*)
│   ├── Base/             → Color, normal, roughness, metallic
│   ├── Vein/             → Crack, marble, organic texture patterns
│   ├── Thematic/         → Melusina-specific alphas, overlays
│   └── Utility/          → Procedural noise, gradient, lookup tables
├── TestBench/            → Development + verification materials
│   └── M_SDF_TestBench   → Render-verify all SDF techniques
└── Reference/            → TurnBasedJRPGTemplate (do NOT ship; study only)
```

---

## 2. Material Categories + Aesthetic Intent

### 2.1 **Toon** (Base Stylized Rendering)
- **Purpose:** Character clothing, environment surfaces (stone, wood, metal, fabric, glass, crystal)
- **Engine:** MooaToon custom toon shading (UE 5.7.2 precompiled engine)
- **Key materials:** M_Toon_Base, M_Character_Clothing, M_Stone_Smooth_Toon, M_Metal_Polished_Toon, M_Glass_Refract_Toon, M_Crystal_Clear_Toon, M_Leather_Toon, M_Fabric_Toon, M_Wood_Toon, M_Water_Toon
- **Render target:** Flat colors + toon outlines + anime-style lighting shadows
- **Compilation:** All must compile clean, EmissiveColor wired (Unlit mode), NO Parameters.TexCoords in Nanite permutation

### 2.2 **SDF** (Procedural Signed-Distance Fields)
- **Purpose:** Architectural detail, impossible geometry, recursive ornament (Escher-inspired)
- **Source:** Inigo Quilez techniques (iquilezles.org), GLSL→HLSL port
- **Key materials:** M_SDF_TrueParallax (facade), M_SDF_Baroque, M_SDF_Klein_Bottle, M_SDF_Mobius_Strip, M_SDF_Penrose_Staircase, M_SDF_GothicArchitecture, M_SDF_Ornament*, M_SDF_RoseWindow, M_SDF_GildedFiligree, M_SDF_GildedStucco, M_MusicalSDF_PulsingGeometry
- **Render target:** Raymarched detail (lavender arches, gilded surfaces, impossible shapes)
- **Compilation:** atan2/lerp HLSL conversion, NO TextureCoordinate CoordinateIndex stray values, Unlit+EmissiveColor wiring, UV-pin-only (no Parameters.TexCoords in Nanite)
- **Audio sync:** M_MusicalSDF_PulsingGeometry reacts to beat via MPC_MusicClock.BeatPulse

### 2.3 **OilPaint** (Impressionist Painterly)
- **Purpose:** Stylized low-poly aesthetic, surreal beauty (Infinity Nikki-inspired)
- **Key materials:** M_OilPainting_MasterBase, M_OilPainting_Gold_Baroque, M_OilPainting_Celestial_Cosmic, M_OilPainting_Bio_Organic, M_OilPainting_Portal_Impressionist, M_OilPaint_Advanced_WorldAligned, M_OilPainting_Void_Dark
- **Render target:** Brush-stroke effect, impasto texture, world-aligned mapping
- **Instancing:** Use MI_* instances with color + intensity parameters per asset

### 2.4 **FX** (Visual Effects + Particles)
- **Purpose:** VFX, particles, atmosphere, magical effects
- **Categories:**
  - **Particles/Niagara:** M_Niagara_Particle_Additive, M_Niagara_Particle_Translucent
  - **Asteroids (GPU):** M_Asteroid_Field_Niagara, M_Asteroid_Metallic, M_Asteroid_Chondrite, M_Asteroid_Carbonaceous, Ring_M, Asteroid_Micro_M, Asteroid_HighDetail_M
  - **Glitter:** M_Glitter_WorldAligned, M_Glitter_UltimateSparkling, M_MooaToon_Glitter_Iridescent
  - **Decals:** M_Decal_Damage, M_Decal_Grime
  - **Volumetric Ink:** M_VolumetricInk_Accumulation, M_Glitter_VolumetricInk_Master
- **Render target:** Translucent, additive, layered atmospheric effects
- **Compilation:** All must support subUV, particle parameters, world-space UV

### 2.5 **Advanced** (Complex MooaToon Expressions)
- **Purpose:** Experimental + high-fidelity stylized effects
- **Categories:**
  - **Aura/Energy:** M_AdvAura_EnergyShell, M_AdvBio_LuminescentVeins
  - **Vortex/Swirl:** M_AdvVortex_DepthInfinity, M_MooaToon_Vortex_Master (+ Pipeline, Radial, Spoke variants)
  - **Metamorph:** M_AdvCrystal_Fracturing, M_AdvMembrane_OrganicTissue, M_AdvFabric_EtherealFlow
  - **Cosmic:** M_AdvQuantum_RealtiyFoam, M_MooaToon_CosmicNebula, M_AdvCelestial_StarMap
  - **Esoteric:** M_AdvVoid_AntiLight, M_AdvShimmer_Hypnotic, M_AdvMetal_LivingAlloy, M_AdvErosion_DecayingRealm
- **Render target:** High instruction count (200–400 PS instr), complex connections, layered effects
- **Instancing:** Always use MI_* for production (lock parameters)

### 2.6 **Procedural** (Generation + Reactivity)
- **Purpose:** Landscape, audio-reactive systems, cosmic/atmospheric backgrounds
- **Key materials:**
  - **Landscape:** M_Landscape_LayerBlend, M_Landscape_HeightBlend, M_Mountain_FarDistant, M_Island_Distant, M_Cloud_Atmospheric
  - **Audio:** M_AudioReactive_BaseMaster, M_MooaToon_SoundWave_Pulse, MPC_MusicClock (param collection)
  - **Cosmo:** M_Cosmo_Master
  - **Glitter Master:** M_Glitter_Enhanced_Master
- **Render target:** Dynamic parameters driven by game systems (beat clock, distance fade, etc.)
- **Compilation:** Must support parameter collections + dynamic material instances

### 2.7 **Layers** (Material Composition System)
- **Purpose:** Reusable layer stacks (ML_*/MLB_*/MF_*), beat-reactive parameter collections
- **Key assets:**
  - **Layers:** ML_ToonBaseInput, ML_Stars (duplicate→modify workflow), ML_SDF, ML_Vein, ML_Cracks
  - **Blends:** MLB_ToonBaseBlend (core)
  - **Functions:** MF_* (Mooa functions, custom helpers)
  - **Param Collections:** MPC_MusicClock (BeatPulse driven by BP_MusicManager)
- **Standard:** Never delete /Game copies of MooaToon layers; always duplicate + modify in /Game/Melodia/ (see FOUNDATION §7)

### 2.8 **Instances** (MI_*)
- **Purpose:** Per-asset customization without modifying masters
- **Rules:** 
  - Every environment/character material must have an instance if it needs tweaking
  - Lock parent reference (prevent accidental reparent)
  - Document parameter overrides (color, roughness, scale, emission)
- **Organization:** Mirror master folder hierarchy under `/Game/Melodia/Materials/Instances/`

---

## 3. Compilation Standards

### 3.1 Mandatory Checks
Every material must pass:
1. **Compile clean** — 0 errors, 0 warnings (except shader warning for advanced raymarching)
2. **Render-verify** — visible in viewport (not silent black/white fail)
3. **Instruction count** — reasonable (Toon: 100–200, SDF: 150–400, Advanced: 200–500)
4. **Nanite safety** — if used with Nanite meshes: NO Parameters.TexCoords, NO CameraVector, NO TangentToWorld (use TextureCoordinate input pin only)
5. **Unlit mode** — if Unlit: BaseColor wired → EmissiveColor (BaseColor alone renders black)

### 3.2 Known Pitfalls (Fixed in Migration)
- ✅ **M_SDF_TrueParallax:** TextureCoordinate CoordinateIndex overflow (50→0 fixed)
- ✅ **M_SDF_Klein_Bottle, Mobius_Strip:** GLSL atan2/lerp→HLSL conversion
- ✅ **M_AdvCrystal_Fracturing, M_Sand_Toon, M_Water_Transparent_Toon, M_EnvironmentSync_AnimatedVines:** NULL samplers fixed
- ⚠️ **Nanite + SDF:** TextureCoordinate works; Parameters.TexCoords errors (use UV pin only)

### 3.3 Render Verification Workflow
1. Create test material instance: `MI_TestMaterial`
2. Assign to `M_SDF_TestBench` blueprint actor in a test level
3. View in viewport (should display expected texture/color/effect)
4. If black/white: check Unlit wiring + EmissiveColor connection
5. Commit verified material to Git

---

## 4. Audio-Reactivity (Rhythm Game Specifics)

### 4.1 Beat-Synchronized Materials
**System:** `BP_MusicManager` (Quartz clock) → `MPC_MusicClock` (param collection) → material expressions

**Key Parameter:** `BeatPulse` (0–1, oscillates at beat frequency)

**Materials Using It:**
- M_MusicalSDF_PulsingGeometry (procedural geometry pulsing)
- M_MooaToon_SoundWave_Pulse (vinyl groove effect)
- M_SDF_TestBench (test reactivity)

**How to Wire:**
1. Material → Material Parameter Collection reference → pick `MPC_MusicClock`
2. Add expression: `Material Parameter Collection → Parameter Name "BeatPulse"`
3. Wire to desired output (emissive intensity, normal scale, vertex displacement)
4. Test: Run level → listen to beat → material should pulse

### 4.2 Audio Asset Creation
Materials built for rhythm:
- **M_MooaToon_VinylGroove** — groove lines react to beat
- **M_MooaToon_SoundWave_Pulse** — wave amplitude = beat strength
- **M_MooaToon_InkSplash** — splash expands on beat trigger
- **M_MooaToon_GhostFlame** — flame color/intensity = beat
- **M_MooaToon_CrystalChime** — crystal rings sparkle on beat
- **M_MooaToon_AuroraSong** — aurora waves sync to song section

---

## 5. Organization Rules (Enforced)

### 5.1 Naming
- `M_*` = master material
- `MI_*` = instance (organize hierarchically mirror masters)
- `ML_*` = material layer
- `MLB_*` = material layer blend
- `MF_*` = material function
- `MPC_*` = material parameter collection
- `T_*` = texture

**Forbidden:** `_BS`, `_BSS`, spaces, diacritics

### 5.2 Folder Organization
- Role determines folder, NOT author
- Hierarchy: `/Game/Melodia/Materials/{Category}/{Subcategory}/`
- Keep material asset + instance pairs in sync

### 5.3 Plugin Assets (NEVER DELETE FROM /GAME)
MooaToon core assets MUST stay in `/MooaToon/` plugin:
- `/MooaToon/MaterialLayers/…`
- `/MooaToon/MaterialLayerBlends/…`
- `/MooaToon/Materials/…`

**Rule:** If core MooaToon asset is missing from /Game, duplicate from plugin to `/Game/Melodia/Materials/Layers/` (not a complete copy; just the missing piece).

---

## 6. Migration Status (2026-06-13)

### Current State
- **Total materials:** 209 (195 from old /Game/_PROJECT/, 13 from TurnBasedJRPGTemplate, 1 root)
- **Scattered locations:** /Game/_PROJECT/04_Materials/*, /Game/Materials/, /Game/TurnBasedJRPGTemplate/Materials/
- **Malformed paths:** 6 materials with `MooaToonM_*` pattern (folder naming corruption)
- **TEMP materials:** 2 marked for deletion (TEMP_DELETE, TEMP_T_Posters_01)

### Phase 2 Tasks
- [ ] Delete malformed-path materials (6) + TEMP materials (2)
- [ ] Create `/Game/Melodia/Materials/` folder structure
- [ ] Move materials from `/Game/_PROJECT/04_Materials/` → target hierarchy
- [ ] Move instances from `/Game/UPDATEDINSTANCES/` → `/Game/Melodia/Materials/Instances/`
- [ ] Move layers + blends from `/Game/MATERIALLAYERS/` → `/Game/Melodia/Materials/Layers/`
- [ ] Verify all 195 project materials compile + render
- [ ] Commit reorganized library to Git

---

## 7. Key Materials for Core Systems

### Rhythm Combat
- **MPC_MusicClock** — beat pulse parameter collection (fed by BP_MusicManager)
- **M_MusicalSDF_PulsingGeometry** — visual beat feedback
- **M_MooaToon_SoundWave_Pulse** — audio-reactive material

### Songcraft Magic
- **M_MooaToon_VinylGroove, InkSplash, GhostFlame, CrystalChime, AuroraSong** — instrument/spell visual signatures
- Custom MI_* instances for each material rarity tier (Common/Rare/Exotic)

### PCG Worlds
- **M_SDF_TrueParallax** — baroque facade (wallhi meshes)
- **M_Cosmo_Master** — cosmic background
- **M_Landscape_LayerBlend, M_Landscape_HeightBlend** — terrain
- **M_OilPainting_Gold_Baroque** — architectural surfaces
- **M_SDF_Baroque, M_SDF_GildedStucco, M_SDF_GildedFiligree** — ornament detail
- **M_Crystal_Clear_Toon, M_Crystal_Colored_Toon** — decorative elements

### UI
- **M_Transition** (from JRPG template, keep for reference)
- Custom MI_* instances for HUD elements (health bars, buttons, menu backgrounds)

---

## 8. Quality Assurance Checklist

Before shipping any material:
- [ ] Compiles with 0 errors
- [ ] Renders correctly in viewport (not black/DefaultMaterial)
- [ ] Instance parent locked (not accidentally reparentable)
- [ ] Parameters documented (if customizable)
- [ ] Instruction count reasonable for target (Toon < 200, SDF < 400, Advanced < 500)
- [ ] Nanite-safe if used with Nanite meshes (no global view params)
- [ ] Render-verified in test level with M_SDF_TestBench
- [ ] Audio-reactive materials wired to MPC_MusicClock (if applicable)
- [ ] Git committed with clean LFS tracking

---

## 9. Aesthetic Direction (Philosophy)

**Melodia's material language:**
- **Toon base:** Clean, readable, anime-inspired character silhouettes + environment
- **SDF procedural:** Inigo Quilez mathematical beauty (recursive, impossible, dreamy)
- **Oil paint:** Stylized low-poly, Infinity Nikki-like walkability + richness
- **Audio-reactive:** Materials pulse/shimmer/glow in time with music → music IS the magic
- **Japanese stylized:** SDF face shadows, toon outlines, matte surfaces, soft lighting
- **Melusina theme:** Lavender/purple tones, ornate baroque detail, surreal dream-logic geometry

**Never:** photorealism, flat colors, unlit environments without emotional purpose

---

## 10. Integration Discipline

**When adding NEW materials:**
1. Assess category (Toon? SDF? Advanced?)
2. Create in `/Game/Melodia/Materials/{Category}/`
3. Verify compilation + render
4. Create MI_* instance if customization needed
5. Document purpose + audio-reactivity (if any)
6. Add to core systems if needed (Rhythm, Songcraft, PCG, UI)
7. Commit with Git LFS tracking
8. Update this spec if adding new category or standard

**When researching new techniques (Shadertoy, Inigo Quilez, etc.):**
- Copy reference link to `/Docs/RESEARCH_shaders_procedural.md`
- Port technique to test material on `M_SDF_TestBench`
- Verify render quality + instruction count
- Only integrate if it serves the game (not for novelty)
- One technique at a time; commit after verification

---

## Reference

- **MooaToon plugin:** /Plugins/MooaToon/ (content-only, 3.2GB)
- **Material gotchas:** See memory file `mooatoon-mcp-material-gotchas.md`
- **Procedural research:** `/Docs/RESEARCH_shaders_procedural.md`
- **Core systems:** FOUNDATION.md §4 (Rhythm, Songcraft specs)
- **PCG materials:** PCG_SYSTEM_COHESION_2026-06-13.md

---

**Master Living Painting — Completed 2026-06-13**
A living specification. Update as materials are added, integrated, and verified.
