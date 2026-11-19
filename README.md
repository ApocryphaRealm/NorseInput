# NorseInput

One list of rebindable controls for the Norse overhaul (NORSE: Oath of Blood). Each module's named actions and the
controls bound to them - gamepad, keyboard, mouse, with tap / hold / double-tap gestures - live in plain files; two
actions on the same control are reported. Survey: `RESEARCH-more-frameworks.md` (#8); modelled on Satisfactory Mod
Loader's key bind registry, Dynamic Key Action Framework and Universal Hotkey Manager.

```ini
[NorseRealtimeNative]
LightAttack = Pad.RB, Mouse.Left
PowerAttack = Pad.RT, Key.F
LockOn      = Pad.R3, Key.Q
BowCancel   = Pad.X, Key.X
```

Author guide: `package\bindings\README.txt`; defaults: `package\bindings\_default.ini` (the gamepad buttons are
NorseRealtimeNative 0.5.x's own; keyboard and mouse are new).

## Parts

- `include/NorseInput.hpp` - header-only: the parser, conflict checks (in load order: the later binding loses), and a
  `Reader` per module that turns the devices into Pressed / Released / Hold / Double events each frame. Triggers have
  hysteresis; keyboard and mouse count only while the game is in front.
- No DLL: the data folder `ue4ss\Mods\NorseInput\bindings\` and its NorseManifest manifest. NorseRealtimeNative reads
  it at start (`nrn.bindings` re-reads), and keeps its own gamepad buttons only when no bindings are installed.
- `tools/input_test.cpp` - the offline test: `xmake build input_test && xmake run input_test <path to _default.ini>`
  (17 checks).

## Status

0.1.0 tested offline and in game (TEST-RESULT-0.1.0.txt: real F / left click / Q presses drove the power shot, the bow
draw and release, and the lock-on). Not yet: a rebinding page (NMMF), the moveset's held / double inputs (moves name
inputs "light" / "power" today), dodge / jump / camera controls (still read directly by NorseRealtimeNative).

## Licence

GPL-3.0-or-later (see LICENSE).
