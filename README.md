# Sega Ages 2500 C for m2-hle2

Sega's C from two PlayStation 2 releases: **Sega Ages 2500 Vol. 16: Virtua Fighter 2** (SLPM-62547) and **Vol. 19: Fighting Vipers** (SLPM-62606). The plan is to clean it up so it builds inside [m2-hle2](https://github.com/biggestsonicfan/m2-hle2), as [gems-c](https://github.com/biggestsonicfan/gems-c) did for Sonic Gems Collection.

The aim is C hooks for the emulator that are as close to the board as we can make them, not a copy of the PS2 games. Where the board does something else, follow the board. The faithful record of the PS2 C is [ages-decomp](https://github.com/biggestsonicfan/ages-decomp) (the submodule `decomp/`).

**State:** the source material is extracted (ages-decomp) and the tools that made it are here. One Fighting Vipers root is converted and hooked: `efc_face_change_cont` (`fn/efc_face_change_cont.h`), behind m2-hle2's `--ages-i960`, and checked against the i960 with `--ages-verify` (see "Checking").

## What the PS2 games do

Both work the way Gems runs Sonic the Fighters. An interpreter runs the arcade's i960 program, unmodified: the programs on the discs are byte-identical to MAME's `vf2` (Revision 2.1) and `fvipers` (Revision D). Two parts are native C instead:

- **Patched i960 functions.** At start-up `sysPatch` writes a table of patches over the decoded i960 program (VF2: 295 records, FV: 183). Most records are an extension call (opcode 0, `sysExtend(n)`); the rest are branches. The extension cases that call a game function are the translated functions: 12 in VF2 (`sysCopPatchCalcUnitMat`, `sysCopPatchOsageDsp`...) and 46 in FV (`calc_unit_mat`, `osage_dsp`, `play_motion`, `rob_disp`...). The other cases are PS2 glue (sound, menus, textures, LOD). The ABI is Gems': the native function is called with 1, works on the i960 register file and bus (`i960GetRegs`, `sysMemRead*` / `sysMemWrite*`), and returns through `i960ReturnProc()`.
- **The coprocessor.** One handler per COP command, through a table that carries the firmware's `Fn_*` names: 112 commands for VF2's Model 2A TGP, 127 for FV's Model 2B SHARC. VF2's run on VU0 in macro mode.

Fighting Vipers shares the engine with Sonic the Fighters, so much of its `FN` list has the same names as Gems' traps (`calc_unit_mat`, `get_frame_dat`, `coli_cont_cop`, `osage_dsp`, `set_obj`...).

## Plan

1. **ages-decomp and ages-c on GitHub.** Done: ages-decomp is the submodule `decomp/` (`git clone --recursive`, or `git submodule update --init`).
2. **Fighting Vipers first.** m2-hle2 already runs `fvipers` (`src/profiles/fvipers.h`). Convert the 46 `FN` roots to `fn/<arcade name>.h` in gems-c's form (`static uint32_t afn_<name>(int entry)`), with a hook at each patch site, behind an `--ages-i960` flag. Started: the flag, the hook table and the first root (`efc_face_change_cont`) are in; 45 roots to go. 36 of FV's 112 helpers are still `FUN_` (see "Naming" below); 33 of them are outside the game code (runtime code VF2 didn't match), and the other 3 have no arcade name.
3. **The FV COP**, `cop/NN_<name>.h`, behind `--ages-cop`. That includes the Model 2B commands Gems never had (`Fn_area_coli`, `Fn_kage_poly`...). gems-c's `sharc/` covers most of the rest, so compare the two before converting.
4. **A verify mode** like `--gems-verify`: run each hooked function both ways and log what differs from the i960. Done for the i960 roots: `--ages-verify` is `--gems-verify`'s verifier.
5. **Virtua Fighter 2.** m2-hle2 has no VF2 profile yet; it needs one for the Model 2A board (`BOARD_MODEL2A_CRX`), and the arcade program needs labels to pair VF2's patch sites with function names.

m2-hle2's side follows `src/core/gems.h`: `src/core/ages.h` has the flags and the runtime API, and the CMake option `M2HLE_AGES_DIR` points at this repository's `ages_impl.h`. A build without it accepts the flags, which then log that they do nothing. As with Gems, neither flag is the board: a hooked function is charged as one i960 instruction, and the PS2's COP is single-precision VU0/FPU arithmetic, not the TGP or SHARC. A netplay session turns both off.

## Layout

| path | what |
|---|---|
| `tools/` | everything used to extract and name the source material (below) |
| `decomp/` | the submodule ages-decomp |
| `fn/<name>.h` | one patched i960 function: `static uint32_t afn_<name>(int entry)`. `entry` 1 = called from the patch site (pops the i960 frame itself, as the PS2's `i960ReturnProc`), 0 = called from other C. Returns 0 |
| `gen_all.py` | writes `ages_all.h`: prototypes, includes and `ages_patches[]`. Only functions that are defined go in the table; the patch sites come from `decomp/FV/FN/INDEX.md` |
| `ages_all.h` | written by `gen_all.py`; not tracked (`.gitignore`) |
| `ages_impl.h` | what m2-hle2's `ages.h` includes |
| `check.sh` | syntax-check everything against an m2-hle2 tree |
| `tools/run/runverify.sh` | run fvipers headless with `--ages-verify` and quit, so the report is written |
| `cop/` | (to come) the FV COP |

No game data is in either repository. The tools need your own copies of the discs.

## Building m2-hle2 with it

```bash
git submodule update --init decomp      # gen_all.py reads decomp/FV/FN/INDEX.md
cmake -S <m2-hle2> -B <build> -DCMAKE_BUILD_TYPE=Release -DM2HLE_AGES_DIR=<this repository>
cmake --build <build> --target m2hle
```

CMake runs `gen_all.py` when it configures, and again when a file in `fn/` changes. Then run m2-hle2 on the `fvipers` set (Revision D, the PS2's program; your own dump) with `--ages-i960`, or `--ages-verify` to check.

The C is written against `ages.h`, which is gems.h's runtime under `ages_` names, since the PS2's ABI is Gems': `AGES_R(n)`, `AGES_G(n)`, `AGES_AC`; `ages_ld8/8s/16/16s/32`, `ages_st8/16/32`, `ages_ldf/stf`, `ages_ldn/stn`; `ages_cop_w/wf/r/rf`; `ages_i960_ret`, `ages_branch`. The PS2's `sysMemRead*` / `sysMemWrite*` become loads and stores at the width and sign the i960 instruction has, and where the PS2 keeps something on the host (a static) that the board keeps in i960 memory, follow the board.

## Checking

```bash
MINIZ_GEN=<m2-hle2 build>/miniz-gen ./check.sh <m2-hle2 checkout>
M2HLE_EXE=<build>/Release/m2hle ROMS_DIR=<folder with fvipers.zip> tools/run/runverify.sh 150 --ages-verify
grep ages-verify /dev/shm/ages-run/m2hle.log
```

`check.sh` compiles `ages.h` with everything here (gcc `-fsyntax-only -Wall`). `--ages-verify` runs each hooked function both ways, as gems-c's `--gems-verify`: the C on a snapshot of the board, then the i960 from the same state to where the C resumed, and logs what differs; the board carries on from the i960's run. At exit it logs one row per function: calls / exact / float-close / regs-only / differ / lost.

Last run (300 s of fvipers attract): `efc_face_change_cont` 264 calls, 264 exact. The same run with the store put one slot off on purpose logs every call as `differ` (two RAM words). Before m2-hle2 #996, that wrong slot passed as float-close: small integers read as denormal floats, and the old tolerance took any two of them as close.

Run lizard (`/opt/fly/bin/lizard -w -C 15 -L 80 fn`) after changing any C here.

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
3. **Call alignment against the arcade:** 35 helpers (`decomp/FV/names_from_arcade.tsv`, `how` = `equal` or `aligned`). Only calls into the game code count. A gap between two calls both sides agree on pairs up only if it's the same length on both sides, and a pair is dropped if either side would get two names. A looser pairing was tried and gave wrong names (a PS2 LOD check came out as `set_ashi_ptn`).
4. **By hand:** 37 helpers (`tools/fv_by_hand.tsv`, `how` = `by hand`), each named from reading both sides, with the evidence in the file's third column. They go in before the alignment, so it can carry on from them.

`ida/dump_calls.py` follows each arcade function's code from its start rather than trusting IDA's function ends, which stop early in some functions. It follows jump tables (`play_motion` had 3 calls before, 31 after), writes an entry stub like `get_fcurve_value` as the function it falls into, and lists named code labels too (`tou_disp` is a label inside `ground_upper_disp`).

Three game-code callees of named functions stay `FUN_`, because they have no arcade name to take:

| FV | Called from | Why |
|---|---|---|
| `0x1cad40` | `rob_disp` | PS2-only: a LOD check |
| `0x1dbb00` | `os_set_matrix` (twice) | PS2-only |
| `0x1f5d80` | `tou_disp` (six times) | the arcade's unnamed `loc_22B84`, the body shared by `tower_check`, `hashira_check2` and `hashira_check`. Each of those loads its own table into `g2` and branches to it. The PS2 passes the table as an argument (`0x22b24`, `0x22b64`, `0x22b44`). |

## Ghidra

Ghidra 12.1.4 with [ghidra-emotionengine-reloaded](https://github.com/chaoticgd/ghidra-emotionengine-reloaded) v2.1.38 (the release built for 12.1.4; it needed no changes). Both executables load as `r5900:LE:32`. VF2's CodeWarrior symbols come in with it.
