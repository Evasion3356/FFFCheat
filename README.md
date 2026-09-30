# FFFCheat

`FFFCheat.asi` is a ScriptHookRDR2 mod for Red Dead Redemption 2's
single-player Five Finger Fillet (`fillet_sp`, game build 1491.50).

## Features

Each feature can be switched off in `FFFCheat.ini`, which is created next to
`FFFCheat.asi` and re-read every time you sit at the table:

```ini
[General]
AnyButtonCounts=true
IgnoreEarlyPress=true
HideTimerMessage=true
```

- **AnyButtonCounts**: any valid fillet button counts as the correct one,
  whatever the generated sequence is.
- **IgnoreEarlyPress**: a press made before the prompt shows, or right after
  a flourish, is ignored instead of counting as a miss.
- **HideTimerMessage**: removes the "MESSAGE" placeholder that covers the
  timer. That's a vanilla-game bug: a game update added a message line to
  the shared minigame timer UI, and `fillet_sp` never fills it in.

## How it works

The first two features patch `fillet_sp`'s bytecode in place, and the third
redirects one entry in the script's native table. Everything is checked
against the exact bytes of the traced 1491.50 script before anything is
written. On a mismatch nothing is applied, and the log says which guard
failed. Everything is restored when the ASI unloads while the script is
still loaded.

Runtime status is written to `FFFCheat.log` in the game directory (or
`%LOCALAPPDATA%\RDR2ASIMods\FFFCheat.log` if that isn't writable). A
successful load logs lines such as:

```text
fillet_sp patched: any-button
fillet_sp patched: ignore-early-press
fillet_sp patched: ignore-early-press-no-phantom-miss
fillet_sp patched: hide-timer-message
```

## Building

Build from this directory with:

```text
"C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" FFFCheat.vcxproj /p:Configuration=Release /p:Platform=x64 /nologo /v:minimal
```

Run `git submodule update --init` after cloning. The post-build step deploys
`FFFCheat.asi` to the RDR2 install directory. A game update requires
re-validating the guard bytes and offsets.
