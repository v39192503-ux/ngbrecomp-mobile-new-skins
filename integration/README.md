# VER40 Controls Pack integration snapshot

This folder contains the isolated Android 1.0.3 source files used for the three-style skin integration build. It is a reference snapshot, not a direct drop-in patch against the branch root: the root is a separate upstream-based source branch.

## Included
- `src/touch_pad.c`: fallback rendering with three skin choices (VER40 Pack, Classic/Kenney, generated/game-style), held-state sprites and skin cache invalidation.
- `src/touch_editor.inc.h`: skin cycle and persistence changes plus the current editor WIP.
- `packaging/android/build.sh`: Android packaging script with D8 class-file argument handling that is safe for paths containing spaces.

## Skin modes
- 0: VER40 pack artwork.
- 1: preserved original Kenney classic assets (`touch_pad`, `touch_nub`, `touch_dpad`, and directional pressed artwork).
- 2: generated/game-style fallback.

The game-provided artwork path remains preferred when the game's textures are available. Game files are not included.

## Validation
Native `libmain.so` compilation succeeded. APK packaging completed, signature v3 verification and 16-KB zip alignment passed, and package/version/ABI were checked. Runtime rendering/input remains untested because no Android device was attached to ADB. The touch editor's full visible UI renderer/profile-layout behavior is still incomplete; this snapshot should not be described as a fully finished editor.
