# Mod4Mac64 Beta 1 — compliance build handoff

This is a reproducible **pre-release preparation workflow**, not legal advice or an automated certification.

## Working tree (MacBook Pro)
```sh
cd ~/Developer/Mod4Win64
git switch feature/macos-native
git pull
bash scripts/prepare-macos-compliance.sh
python3 scripts/assemble-qt-notices.py
bash scripts/prepare-macos-source-assets.sh
```

The two latter commands respectively (1) create `dist/compliance-macos/Qt-Third-Party-Notices.REVIEW.txt` using the installed Qt SPDX JSON, and (2) download checksum-verified original Qt 6.12.0 source (~423 MB) and mpg123 1.33.7 source (~1.1 MB) into `dist/macos-source-assets/`, writing a SHA256SUMS.txt manifest.

**Do not simply rename the REVIEW notice to `Qt-Third-Party-Notices.txt`.** First compare copyright and full licence conditions against the authoritative Qt source `qt_attribution.json` metadata and `LICENSES` contents, and verify the included components against the final app ZIP. The SBOM contains optional components not necessarily compiled into the shipped app.

## LGPL source and replacement requirements

Use `docs/MACOS-SOURCE-REPLACEMENT-DRAFT.md` as a starting point. Before a public release:

1. Provide the exact source for the linked Qt LGPLv3 libraries and mpg123 LGPLv2.1, including any distributor-applied patches, and a feasible method to rebuild or replace them. Original source archives may need corresponding patch/build configuration supplements. Qt source archive includes many modules beyond those actually deployed; extra source is not a problem.
2. Document how a recipient can replace the dynamically linked Qt frameworks and `libmpg123.0.dylib` in the app bundle, keeping architecture/install names compatible, and re-sign the modified app with ad-hoc signing.
3. Include complete licence texts and all relevant third-party copyright conditions. The generated REVIEW notices and the SBOM file are evidence but not substitutes for these originals.
4. Verify the final application bundle has no packaged FFmpeg backend/libraries. Perform a fresh audit of frameworks/plugins and machine-specific binary paths.
5. Ensure the full actual Mod4Mac64 source corresponds to the published Mac release tag. Keep the tag pointing at the source used to build the ZIP.
6. Make all source distribution assets available to recipients under appropriate LGPL-compliant terms (e.g. attach to the same public GitHub release), not just an upstream link. Verify the download links **after** publication and note which source archive is which.
7. Have a qualified licence reviewer check the third-party materials if legal certainty is required.

## Planned release assets

- `Mod4Mac64-v0.9.0-beta.1-macos-arm64.zip` — regenerate only when notice/source work is approved
- `qt-everywhere-src-6.12.0.tar.xz` — official upstream source, checksum checked by script
- `mpg123-1.33.7.tar.bz2` — official upstream source, checksum checked by script
- `SHA256SUMS.txt` — checksums for those sources; include binary ZIP hash too in final release notes
- Application source from the tagged `dlestrange/Mod4Win64` repo; consider explicit source archive asset
- Extra patches and build instructions (if required), and original licence notices

**Release gate stays closed** while `Qt-Third-Party-Notices.txt` and `SOURCE-AND-REPLACEMENT-INSTRUCTIONS.txt` are missing. Do not populate either with placeholders just to pass the file-existence test.

## Source links verified during preparation
- https://www.qt.io/development/offline-installers
- https://www.qt.io/development/open-source-lgpl-obligations
- https://doc.qt.io/qt-6/licenses-used-in-qt.html
- https://sourceforge.net/projects/mpg123/files/mpg123/

