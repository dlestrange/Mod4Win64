# Mod4Win64 Audio Prototype (v0.3)

This branch adds initial actual module playback using libopenmpt and Qt Multimedia.

## Requirements
- Qt 6 Widgets **and Multimedia** (MSVC 64-bit kit), Visual Studio C++ and CMake.
- Official 64-bit Windows **libopenmpt.dll**, from https://lib.openmpt.org/libopenmpt/download/ — use Windows amd64 build. Copy its DLL and any required runtime dependencies into the same directory as the built Mod4Win64.exe, or make them discoverable on the process DLL search path.
- Deploy Qt runtime DLLs/plugins with `windeployqt --debug` after building.

## Try it
1. Fetch/checkout `feature/audio-playback-v03`.
2. Configure and Build All in Visual Studio.
3. Run Mod4Win64; click **Open** to browse folders.
4. Select a module in the left column and click **Add >>**.
5. Double-click its entry in the **Playlist** to load and play.
6. Use **Play**, **Pause**, **Stop** in the main player. Play resumes a paused song; after Stop, selecting a module loads it again.

## Known limitations
- Experimental and **not yet tested on Windows** by the assistant. Please report compiler and runtime output.
- No real pattern counters, effects, playlist auto-advance, seek or instrument information.
- The existing home-directory-limited file tree is unchanged in this patch; drive navigation will be fixed separately.
- Loaded module starts automatically on playlist double-click; transport remains basic.
- The libopenmpt API is resolved dynamically from its official x64 DLL. No libopenmpt link/import library is required for this first pass.

Source credit: Mod4Win64 is a preservation-oriented project inspired by the original Mod4Win and its developers.
