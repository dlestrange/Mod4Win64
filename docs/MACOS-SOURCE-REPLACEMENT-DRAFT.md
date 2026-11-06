# Mod4Mac64 v0.9.0 Beta 1 — source and library replacement guide (DRAFT)

**Not yet a release-ready source offer.** This guide must be completed and independently reviewed against the *actual final ZIP* and the Qt/Homebrew source corresponding to its binaries. Do not copy it into the public compliance directory without resolving the checks below.

## Application source and modifications

Project: https://github.com/dlestrange/Mod4Win64
Mac beta branch: `feature/macos-native`
Release tag (planned): `mod4mac-v0.9.0-beta.1`

Record the exact release commit and make sure the source is accessible to every recipient. The application is licensed separately from its dependencies.

## Bundled libraries (to verify against final ZIP)

- Qt 6.12.0 frameworks/plugins (download corresponding LGPL source, including any patches/build configuration used by this binary distribution).
- libopenmpt 0.8.10 from Homebrew.
- mpg123 1.33.7 from Homebrew (LGPL-2.1-only per Homebrew package metadata).
- libogg 1.3.6 from Homebrew.
- libvorbis 1.3.7 from Homebrew.
- FFmpeg components should be absent in the revised distribution. Verify by enumerating **every** Mach-O binary within the final ZIP.

Source landing pages:
- https://download.qt.io/official_releases/qt/6.12/6.12.0/
- https://github.com/OpenMPT/openmpt
- https://www.mpg123.org/
- https://github.com/xiph/ogg
- https://github.com/xiph/vorbis

**Action required:** provide the full corresponding source and build information under an appropriate distribution method meeting all relevant licenses. Do not rely on the above upstream links alone as a complete offer.

## Replacing the dynamically linked libraries (draft)

The application dynamically links Qt frameworks and uses dynamic loading of `libopenmpt.dylib` from the app's `Contents/Frameworks` folder. The `libmpg123` dependency is dynamically linked from `libopenmpt`. The packaged libraries can be replaced with ABI-compatible, user-modified builds while preserving the expected framework and dylib layout, install names and architecture (`arm64`). After modification, a recipient may need to re-sign the app bundle with an ad-hoc signature, e.g.:

```sh
codesign --force --deep --sign - /path/to/Mod4Mac64.app
codesign --verify --deep --strict --verbose=2 /path/to/Mod4Mac64.app
```

This is not proof that all LGPL freedoms are met. Verify that the app is not technically restricting library replacement, and describe exact build and replacement instructions for the distributed binaries.

## Before release

- [ ] Replace this draft with a truthful, complete release guide; record exact versions, source archives/checksums, build configuration, and any modified library patches.
- [ ] Obtain Qt's relevant bundled third-party notices for the actual deployed frameworks/plugins.
- [ ] Include complete copyright-bearing BSD notices plus LGPLv3/GPLv3/LGPLv2.1 texts.
- [ ] Make corresponding source and installation/replacement information available as required.
- [ ] Ensure the release ZIP contains final compliance files and the release asset was generated from the correct commit.
