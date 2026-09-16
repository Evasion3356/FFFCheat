# FFFCheat

`FFFCheat.asi` patches Red Dead Redemption 2's single-player
`fillet_sp` script for game build 1491.50.

The decompiled `func_657` at bytecode position `0x1921A` is an unused
override in the input-validation path. The patch changes its return value
from `false` to `true`. The script still requires one of the four valid
fillet buttons to be pressed, but it no longer compares that button against
the generated sequence.

The mod waits for `fillet_sp` to be loaded, clones its script bytecode pages,
verifies the expected `PUSH_CONST_0` / `LEAVE` sequence, changes only the
constant to `PUSH_CONST_1`, and swaps the program to the cloned pages. The
original pages are restored when the script changes or the ASI unloads.

Build from this directory with:

```text
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" FFFCheat.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```

The post-build step deploys `FFFCheat.asi` to the configured RDR2 game
directory. The implementation targets the 1491.50 script layout and
bytecode offsets; a game update requires re-validating the guard bytes and
patch position.

Runtime status is written to `FFFCheat.log` in the same game directory. A
successful patch produces:

```text
fillet_sp patched: any valid button now counts as correct
```
