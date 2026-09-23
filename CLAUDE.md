# FFFCheat

`FFFCheat.asi` is a ScriptHookRDR2 ASI for RDR2 build 1491.50. It patches
the single-player `fillet_sp` script so a valid Five Finger Fillet button
press counts as correct regardless of the generated sequence, and so presses
made too early are ignored instead of counting as misses.

## Implementation

`src/FFFCheat.cpp` finds the loaded `rage::scrProgram` named `fillet_sp`
and patches its bytecode in place (via `VirtualProtect`). Do
**not** clone the code pages or swap `m_CodeBlocks`: the game frees the
program's pages itself when the script unloads, and pointing it at our heap
memory crashed the game (error FFFFFFFF) on leaving the table. On ASI
unload the byte is restored only if the same program/page is still live.

The sole patch is in `fillet_sp.ysc.c`'s unused `func_657` override:

```c
BOOL func_657(var uParam0) // Position - 0x1921A
{
    return false;
}
```

The decompiler's position is the start of the function’s five-byte `ENTER`
prologue, **not** the return-value opcode. In build 1491.50 the complete
function byte sequence is:

```text
22 01 03 00 00 2F 50 01 01
ENTER          PUSH_CONST_0 LEAVE
```

`PUSH_CONST_0` is therefore at `0x1921F`, not `0x1921A`; the patch replaces
only `0x2F` with `0x09` (`PUSH_CONST_1`). The complete byte sequence is the
runtime guard. Do not weaken it or treat a guard failure as safe to patch:
it means the installed game/script layout differs from the traced build.

### Second patch: ignore early presses

`func_479` (0xFB17) treats a press as a miss when `f_86 > f_87` (the press
counter is ahead of the prompt counter, which is the case right after a
flourish until the next anim event) or when the input-window anim event
`-534847913` has not fired. The miss block starts at `0xFE02`
(`09 66 01`, i.e. `PUSH_CONST_1; LOCAL_U8_LOAD 1`) and ends with a `J` to the
function's `LEAVE` at `0xFEBA`. The patch overwrites those three bytes with
`68 B5 00` (`J +0xB5` -> `0xFEBA`), so an early press is dropped with no
penalty. Jump offsets are relative to the end of the `J` instruction.
Guards cover the condition (0xFDD8-0xFE0C), the block's closing `J`
(0xFE2E) and the `LEAVE` (0xFEBA). All guards are checked before any patch is
written; the patches apply all-or-nothing.

Offsets here come from disassembling `fillet_sp.ysc.full` with the
decompiler's `RDR1355OpcodeSet` map; the file offset is code offset + 176.

Bytecode pages are 0x4000 bytes, so code offsets must be resolved as:

```cpp
page = codeOffset >> 14;
offsetInPage = codeOffset & 0x3FFF;
```

The target file that was traced is:

```text
D:\Backup\Stuff\RDR2 Shit\Scripts\1491.50\script_rel\fillet_sp.ysc.c
```

The raw compiled reference used to confirm the byte sequence is:

```text
D:\Backup\Stuff\RDR2 Shit\SciptsCompile\script_rel\fillet_sp.ysc.full
```

## Configuration

`src/Config.cpp` reads `FFFCheat.ini` (created with defaults next to
`FFFCheat.asi` in the game directory) through inipp, a git submodule at
`external/inipp` (same commit PokerCheat uses). Each patch is gated by a
boolean in `[General]`, and disabled patches are neither guarded nor written:

```ini
[General]
AnyButtonCounts=true
IgnoreEarlyPress=true
```

The INI is re-read each time the `fillet_sp` script loads (i.e. each time you
sit at the table), so edits apply without restarting the game. Do not use
`<filesystem>` for config I/O: it overflowed ScriptHookRDR2's small fiber
stack in sibling projects. After cloning, run
`git submodule update --init`.

## Coding conventions

- Use C++ casts (`static_cast`, `reinterpret_cast`, `const_cast`), never
  C-style casts.
- Do not add fixed-size C string buffers or `sprintf`-style formatting.
  Use `std::string` or `std::ostringstream`.
- Log through `Log::Write`; runtime output goes to `FFFCheat.log` in the
  game directory, or `%LOCALAPPDATA%\RDR2ASIMods\FFFCheat.log` when that
  isn't writable (see `src/LogFallback.h`; `tests/LogFallbackTests.vcxproj`
  covers it).
- Keep modifications surgical. Never hand the game memory it did not
  allocate, and never dereference a cached `scrProgram*` after the script
  may have unloaded; compare pointers first.

## Build and deploy

Build from this directory:

```text
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" FFFCheat.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```

`Debug|x64` is also available. The project's `PostBuildEvent` auto-locates
the RDR2 install directory (`BuildTools\Find-RDR2GameDir.ps1` -- vendored
identically into every sibling project, since each is its own separate
git repo) and deploys `FFFCheat.asi` there, on every build regardless of
whether the build itself was up to date.
`DisableFastUpToDateCheck` is set in the `.vcxproj.user` so this also
holds for Visual Studio IDE builds, not just command-line MSBuild.

RDR2 must not have this ASI loaded while the post-build copy runs. Check:

```text
tasklist //FI "IMAGENAME eq RDR2.exe"
```

A successful run logs:

```text
fillet_sp patched: any valid button now counts as correct
```

## Source layout

- `src/main.cpp` registers `ScriptMain`; every frame calls
  `FFFCheat::OnTick()`.
- `src/FFFCheat.cpp` owns bytecode validation, in-place patching, and
  restoration.
- `src/GamePointers.cpp` locates the game’s script-program table and finds
  a program by its JOAAT hash.
- `src/PatternScan.cpp` provides the main-module signature scanner and RIP
  address resolver.
- `external/RDR-Classes/` (submodule, YimMenu/RDR-Classes) provides the RAGE
  script-program definitions; `external/ScriptHookSDK/` (submodule) provides
  the ScriptHookRDR2 headers and import lib.

## If the guard fails after a game update

Do not change the expected bytes blindly. Re-decompile the matching
`fillet_sp` script, locate `func_657`, inspect its raw code-page bytes, and
update both the offset and the complete guard together. If `func_657` no
longer returns a constant false value, retrace the input-validation path
before designing a new patch.
