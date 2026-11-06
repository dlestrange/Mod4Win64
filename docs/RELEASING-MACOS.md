# Mod4Mac64 v0.9.0 Beta 1 — release preparation

**Status:** candidate for a limited Apple Silicon beta, not yet approved for
public distribution. The repository is private at the time of preparation.

## Proposed GitHub release

**Title:** Mod4Mac64 v0.9.0 Beta 1 — Apple Silicon

**Tag:** `mod4mac-v0.9.0-beta.1`

**Target:** `feature/macos-native` until the Mac port is reviewed and merged.

**Description:**

> The first native Apple Silicon beta of **Mod4Mac64**, a macOS companion
> to Mod4Win64 and a revival of the classic Mod4Win tracker-module player.
>
> Built with Qt 6 and libopenmpt. Features a retro green-dot-matrix player,
> MOD/XM/S3M/IT playback through OpenMPT, jukebox playlists, instrument
> information, live Effects panel, transport controls, and audio configuration.
>
> Tested on an Apple Silicon MacBook Pro and MacBook Neo. **Beta software:**
> further compatibility testing is welcome.
>
> **Apple Silicon only (arm64)** in this release. Not tested for Intel Macs.
>
> **Signing:** This testing build is ad-hoc signed, **not notarised**. macOS
> Gatekeeper may block an Internet-downloaded app. Only open software you
> trust; do not disable Gatekeeper system-wide. A Developer ID signed,
> notarised release is planned if public distribution proceeds.
>
> This distribution includes third-party open source components. See the
> accompanying licence and source-availability documents.

## Developer packaging

Install the Qt macOS arm64 kit, build the app, and install:

    brew install cmake ninja libopenmpt dylibbundler

Configure and build:

    cmake -S . -B out/build/macos-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/macos" -DCMAKE_OSX_ARCHITECTURES=arm64
    cmake --build out/build/macos-arm64

Package:

    chmod +x scripts/package-macos.sh
    ./scripts/package-macos.sh

Output: `dist/Mod4Mac64-v0.9.0-beta.1-macos-arm64.zip`.

The script checks for some unresolved development-machine library references
and performs ad-hoc signing. It cannot attest complete portability, ensure
the complete third-party source distribution, or notarise the app.

## Required public-release checks

- [ ] Confirm release build and the bundle are arm64.
- [ ] Test ZIP downloaded/expanded on the Neo with no development tools.
- [ ] Test Effects scales 100%, 150%, 200%, 300%, playlists and audio.
- [ ] Ensure all Qt libraries/plugins, OpenMPT and transitive dylibs are
      included and resolve within the bundle or to Apple system frameworks.
- [ ] Include Qt, FFmpeg (if bundled), libopenmpt and mpg123 (if bundled)
      licence notices and verify actual bundled library versions/terms.
- [ ] Provide the correct corresponding source code and build information
      required for the actual LGPL-covered binaries; the earlier Windows
      licensing supplement is NOT by itself a substitute.
- [ ] Decide public release signing: Developer ID + notarisation is the
      preferred standard experience. If unsigned/ad-hoc, identify clearly
      as a limited test beta with extra macOS approval steps.
- [ ] Confirm the GitHub repository and release are accessible to intended
      testers (private repo releases are not publicly downloadable).
- [ ] Create the Mac-specific release tag from the reviewed Mac commit;
      do not repoint the Windows beta tag.
- [ ] Upload Mac ZIP and compliance material, with checksums; retain Windows
      release assets unchanged.
