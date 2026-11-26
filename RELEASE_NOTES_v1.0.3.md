# Ninja Gaiden Black Android — v1.0.3 (draft)

## Build
- Android ARM64 APK compiled with the Android NDK toolchain.
- Package ID: `io.github.gamboozino.ngbrecomp`.
- Minimum Android API: 29 (Android 10).
- Includes the native game runtime and Adreno Turnip Vulkan driver.
- APK contains no copyrighted game data; users must provide their own game files.

## Validation performed
- APK signature verification (debug signing).
- ZIP alignment validation, including 16 KB alignment checks.
- Manifest/package metadata inspection.
- ARM64 ELF checks for bundled native libraries.
- SHA-256 checksum generated.

## Known limitations / testing status
- No physical Android device was connected for runtime testing during this build.
- Optional Trilogy/Tomoe modules were not generated and are not included.
- This is a development-signed APK, not a production signing key.
- Release publication is pending GitHub authentication and upload.

## APK checksum (SHA-256)
`8eada732577d7969e07b40eece64d1281cf7a9d83366c6b390073941c96e8938`
