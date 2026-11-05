# Mod4Win64

**Windows v0.9 beta** — a modern Qt 6 revival of the classic Mod4Win tracker module player, powered by libopenmpt.

The Windows player is feature-complete for this beta milestone. It includes tracker playback, a classic dot-matrix player and transport, a synchronised Effects panel, instrument/sample details, a persistent Jukebox playlist, and configurable audio playback options.

## Windows build and portable release

See [Windows release instructions](docs/RELEASING-WINDOWS.md) for the x64 Release CMake build, embedded icon, portable ZIP packager, Qt deployment, required libopenmpt companion DLLs, clean-machine testing and third-party licensing.

The portable ZIP is built locally under the ignored `dist/` directory. **A public binary release has not yet been published from this repository.**

## Development status

- **Windows:** v0.9 beta; tested on development and a second Windows machine. Further testing and polish are pending (including 150% Effects panel scaling).
- **macOS:** native Apple Silicon build is the next development phase; Qt and decoder loading/deployment need platform-specific work.
- **v1.0:** planned after cross-platform parity, regression testing, packaging and licensing checks.

## Credits

Mod4Win64 is a revival inspired by the original Mod4Win, whose developers are credited in the application as **Kay Bruns, Uwe Zaenker and Jens Puchert**. Playback decoding uses **libopenmpt**; the desktop interface uses **Qt 6**.

This project is not represented as an official continuation by the original Mod4Win developers. See the repository licence and any redistributed third-party component notices for applicable terms.
