# Mod4Mac64 — first native Apple Silicon build

The Mac application is named **Mod4Mac64**. It shares the Qt/C++ source code with
Mod4Win64 for Windows; do not create a separate repository or duplicate audio logic.

## Prerequisites

- macOS on Apple Silicon; Xcode and its command line tools installed
- Homebrew (optional but convenient for developer dependencies)
- Qt 6.12 (macOS, arm64/universal installation) with Widgets and Multimedia
- CMake and Ninja
- libopenmpt built for Apple Silicon, with compatible dependencies

Example Homebrew installation (after Homebrew is available):

    brew install cmake ninja libopenmpt

Qt can be installed through the official Qt Online Installer. The
Qt macOS installation path is generally `~/Qt/6.12.0/macos`.
Avoid mixing Intel/x86_64 Qt libraries with an arm64 build.

## Configure and build

From Terminal at the repository checkout:

    git fetch origin
    git switch feature/macos-native
    cmake -S . -B out/build/macos-arm64 -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH="$HOME/Qt/6.12.0/macos" -DCMAKE_OSX_ARCHITECTURES=arm64
    cmake --build out/build/macos-arm64

Launch the resulting app bundle:

    open out/build/macos-arm64/Mod4Mac64.app

Or launch it directly to see Qt library loader diagnostics:

    ./out/build/macos-arm64/Mod4Mac64.app/Contents/MacOS/Mod4Mac64

## Tracker decoder

Qt and the basic player UI can start without libopenmpt. Opening a tracker
module requires `libopenmpt.dylib` built for arm64. The decoder searches:

- Mod4Mac64.app/Contents/Frameworks/libopenmpt.dylib (future packaged app)
- Next to the bundle executable
- /opt/homebrew/lib/libopenmpt.dylib (typical Apple Silicon Homebrew)
- /usr/local/lib/libopenmpt.dylib
- Qt/system loader search paths

This is a developer build: no macdeployqt packaging, relocation, signing or
notarisation is configured yet. A production .app will need a bundled decoder
and dependency closure, an icon, macdeployqt validation, and a separate
redistribution/licence check.

## First validation

1. Check window appearance and the Mac-specific MOD4MAC 64 logo.
2. Open the Jukebox and load a MOD or XM module.
3. Verify actual audio output and transport controls.
4. Open the Effects panel and inspect layout scaling on the Retina display.
5. Test persistent playlists and Setup settings, and check any shortcuts.

Note: The Windows 0.9 code remains unchanged on the main branch.
