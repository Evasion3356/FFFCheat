# Changelog

All notable user-facing changes to FFFCheat are recorded here. Format
loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [1.3.0] - 2026-09-23

### Fixed
- If `fillet_sp` doesn't match the expected game build, FFFCheat no longer
  re-reads `FFFCheat.ini` and writes to its log every frame while you sit
  at the table. It now checks once per table visit.
- The patch is re-applied correctly if the game reloads `fillet_sp` at the
  same memory addresses.
- Closing the game no longer tries to restore the patched bytes while the
  game is already shutting down.
- If the game folder can't be written (e.g. a `C:\Program Files` install,
  or a read-only/locked log file), the log now goes to
  `%LOCALAPPDATA%\RDR2ASIMods\FFFCheat.log` instead, and its first line
  names the path that couldn't be used.

## [1.2.0] - 2026-09-19

### Added
- Presses made before the prompt shows, or right after a flourish, are
  ignored instead of counting as misses.
- `FFFCheat.ini` (`[General]` `AnyButtonCounts`, `IgnoreEarlyPress`) turns
  each patch on or off. It's re-read every time you sit at the table.

## [1.1.0] - 2026-09-19

### Fixed
- Leaving the Five Finger Fillet table no longer crashes the game (error
  FFFFFFFF). The patch now edits the game's own bytecode in place instead
  of swapping in a copy the game later tried to free.

## [1.0.0] - 2026-09-17

### Added
- Any valid Five Finger Fillet button press counts as the correct one.
