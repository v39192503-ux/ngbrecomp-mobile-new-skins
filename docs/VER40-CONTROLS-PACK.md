# VER40 Controls Pack — Android touch controls

This branch replaces the previous assets-only staging update with the current integrated source tree and the 1.0.3 Controls Pack Android build.

## Features implemented in the source
- Custom VER40 artwork for touch buttons, analog sticks, D-pad, shoulders, triggers and system controls.
- Preserved original Kenney-style/classic fallback sprites alongside the VER40 and generated/game-art fallback skin modes.
- On-screen Controls editor HUD: profile toggle, size and opacity adjustments, hide/show, reset (confirmation when resetting all), Save and Cancel.
- Select a touch control by tapping it; drag it to reposition and use its resize handle/pinch to resize.
- Settings drawer: grid toggle, snapping, grid spacing, skin cycling and layout choice.
- Editor preview switches to the profile/layout being edited, and the custom positions/visibility/size are rendered into the active controls.
- Custom layout and editor settings are saved in `touch-layout.ini`.

## Controls
- Gear icon opens the editor.
- Drag selected control to move; drag the corner handle or pinch to resize.
- Main panel buttons control profile, size, opacity, hide, reset, cancel/save and collapse.
- Hold the panel's center tab to open the settings drawer; tap to collapse/expand.
- Drawer controls grid, snap, grid size, skin and layout.

## Limitations / test status
- The MENU and GAME profiles can be selected manually in the editor. Automatic switching based on the game's menu state is not yet connected.
- The editor overlay was compiled into the Android candidate APK. No device runtime test has been performed in this publishing session because an Android device is not connected through ADB.
- Generated recompilation sources and game files are intentionally excluded. Build from your own legally obtained retail XISO/XBE by following the project's generation pipeline.
- This repository does not include game ROM/XISO/XBE, game textures, or extracted/generated game code.
