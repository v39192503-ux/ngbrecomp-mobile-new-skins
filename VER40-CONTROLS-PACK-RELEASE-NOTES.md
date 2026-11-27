# Ninja Gaiden Black Android v1.0.3 — VER40 Controls Pack

## Updated source and features
- Replaced the branch's previous assets-only staging folder with the full sanitized source for the VER40 Controls Pack.
- Android touch control skins: VER40, Classic/Kenney preserved originals and Game-style fallback.
- Visible editor HUD and drawer: move/resize controls, adjust size and opacity, hide/show, reset confirmation, Save/Cancel, grid/snap/spacing, skin and layout.
- Custom positions, sizes and visibility now render through the Android touch overlay; selected-control outline and resize handle are included.
- Profiles GAME/MENU are manually switched in the editor (automatic menu detection not yet wired).

## Build / validation
- Android ARM64 native target compiled and APK packaged.
- Package: `io.github.gamboozino.ngbrecomp` (versionCode 1004; versionName `1.0.3 Controls Pack`)
- APK signature v3 verification passed.
- 16 KB ZIP alignment verification passed.
- Manifest, target SDK 35, min SDK 29, arm64 ABI confirmed.
- SHA-256: `65205bedfe35530bacd5b879feea4b9484bb2eac43bdda9746dfa0eed688af8c`
- Runtime install/visual/input test remains pending; no Android device was connected via ADB.

Game files and generated recompilation source are not included. Use your own legally obtained retail game data.