# Mod4Win64 — Windows x64 portable release

Mod4Win64 uses Qt6 Widgets/Multimedia and dynamically loads libopenmpt.dll.
The standalone EXE needs Qt libraries, plugins and the tracker decoder.

## 1. Configure Release

Use a 64-bit MSVC + matching Qt installation. The usual Visual Studio
CMake x64-Release profile is fine. For a matching developer console:

    cmake -S . -B out/build/x64-Release -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_PREFIX_PATH=C:/Qt/6.12.0/msvc2022_64

CMake now generates and embeds a multi-resolution Windows application icon.
No extra image or graphics library is required to build the icon.

## 2. Build and bundle

Use the 64-bit libopenmpt.dll that already works for your player. Run
PowerShell in the project directory:

    .\scripts\package-windows.ps1 -BuildDir out\build\x64-Release -LibOpenMptDll "C:\path\to\libopenmpt.dll" -QtBin "C:\Qt\6.12.0\msvc2022_64\bin"

Expected output:

- dist\Mod4Win64\Mod4Win64.exe
- dist\Mod4Win64\libopenmpt.dll and its required openmpt-mpg123.dll, openmpt-vorbis.dll, openmpt-zlib.dll companions
- dist\Mod4Win64\Qt6*.dll and platforms\qwindows.dll from windeployqt
- dist\Mod4Win64-Windows-x64-portable.zip

The script automatically copies openmpt-mpg123.dll, openmpt-vorbis.dll and openmpt-zlib.dll from beside libopenmpt.dll, failing early if any are missing. Use a matching x64 distribution. ExtraDlls may still be used for other decoder/runtime dependencies. Check transitive DLL dependencies (e.g. via dumpbin /dependents) and include applicable notices and licenses.

## 3. Test on a clean machine

Extract the ZIP in a DIFFERENT folder, ideally on a Windows 11 machine without
Visual Studio, Qt or libopenmpt installed. Verify:

1. App starts without DLL/plugin errors; titlebar and Explorer icon appear.
2. MOD/XM/S3M files load and play with audible sound.
3. Effects/Instrument panels display correctly and follow actual playback.
4. PREAMP, Mono/Stereo/Surround and Setup options work.
5. Playlist survives closing and reopening (including changed order).
6. Filelist and playlist double-click behaviours, and Remove All, still work.

Qt's windeployqt deploys Qt libraries and plugins. It may not find every
non-Qt dependency of your own libopenmpt.dll; check that separately.

## 4. Before publishing

The project's LICENSE is NOT sufficient for third-party runtime DLLs.
Include the applicable Qt, libopenmpt, compiler runtime and other component
notices and licences, and comply with the distribution terms (including
LGPL requirements for the Qt version you bundle). The 2026 Beta title and
About text should be updated before a public v1.0.

## Later: installer and macOS

Once the portable ZIP passes testing, an Inno Setup installer could add a
Start Menu shortcut, uninstaller, and *optional* tracker file associations.
Do not claim .mod/.xm/.s3m/.it associations without the user's consent.

Native macOS packaging will be a separate macdeployqt / .app / .dmg workflow
with a compatible libopenmpt dylib.
