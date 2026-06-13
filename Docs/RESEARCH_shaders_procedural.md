# Research — Shaders & Procedural (REFERENCE ONLY, not yet integrated)
*Saved 2026-06-13. Study material for LATER, disciplined, one-at-a-time integration AFTER the foundation is locked + committed. Do NOT bulk-integrate. Most of these are references/tools, not drop-in UE assets.*

## Status legend
- 📚 study/reference (port or learn from by hand)  · 🛠️ external tool (separate app) · ⭐ highest aesthetic fit for Melodia

## Tier 1 — Artistic mathematics (the foundation references)
- 📚⭐ Inigo Quilez — https://iquilezles.org/ — SDFs, raymarching, fractals, palettes. THE reference. Port techniques into MooaToon Custom HLSL nodes.
- 📚 Shadertoy — https://www.shadertoy.com/ — community shaders (GLSL → port to HLSL).

## Tier 2 — Math-based world gen (study)
- 📚 ProceduralMeshDemos — https://github.com/SiggiG/ProceduralMeshDemos (Sierpinski/recursive fractal meshes, C++)
- 📚 Procedural-Terrain-Generation — https://github.com/drigil/Procedural-Terrain-Generation (Perlin + vertex shaders)
- 📚 PerlinNoise-Generation — https://github.com/husain34/PerlinNoise-Generation (islands/caves/clouds)
- 📚 procedural-map-gen — https://github.com/mjvar/procedural-map-gen (Voronoi biomes)
- 📚 voroTerrain — https://github.com/gwio/voroTerrain (Voronoi, OpenFrameworks)

## Tier 3 — Asian implementations (study)
- 📚 weiyinfu/Fractal — https://github.com/weiyinfu/Fractal (IFS fractals)
- 📚 L-System implementations (organic growth) · FractalDB datasets

## Advanced stylized / Japanese rendering (⭐ best Melodia fit — study, implement selectively)
- 📚⭐ SDF face shadow mapping (anime lighting) — https://unrealengine.hatenablog.com/entry/2024/02/28/222220
- 📚⭐ Epic "Stylized Rendering Insights from Japan" — https://dev.epicgames.com/community/learning/talks-and-demos/7evy/unreal-engine-stylized-rendering-insights-from-japan
- 📚 Custom shading models (engine-level) — https://kafues511.jp/2025/02/15/3285/ (needs C++ engine mod — heavy; MooaToon already provides toon shading, prefer that)
- 📚 Historia custom raytrace — https://historia.co.jp/archives/55185/

## Tools (external apps — NOT integrated into project)
- 🛠️ Material Maker (procedural textures) · 🛠️ World Creator (GPU terrain+erosion) · 🛠️ Tilemancer (tiles)

## VFX / aesthetic
- 📚⭐ BUKKBEEK — https://bukkbeek.github.io/ (stylized low-poly + EffectBlocks VFX approach)
- Youfulca — music aesthetic reference (Infinity Nikki match)

## Integration discipline (when the time comes — NOT before foundation is locked + committed)
1. Foundation first: project opens clean, git+LFS committed, rhythm hooks re-applied, vertical slice playable.
2. Then pick ONE technique, implement on a TEST asset (M_SDF_TestBench), render-verify, commit. Never bulk.
3. Prefer MooaToon Material Layers (existing pipeline) over engine C++ shading-model mods.
4. SDF face shadows + stylized-rendering talk = highest-value first candidates for the Melodia look.
- Related prior work already in project: 4 procedural ML layers (Mandelbrot/Perlin/FractalOrnament/LSystemVine), MPC_MusicClock beat-reactive materials.
