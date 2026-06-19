# Melodia — Project Health Review & UE 5.8 Separation Plan

**Engine:** MooaToon UE 5.7.2 (precompiled fork) · **Module:** `MelodiaMelusina_PROD` (~165 files / ~28.5k LOC)
**Living document** — maintained by the `/loop` review (cron `e21c5d3f`). Data-verified claims only; unverified items are tagged `[NEEDS-VERIFY]`.

Started: 2026-06-18 · Last updated: iteration 1 (2026-06-18)

---

## 0. Executive summary

The recent instability was **not** the engine and **not** the 5.8 fork — it was AI-generated PCG/bootstrap code in the project's own C++ module. Three issues are now fixed and verified this session. The remaining items are mostly **hygiene and fragility** (git state, build toolchain, dormant over-engineered PCG code, material robustness), plus a deliberate **5.8 separation plan** so toon-shading experimentation never destabilizes the MooaToon 5.7 production line.

Severity scale: **P0** crash/blocking · **P1** high (data-loss / correctness risk) · **P2** medium · **P3** cleanup.

---

## ⮕ DIRECTION PIVOT (2026-06-18, user) — clean-slate UE 5.8 environment portfolio

**Decision:** Stop fixing/growing Melodia-the-game. The user is professionally an **environment designer**; the real goal is *"create lots of clean iterations of different environments in different styles, using UE purely as a tool."* → a **fresh, lean UE 5.8 project** for an environment-design portfolio. Back to basics, only what works.

**Why this supersedes the migrate-Melodia plan:** sidesteps every open risk above — no MooaToon fragility, no forward-resave disaster (clean slate ≠ migration; assets are re-authored selectively), no broken gameplay systems, official engine support.

**5.8 toon shading — VERIFIED (web, 2026-06):** UE 5.8 ships **Experimental Substrate Toon Shading** (Toon BSDF + Toon Profile asset; ramp diffuse/specular, hatching, anisotropic spec; Lumen + all light types). Caveats: **Experimental**, **requires Substrate ON**, **no forward rendering** yet — all fine for a deferred desktop portfolio. **Outlines** = post-process material (DIY or cheap marketplace e.g. CelEdge / Cel Shader Pro), not an engine toggle.

**Essentials-only sandbox (enable nothing else):**
- Engine: Lumen + Nanite (default), **Substrate ON**, **MegaLights** (production-ready in 5.8).
- Plugins: **PCG + PCGExtendedToolkit** (simple working scatter), **Movie Render Graph** (renders/flythroughs), Modeling Tools, Water (optional). NO gameplay / MCP / MooaToon.
- Scaffolding: reusable **`_Template` level** (PostProcess vol w/ Toon Profile + outline, lighting rig, CineCamera flythrough, locked exposure) duplicated per style; content `/Environments/<Style>/` + `/Shared/`.

**Carry over (re-author, not bulk-migrate):** Blender `*_greybox` style toolset (baroque/art-nouveau/brutalist/islamic); selected env art/materials; PCG scatter *concept* rebuilt simple.
**Leave behind in Melodia:** gameplay C++ (battle/dialogue/rhythm/JRPG), portfolio auto-bootstrap, MooaToon binding, marketplace bloat.

**Repo:** fresh clean LFS repo. Decision: reuse the existing `ue58` remote (reset clean) vs. a brand-new repo. `[NEEDS user decision]`

**Day-to-day loop:** greybox in Blender (style addon) → new level from `_Template` → PCG scatter → light + toon/outline → Movie Render Graph → commit. Repeat per style.

---

## 1. Fixed & verified this session

| # | Issue | Root cause | Fix | Evidence |
|---|-------|-----------|-----|----------|
| A | Editor crash on every launch (P0) | `PumpEditorUntilPCGComplete` called `GEditor->Tick()` from inside `OnConstruction` during startup map-load → nested dynamic-resolution `BeginFrame` → fatal assert | Pump ticks only the PCG subsystem; OnConstruction kicks async generate (commit `cc764f0`) | Editor launched clean; bootstrap completed the previously-fatal `RepopulatePortfolioTerraceLevel`; 0 crash markers |
| B | Per-launch "portfolio system" churn (P1) | `UMelodiaEditorContentBootstrap` regenerated + re-saved demo/portfolio levels every launch | Whole bootstrap gated behind `bEnableAutoBootstrap` (default OFF) (commit `92b511d`) | Log: `auto-bootstrap disabled`; `.umap` clean after launch (NO CHURN) |
| C | Unity-build ODR collision (P2) | `PCGBezier{Terrace,Cloister}Settings.cpp` defined identical `MakeSeparatePointPins` in a shared namespace | Per-file private namespaces (in `cc764f0`) | Module compiles & links clean |
| D | 2 broken marketplace packs (P3) | OGMainMenu + AdvWorldInteraction non-functional, ~723 MB | Deleted (commit `3451c6e`) | 722.6 MB freed; tree clean of them |

`origin/master` is up to date through `3451c6e`. The bootstrap-disable (`92b511d`) is **committed on local master, not yet pushed** (origin ahead-1) — awaiting user OK.

---

## 2. Open issues / risks

### P1 — Git hygiene (high: risk of lost work + unclear state)
- **~213 uncommitted entries / 150 untracked.** Large amounts of WIP (content edits, Scripts, the whole `Scripts/blender_*` greybox toolset) have never been committed. Risk: accidental loss, no recovery point, unreviewable diffs.
- **`.gitignore` gaps:** `build_log.json`, `build_log.uba`, `build_log-backup-*.txt` are untracked but should be ignored (only `build_log.txt` is). Likewise many `Scripts/_*.txt` scratch dumps.
- **OFPA actor data untracked (iter 2, potential data gap):** `Content/__ExternalActors__` (126 files) and `Content/__ExternalObjects__` (18 files) are **entirely untracked** (0 tracked, ~2 MB). These hold World-Partition / One-File-Per-Actor placed-actor data. If any level uses WP/OFPA, a committed `.umap` without these loses its actors on a fresh clone. *Action:* confirm whether levels use WP; if so, track them — if not, ignore them explicitly. `[NEEDS user decision]`
- **`Content/_PROJECT` (1.3 GB / 1839 files) is properly tracked** (LFS) — only 1 stray untracked file. Not a gap.
- **`Content/Python`** = the 2 UE-side live-link files I staged (`init_unreal.py`, `livelink_unreal.pyc`) — commit candidate once you confirm the live-link setup.
- **Branch/remote sprawl:** remotes `origin` (master, ue5.8-eval) and `ue58` (main, master); local branches `master`, `ue5.8-eval`, plus the two session fix branches. No documented strategy for which remote/branch is authoritative for what.
- **Fix:** (1) extend `.gitignore` (draft below); (2) resolve the OFPA question; (3) triage the WIP into logical commits (Blender greybox toolset; content; config) or explicitly discard; (4) write a one-paragraph branch/remote policy. Low risk, high payoff. `[PROPOSED — needs user OK before any commit/push]`

**Draft `.gitignore` additions (proposed, NOT applied — review the `Scripts/_*` globs against real scripts first):**
```gitignore
# Build logs / accelerator artifacts
build_log.json
build_log.uba
build_log*.txt
build_log-backup-*.txt
# Editor/tool scratch dumps
Scripts/_*.txt
.cursor/_*.txt
```

### P2 — Build toolchain fragility
- Precompiled, stripped MooaToon engine: **cannot build engine C++ plugins**; game module builds fine.
- **UBA memory-kill loop** under RAM pressure: with Blender (×3 heavy) + Substance open, system commit hit UBA's threshold → every compile worker killed + retried forever (336 MB log before I killed it). Also transient `paging file too small` (C3859/C1076) under 20-way parallel PCH.
- **Root cause:** 64 GB RAM + heavy concurrent DCC apps + UBA's commit threshold + lazy pagefile growth.
- **Fix (proven this session):** build with `-NoUBA -MaxParallelActions=4` (or free RAM first). Optionally make persistent via `BuildConfiguration.xml` (`bAllowUBAExecutor=false`). Document in README. `[VERIFIED workaround]`

### P2 — Dormant over-engineered PCG code
- The auto-bootstrap is disabled, but the **portfolio/bezier toolkit it drove still exists** (`MelodiaPCGLevelKit`, `PCGBezier*Settings`, `MelodiaEditorContentBootstrap`, bezier preset library). This is the "portfolio system" the user did not want; they want simple constrained graphs.
- **Fix (proposed):** decide keep-dormant vs. trim. If trimming, remove the bootstrap + bezier portfolio kit, keep minimal place-and-constrain PCG kits. `[NEEDS user decision]`

### P2 — Material / shader robustness `[NEEDS-VERIFY]`
- Project history (memory) records repeated "compiles but renders DefaultMaterial" silent fallbacks and MooaToon-specific Custom-node / Nanite-permutation quirks. Not re-checked this session.
- **This is the single biggest risk to any 5.8 migration** (toon materials are authored against MooaToon's engine shading).
- **Fix:** a data-verified material audit (render/instruction-count checks, not compile-success). Scheduled for a later iteration.

### ~~P3 — Latent unity collisions~~ `[AUDITED — clean, iter 2]`
- Grepped all `namespace` declarations across the module: **all 15 named namespaces are unique** and there are **0 anonymous namespaces** (the classic unity-ODR source). The fixed `MakeSeparatePointPins` collision was the only one. Residual risk: file-scope `static` free functions with identical signatures (not exhaustively diffed) — low, deferred.

---

## 3. Prioritized fix plan

1. **[done]** P0 crash, P1 bootstrap churn — fixed & verified.
2. **P1 git hygiene** — `.gitignore` patch + WIP triage + branch/remote policy. *(low risk; needs OK to commit)*
3. **P2 build standardization** — document `-NoUBA -MaxParallelActions=4`; consider `BuildConfiguration.xml`.
4. **P2 PCG trim decision** — keep dormant vs. remove portfolio/bezier toolkit. *(needs user decision)*
5. **P3 unity-collision audit** — grep sibling files, fix any found.
6. **P2 material audit** — data-verified render checks; informs 5.8 feasibility.

---

## 4. UE 5.8 separate-repo plan (draft — no execution without approval)

**Current state (data):** remote `ue58` = `github.com/fromage3900/melodia-ue58.git` (branches `main`, `master`); local branch `ue5.8-eval` + `origin/ue5.8-eval`; scaffolding commit `bc95996`; `Open Melodia Editor UE58.bat` points to **stock Epic `UE_5.8`** (NOT a toon engine) and is **guarded** to refuse running against the MooaToon-bound `.uproject`.

**Goal:** an independent 5.8 experimentation line where toon-shading work happens without ever forward-resaving / destabilizing MooaToon 5.7 production on `master`.

**Recommended topology:** a **separate working directory** (e.g. `G:\Melodia-UE58\`) cloned from the `ue58` remote — *not* a branch in the same working tree. Reason: UE asset upgrades are forward-only; mixing 5.7-MooaToon and 5.8-stock content in one tree invites the resave disaster.

**Open questions before committing to 5.8 (decision-gating):**
- **Does stock UE 5.8 actually ship a usable in-engine toon/NPR shading model, or just Substrate + DIY?** `[NEEDS-VERIFY — research offered, not yet run]` This is the make-or-break fact.
- If yes: does it reproduce the target Genshin/HSR look at MooaToon quality? Native toon ≠ MooaToon toon; existing materials won't port — they'd be rebuilt.

**Draft migration/separation steps (for the 5.8 tree only):**
1. Clone `ue58` remote into `G:\Melodia-UE58\`; set its own EngineAssociation to stock UE 5.8.
2. Reconcile plugins: audit the `.uproject` plugin list against stock 5.8 (drop MooaToon-only / unavailable; verify UnrealMCP, VRM4U, PCGExtendedToolkit, etc.).
3. Migrate a *small* content slice first (one level + a few materials) and resave in 5.8 — measure how badly toon materials break. Do **not** bulk-resave.
4. Run the toon-look validation experiment; decide go/no-go.
5. Keep remotes strictly separated; never auto cross-push 5.7↔5.8.

---

## 4b. 5.8 sandbox build-out specs (iterative, grounded in the real project)

### Item 1 — Project setup spec  `[grounded in BS_GodFile.uproject, iter 4]`
**Project:** `G:\EnvironmentPortfolio\BS_GodFile\BS_GodFile.uproject` — UE 5.8, C++ (module `BS_GodFile`, minimal stub), engine GUID `{A09DB5AA-…}`.

**Already configured (verified in `DefaultEngine.ini`) — a good clean base:**
- `r.Substrate=True` ✅ — **required for the 5.8 toon model, already ON**
- `r.DynamicGlobalIlluminationMethod=1` + `r.ReflectionMethod=1` = **Lumen** GI + reflections ✅
- `r.Shadow.Virtual.Enable=1` (Virtual Shadow Maps) ✅ · `r.GenerateMeshDistanceFields=True` ✅
- `r.AllowStaticLighting=False` (fully dynamic) ✅ · `r.RayTracing=True` ✅
- Plugins enabled: only `ModelingToolsEditorMode` (clean — no bloat) ✅

**To ADD for the env-portfolio essentials (exact `.uproject` plugin names):**
| Need | Enable | Notes |
|------|--------|------|
| PCG scatter | **`PCG`** | built-in |
| PCG advanced nodes | **`PCGExtendedToolkit`** | Fab plugin — **verify installed for 5.8** `[NEEDS-VERIFY]` |
| Portfolio renders / flythrough | **`MovieRenderPipeline`** | Movie Render Graph is production-ready in 5.8 |
| Geometry ops for PCG | **`GeometryScripting`** | optional |
| Asset import (Megascans/Fab) | **`Fab`** | for kit-bashing environments |
| Water environments | **`Water`** | optional |
| MegaLights | ini: `[/Script/Engine.RendererSettings]` → `r.MegaLights.EnableForProject=1` | strong for many-light env scenes |

**Substrate Toon (the whole point) — `[NEEDS in-editor verify]`:** Substrate is on; the 5.8 toon look is an *experimental* Substrate Toon BSDF + a **Toon Profile** asset. To confirm in the running editor: whether it needs an extra experimental toggle or a specific `r.Substrate.ProjectGBufferFormat` (currently `0`; NPR is documented as using the "Blendable GBuffer (legacy)" path). **Do not change blindly — verify in-editor first.**

**Keep OUT:** gameplay plugins, MCP/VibeUE, MooaToon. None present — keep it that way.

---

## 5. Iteration log
- **Iter 1 (2026-06-18):** Created this doc. Recorded fixed items A–D, open issues, prioritized plan, and 5.8 plan skeleton. Next: (a) draft `.gitignore` patch for build logs/scratch; (b) audit remaining untracked content trees (`Content/_PROJECT`, `__ExternalActors__/__ExternalObjects__`); (c) grep for further unity-collision risk; (d) tee up the 5.8 NPR/toon research question for the user.
- **Iter 2 (2026-06-18):** Did (a)(b)(c). Unity-collision audit → **clean** (15 unique namespaces, 0 anon namespaces); P3 closed. Untracked-content audit → found OFPA `__ExternalActors__`/`__ExternalObjects__` entirely untracked (potential data gap); `_PROJECT` (1.3 GB) properly tracked. Drafted `.gitignore` additions (not applied).
- **Iter 3 (2026-06-18, USER PIVOT):** User clarified the real goal — environment-design portfolio, UE-as-tool, many styled iterations. Verified UE 5.8 native toon (Experimental Substrate Toon). Added the **DIRECTION PIVOT** section; plan reframed from "migrate Melodia" → "clean-slate 5.8 env sandbox." Review loop re-pointed to this goal. Next: (1) project setup spec (plugins, Substrate, render settings); (2) `_Template` level spec; (3) carry-over prep for the Blender greybox toolset; (4) repo decision (reuse `ue58` vs new).
- **Iter 4 (2026-06-18):** Did item (1) **grounded in the real `BS_GodFile.uproject`** — found Substrate/Lumen/VSM/RT/dynamic-lighting already ON, only `ModelingToolsEditorMode` enabled. Wrote §4b Item 1 spec: exact plugins to add (`PCG`, `PCGExtendedToolkit`, `MovieRenderPipeline`, `GeometryScripting`, `Fab`, `Water`) + `r.MegaLights.EnableForProject=1`, and flagged the Substrate-Toon enable as in-editor-verify. Next: item (3) inventory the Blender `*_greybox` toolset under `Scripts/` (carry-over), then item (2) `_Template` level spec.
