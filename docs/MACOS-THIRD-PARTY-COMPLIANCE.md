# Mod4Mac64 beta — third-party distribution audit

**Status: OPEN / NOT CLEARED FOR PUBLIC RELEASE.** This is an inventory and verification plan, not a substitute for license texts, corresponding source, or a legal review.

## Identified from the local v0.9.0-beta.1 Apple Silicon package

- **Qt 6.12.0** (reported by qmake): Qt frameworks (not enumerated by the earlier `find -name '*.dylib'` command) and plugins:
  - `platforms/libqcocoa.dylib`, `styles/libqmacstyle.dylib`
  - `tls/libqsecuretransportbackend.dylib`, `tls/libqopensslbackend.dylib`, `tls/libqcertonlybackend.dylib`
  - `iconengines/libqsvgicon.dylib`
  - `multimedia/libffmpegmediaplugin.dylib`, `multimedia/libdarwinmediaplugin.dylib`
  - `imageformats/libqgif.dylib`, `imageformats/libqico.dylib`, `imageformats/libqjpeg.dylib`
  - `networkinformation/libqapplenetworkinformation.dylib`
- **libopenmpt**: `libopenmpt.dylib`; Homebrew build previously reported as 0.8.10.
- **Audio dependencies**: `libmpg123.0.dylib`, `libogg.0.8.6.dylib`, `libvorbis.0.dylib`, `libvorbisfile.3.dylib`.
- **FFmpeg**: `libswresample.7.dylib`, `libswscale.10.dylib`, `libavformat.63.dylib`, `libavutil.61.dylib`, `libavcodec.63.dylib`.

The above inventory comes from a local file-name listing. It does **not** verify full Qt framework contents, actual binary license configurations, architecture, or versions of every embedded library.

## Licensing research references

- Qt 6.12 licensing: https://doc.qt.io/qt-6.12/licensing.html
- Qt LGPL obligations: https://www.qt.io/development/open-source-lgpl-obligations
- Qt open-source FAQ: https://www.qt.io/faq/qt-open-source-licensing
- FFmpeg legal requirements: https://ffmpeg.org/legal.html

Do not assume FFmpeg is LGPL without checking the exact build configuration and any GPL-enabled components. Do not treat links alone as satisfying any applicable requirements to supply license texts and corresponding source.

## Before publishing

- [ ] Inventory every regular file and symlink within `Contents/Frameworks`, `Contents/PlugIns`, and `Contents/Resources`, including `.framework` binaries; record versions and architectures.
- [ ] Determine licensing and build configuration of the bundled FFmpeg libraries (and whether unused FFmpeg media plugin and libraries can be safely excluded **with retesting**).
- [ ] Confirm license and copyright notices for exact builds of Qt, libopenmpt, mpg123, Ogg, Vorbis, FFmpeg if retained, and other nested third-party code as applicable.
- [ ] Include full required license texts and notices **inside the distributed ZIP** and make matching source/build information available as required by each applicable license, especially LGPL covered components.
- [ ] Verify recipients can modify/replace LGPL components as required, including any installation/relinking instructions.
- [ ] Rebuild package after compliance materials are included, then check ZIP checksum and test the **new exact ZIP** on the MacBook Neo.
- [ ] Check app and library codesign validation, install names, and architecture.
- [ ] Publish a Mac-specific prerelease tag and asset only after all boxes above are cleared; repository visibility must allow intended testers to download.

**Note:** The app's own top-level MIT LICENSE is not sufficient for bundled third-party components.
