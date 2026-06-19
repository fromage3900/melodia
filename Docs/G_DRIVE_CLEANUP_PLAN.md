# G: Drive Cleanup & Migration Plan

Generated 2026-06-18 from a live `du` scan. **Analysis only — nothing moved/deleted yet.** Awaiting user approval per category.

## Drives
| Drive | Label | Size | Free | Role |
|------|-------|------|------|------|
| **C:** | Windows | 953 GB | **21 GB ⚠️ critically low** | system |
| **F:** | Seagate Portable | 1863 GB | **1609 GB** | ← archive target (USB) |
| **G:** | Hatsune Miku Drive | 931 GB | **54.5 GB** | working drive (this cleanup) |

G: is ~886 GB used. Goal: free it up by archiving large inert data to F: and trimming junk.

---

## 🔴 The headline: `G:\MelodiaMelusina` = **305 GB**
That single folder is **a third of the drive** — and it was **modified today (6:07 PM)**, so it is NOT obviously a dead backup. The active project (`G:\Melodia`) is only 33 GB; `MelodiaMelusina` is almost certainly a bloated working copy (accumulated DerivedDataCache / Intermediate / Saved / duplicate content). **Biggest single win on the drive — but needs your call.** Options once you confirm what it is:
- If superseded → archive whole folder to F: (**+305 GB**), or
- If you want to keep it → just delete its regenerable `DerivedDataCache/`, `Intermediate/`, `Saved/` (likely 200 GB+ of that 305 is regenerable cache). `[NEEDS YOUR DECISION]`

---

## ✅ SAFE TO ARCHIVE → F: (inert, no path dependencies, reversible)
Move to `F:\_FromG_Archive\<category>\`. ~**285 GB** of easy wins:
| Folder | Size | Notes |
|--------|------|-------|
| `Renders` | 156 GB | render outputs — terminal, nothing references them |
| `cinnbunrender` | 80 GB | more render output |
| `Shepherd_Brennan_VP_Set` | 11 GB | school project archive |
| `Humber Semester 5` | 11 GB | school archive |
| `Shepherd_Brennan_Assignment_03` | 9.5 GB | school archive |
| `6000.3.7f1` | 7.7 GB | Unity 6 install/project |
| `pablender …zbrush brushes` | 5.7 GB | asset pack |
| `Stylized Enchanted Forest Asset Pack`, `Trimsheets`, `Shepherd_Brennan_10`, misc | ~3 GB | asset/school archives |

## ❓ DECISION NEEDED (big, but ambiguous — confirm before I touch)
| Item | Size | Question |
|------|------|----------|
| `MelodiaMelusina` | **305 GB** | superseded? (see headline) |
| `ueprojects` | 72 GB | any active UE project in here, or all old? |
| `programs` | 37 GB | installed/portable apps — some may have path/registry deps |
| `DuvetMikuV6` | 25 GB | still needed, or archive? |
| `$RECYCLE.BIN` | 13 GB | empty it? (deletion — given your drive's history I won't auto-empty) |

## ⛔ DO-NOT-TOUCH (load-bearing / active — moving breaks things)
| Item | Size | Why |
|------|------|-----|
| `MooaToon-main` | 42 GB | the engine — path baked into Melodia launch scripts + `.uproject` |
| `Melodia` | 33 GB | active git project (just pushed clean) |
| `EnvironmentPortfolio` / `BS_GodFile` | 2.8 GB | **active 5.8 sandbox, currently open/compiling** |
| `SteamLibrary` | 30 GB | move only via Steam's own "move install folder" UI, never by hand |
| `MooaToon-Engine-Precompiled-5.7…` | (skipped) | engine copy — verify before touching |

## 🗑 Likely junk (review, then delete)
- Loose `tmp*` files in G:\ root (temp downloads).
- Duplicate installers: `OperaGXSetup.exe` + `(1).exe`, `Voiceger-Setup-2.0.0` + `3.0.0`, etc.
- `.blend1` autosave backups scattered in root.

---

## Projected result
- Archiving the **SAFE** set → G: free goes **54 GB → ~340 GB**.
- Plus clearing `MelodiaMelusina` cache or archiving it → **~550–645 GB free**. Transformative.

## Method (when approved)
- `Move-Item` to F: = copy-then-delete, so the **source is preserved if the USB transfer is interrupted** (no data-loss risk; only a partial copy to clean up).
- Execute **one category at a time**, verify size on F: matches before deleting source, log each move.
- Do the SAFE set first; hold DECISION items for explicit per-item OK.

## Separate note: C: is at 21 GB free ⚠️
Some of your pain may be C:, not G:. Worth a look later (likely Epic/UE DDC, shader caches, `%LOCALAPPDATA%` bloat).
