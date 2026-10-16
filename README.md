# Sega Ages 2500 C for m2-hle2

Sega's C from two PlayStation 2 releases: **Sega Ages 2500 Vol. 16: Virtua Fighter 2** (SLPM-62547) and **Vol. 19: Fighting Vipers** (SLPM-62606). The plan is to clean it up so it builds inside [m2-hle2](https://github.com/biggestsonicfan/m2-hle2), as [gems-c](https://github.com/biggestsonicfan/gems-c) did for Sonic Gems Collection.

The aim is C hooks for the emulator that are as close to the board as we can make them, not a copy of the PS2 games. Where the board does something else, follow the board. The faithful record of the PS2 C is [ages-decomp](https://github.com/biggestsonicfan/ages-decomp) (`decomp/`, once it's a submodule).

**State:** the source material is extracted (ages-decomp) and the tools that made it are here. No function is converted yet.

## What the PS2 games do

Both work the way Gems runs Sonic the Fighters. An interpreter runs the arcade's i960 program, unmodified: the programs on the discs are byte-identical to MAME's `vf2` (Revision 2.1) and `fvipers` (Revision D). Two parts are native C instead:

- **Patched i960 functions.** At start-up `sysPatch` writes a table of patches over the decoded i960 program (VF2: 295 records, FV: 183). Most records are an extension call (opcode 0, `sysExtend(n)`); the rest are branches. The extension cases that call a game function are the translated functions: 12 in VF2 (`sysCopPatchCalcUnitMat`, `sysCopPatchOsageDsp`...) and 46 in FV (`calc_unit_mat`, `osage_dsp`, `play_motion`, `rob_disp`...). The other cases are PS2 glue (sound, menus, textures, LOD). The ABI is Gems': the native function is called with 1, works on the i960 register file and bus (`i960GetRegs`, `sysMemRead*` / `sysMemWrite*`), and returns through `i960ReturnProc()`.
- **The coprocessor.** One handler per COP command, through a table that carries the firmware's `Fn_*` names: 112 commands for VF2's Model 2A TGP, 127 for FV's Model 2B SHARC. VF2's run on VU0 in macro mode.

Fighting Vipers shares the engine with Sonic the Fighters, so much of its `FN` list has the same names as Gems' traps (`calc_unit_mat`, `get_frame_dat`, `coli_cont_cop`, `osage_dsp`, `set_obj`...).

## Plan

1. **ages-decomp and ages-c on GitHub.** The owner creates `biggestsonicfan/ages-decomp` and `biggestsonicfan/ages-c`. Then ages-decomp is pushed and added here as the submodule `decomp/`.
2. **Fighting Vipers first.** m2-hle2 already runs `fvipers` (`src/profiles/fvipers.h`). Convert the 46 `FN` roots to `fn/<arcade name>.h` in gems-c's form (`static uint32_t afn_<name>(int entry)`), with a hook at each patch site, behind an `--ages-i960` flag. Name the remaining FV helpers first: 94 of the 112 are still `FUN_` (see "Naming" below).
3. **The FV COP**, `cop/NN_<name>.h`, behind `--ages-cop`. That includes the Model 2B commands Gems never had (`Fn_area_coli`, `Fn_kage_poly`...). gems-c's `sharc/` covers most of the rest, so compare the two before converting.
4. **A verify mode** like `--gems-verify`: run each hooked function both ways and log what differs from the i960.
5. **Virtua Fighter 2.** m2-hle2 has no VF2 profile yet; it needs one for the Model 2A board (`BOARD_MODEL2A_CRX`), and the arcade program needs labels to pair VF2's patch sites with function names.

What m2-hle2 gets will follow `src/core/gems.h`: an `ages.h` with the runtime API and the tables, a CMake option `M2HLE_AGES_DIR` pointing at this repository's `ages_impl.h`, and flags that log that they do nothing when the build doesn't have it. As with Gems, neither flag is the board: a hooked function is charged as one i960 instruction, and the PS2's COP is single-precision VU0/FPU arithmetic, not the TGP or SHARC. A netplay session turns both off.

## Layout

| path | what |
|---|---|
| `tools/` | everything used to extract and name the source material (below) |
| `decomp/` | (to come) the submodule ages-decomp |
| `fn/`, `cop/`, `gen_all.py`, `ages_impl.h`, `check.sh` | (to come) the conversion, in gems-c's layout |

No game data is in either repository. The tools need your own copies of the discs.

## Tools

`tools/run_all.sh <VF2 disc dir> <FV disc dir> <work dir> <out dir>` rebuilds ages-decomp from the discs. It runs these steps:

| tool | does |
|---|---|
| `cvm.py <cvm> [outdir]` | lists or extracts a CRI CVM archive (`BIN.CVM`: an ISO 9660 image at offset `0x1800`) |
| `cricmp.py <in> <out>` | unpacks a CRI `CRICMP` file (`CMP_DecSlideStd`), such as the i960 programs (`IC12_15.CMP`, `ROM_CODE1.CMP`) |
| `ghidra/ExportFuncs.java` | headless post-script: writes `functions.tsv` (address, name, size, callees) for a program |
| `match.py` | names FV's functions by matching them against VF2's: masked-instruction hashes, address order, call lists, gaps between matches |
| `ages_scan.py` | dumps an executable's patch table, extension switch and COP table to `patches.tsv`, `ext.tsv`, `cop.tsv` |
| `ages_roots.py` | picks the functions to export (`roots.tsv`): the extension cases that call game code, and every COP handler |
| `ida/dump_names.py`, `ida/dump_calls.py` | IDA scripts: the arcade program's names, and each function's calls in order |
| `i960names.py` | names FV's roots after the arcade function at their patch site |
| `arcade_align.py` | names FV's helpers by lining up each named function's calls against its arcade original's |
| `ghidra/AgesTypes.java` | gives the runtime calls prototypes, and the register file a struct |
| `ghidra/AgesExport.java` | decompiles the roots and their callees (three deep) to one file each, with `helpers.tsv` |
| `export.sh` | runs the two scripts above on both programs, then `make_index.py` |
| `make_index.py` | writes each folder's `INDEX.md` |
| `audit.py` | writes `AUDIT.md` (lizard) |

The table addresses that `run_all.sh` passes to `ages_scan.py` were found by reading the code:

| | VF2 | FV |
|---|---|---|
| patch table (`g_patchTbl`) | `0x20ab70` | `0x288fe0` |
| `sysPatch` | `0x1b5f00` | `0x1c41c0` |
| extension switch (`sysExtend`) | `0x1b5480`, table `0x20b950`, 0x6a cases | `0x1c2dc0`, table `0x2898d0`, 0x8f cases |
| COP table (`g_JumpTbl`) | `0x207a50` | `0x23fc20` |

VF2's come from its symbol table. FV's were found by looking for the same loop shapes in its decompiled code.

## Naming

VF2's names are Sega's own, from the symbol table. FV is stripped, and its names come from three places:

1. **VF2:** 2,112 of 3,163 functions match (`decomp/FV/names_from_vf2.tsv`): the shared runtime, the interpreter, the COP and the libraries. None of the 141 functions in FV's game code (`0x1c8000`-`0x200000`) match; it differs too much from VF2's.
2. **The arcade patch sites:** all 46 roots, from a labelled IDA database of `fvipers`.
3. **Call alignment against the arcade:** 14 helpers so far (`decomp/FV/names_from_arcade.tsv`). Only calls into the game code count. A gap between two calls both sides agree on pairs up only if it's the same length on both sides. A looser pairing was tried and gave wrong names (a PS2 LOD check came out as `set_ashi_ptn`).

The IDA database's function ends are incomplete for some functions, so their call lists are short. Fixing those in IDA and running `ida/dump_calls.py` again should name more helpers.

## Ghidra

Ghidra 12.1.4 with [ghidra-emotionengine-reloaded](https://github.com/chaoticgd/ghidra-emotionengine-reloaded) v2.1.38 (the release built for 12.1.4; it needed no changes). Both executables load as `r5900:LE:32`. VF2's CodeWarrior symbols come in with it.
