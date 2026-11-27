# VER40 Controls Pack — 1.0.3 source update

## Updated
- Replaced the branch's prior assets-only integration snapshot with the full sanitized source tree from the isolated Android 1.0.3 work copy.
- Integrated the visible editor HUD, settings drawer, selected-control outline and resize handle, grid overlay, and profile/layout preview rendering.
- Custom overrides now participate in touch-control drawing, including visibility and layout/profile placement; rendering cache invalidates on changes.
- Three fallback skin modes are wired in the stand-in renderer, including original classic assets.
- Added documentation for the editor and its controls.

## Validation
- Native Android ARM64 C/C++ target built successfully after editor rendering integration.
- APK packaging completed successfully; signature v3 and 16-KB zip alignment were verified for the candidate.
- Runtime install/interaction tests remain pending (no ADB device attached).
- Automatic game-menu profile switching remains pending; profile selection is manual.

## Distribution hygiene
The published branch intentionally omits XISO/XBE/game files, generated recompilation source, build output, local Python environments, and personal continuation notes.
