# VER40 Controls Pack — Android touch controls

This branch contains the integrated Controls Pack source plus the unbuilt v0.9.66 behavior/visual corrections described below. The published v1.0.3 APK is a separate, unchanged baseline and does not contain these branch edits.

## Features implemented in the source
- Custom VER40 artwork for touch buttons, analog sticks, D-pad, shoulders, triggers and system controls.
- Three selectable skins: VER40 custom artwork, Classic/Kenney, and Game-style artwork. The selected skin is rendered during gameplay and is not limited to the game-art fallback path.
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
- The MENU and GAME profiles switch automatically with the game's boot, menu, loading-card, movie and 2D-menu signals; either profile can also be chosen directly in the editor.
- The changes described here have not been compiled into a new APK: this environment has no CMake, Android SDK/NDK, SDL2 development package or connected ADB device. The original v1.0.3 APK remains separate and unchanged; runtime touch/visual tests are pending.
- Generated recompilation sources and game files are intentionally excluded. Build from your own legally obtained retail XISO/XBE by following the project's generation pipeline.
- This repository does not include game ROM/XISO/XBE, game textures, or extracted/generated game code.
