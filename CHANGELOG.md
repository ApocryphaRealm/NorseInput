# Changelog

## 0.1.0 - 2026-11-17 (tested offline, 17 checks, and in game: TEST-RESULT-0.1.0.txt)
### Added
- Bindings in bindings/*.ini: [Module] then Action = controls (Pad, Key, Mouse) with :hold and :double gestures; conflicts reported, the later binding loses.
- include/NorseInput.hpp: parser, checks, Reader (Pressed / Released / Hold / Double events); _default.ini for NorseRealtimeNative.
