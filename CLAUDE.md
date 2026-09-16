# FFFCheat

`FFFCheat.asi` is a ScriptHookRDR2 ASI for RDR2 build 1491.50. It patches
the single-player `fillet_sp` script so a valid Five Finger Fillet button
press counts as correct regardless of the generated sequence.

## Implementation

`src/FFFCheat.cpp` finds the loaded `rage::scrProgram` named `fillet_sp`,
clones all of its bytecode pages, modifies the clone, and changes the
program's code-page table to the clone. The original table is restored when
the script unloads or the ASI unloads.

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

## Coding conventions

- Use C++ casts (`static_cast`, `reinterpret_cast`, `const_cast`), never
  C-style casts.
- Do not add fixed-size C string buffers or `sprintf`-style formatting.
  Use `std::string` or `std::ostringstream`.
- Log through `Log::Write`; runtime output goes to `FFFCheat.log` in the
  game directory.
- Keep modifications surgical. The copied bytecode pages must remain owned
  by `PatchedProgram` until the original page-table pointer is restored or
  the target script unloads.

## Build and deploy

Build from this directory:

```text
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" FFFCheat.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```

`Debug|x64` is also available. The project deploys `FFFCheat.asi` to:

```text
E:\SteamLibrary\steamapps\common\Red Dead Redemption 2
```

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
- `src/FFFCheat.cpp` owns page cloning, bytecode validation, patching, and
  restoration.
- `src/GamePointers.cpp` locates the game’s script-program table and finds
  a program by its JOAAT hash.
- `src/PatternScan.cpp` provides the main-module signature scanner and RIP
  address resolver.
- `external/RDR-Classes/` contains the minimal RAGE script-program
  definitions needed by this project.

## If the guard fails after a game update

Do not change the expected bytes blindly. Re-decompile the matching
`fillet_sp` script, locate `func_657`, inspect its raw code-page bytes, and
update both the offset and the complete guard together. If `func_657` no
longer returns a constant false value, retrace the input-validation path
before designing a new patch.
