# Mac beta — scoped Qt 6.12.0 attribution review

**Working review, NOT licence clearance.** Based on an extracted `PackageName`, `PackageLicenseDeclared`, `PackageCopyrightText` list for the installed Qt 6.12.0 `qtbase`, `qtmultimedia`, `qtsvg` SPDX SBOMs, and the previously reported app-bundle inventory. For exact notices, consult full SPDX records and Qt upstream attribution texts, not just the three extracted fields.

## Confirmed distributed Qt framework directories

`QtCore.framework`, `QtGui.framework`, `QtWidgets.framework`, `QtNetwork.framework`, `QtMultimedia.framework`, `QtSvg.framework`, `QtConcurrent.framework`, `QtDBus.framework`.

## Confirmed deployed plugins (previous bundle inventory)

- Platform: `libqcocoa.dylib`, `libqmacstyle.dylib`
- TLS: `libqsecuretransportbackend.dylib`, `libqopensslbackend.dylib`, `libqcertonlybackend.dylib`
- Multimedia: `libdarwinmediaplugin.dylib` (FFmpeg plugin explicitly removed by beta packaging script)
- Image formats: `libqgif.dylib`, `libqico.dylib`, `libqjpeg.dylib`
- SVG icon: `libqsvgicon.dylib`
- Network information: `libqapplenetworkinformation.dylib`

The above is **not** a binary-level proof that optional or bundled third-party code is absent. Re-inventory the final ZIP and check the exact Qt library binary contents.

## Qt upstream third-party entries worth reviewing first

Relevant-to-shipped-framework candidates taken from the uploaded SBOM extract:
- **QtCore**: `Core_Attribution_unicode-character-database`, `unicode-cldr`, `tlexpected`, `siphash`, `blake2`, `md4`, `md5`, `sha1`, `sha3_endian`, `sha3_keccak`, `rfc6234`, `tinycbor`, `qeventdispatcher_cf`, `doubleconversion`, `easing`, `forkfd`, `tika-mimetypes`; plus PCRE2-related packages if actually bundled.
- **QtGui / Qt image plugins**: `BundledLibpng`, `BundledLibjpeg`, `BundledFreetype`, `BundledHarfbuzz`, `Gui_Attribution_grayraster`, `smooth-scaling-algorithm`, `aglfn`, `xsvg`, `emoji-segmenter`, `webgradients`, `icc-srgb-color-profile`, `md4c`, and other platform-specific entries only if actually present.
- **QtNetwork**: `Network_Attribution_psl-data` and `libpsl`.
- **QtDBus**: `DBus_Attribution_libdbus-1-headers`.
- **Cocoa plugin**: `QCocoaIntegrationPlugin_Attribution_cocoa-platform-plugin`.
- **QtMultimedia**: `Multimedia_Attribution_dr_libs`; `BundledTLSF` or other multimedia components only if incorporated into the shipped framework/plugin.
- **QtSvg**: Qt LGPL module / SVG icon plugin. The extract does not expose an additional SVG-specific attribution entry.

The extract also contains **build-only or unshipped components** such as `moc`, `rcc`, `macdeployqt`, `qmake`, test modules, Android deployment tools, SQL plugins, spatial-audio/Quick modules, and the excluded Qt FFmpeg media plugin. Their presence in an installation-wide SBOM does not itself mean their notices are required in this app's ZIP.

## Critical outstanding checks

1. Obtain the exact full Qt 6.12.0 attribution records and copyright-bearing licence terms for the components incorporated into the shipped frameworks/plugins; `NOASSERTION` in a package field is not a licence classification.
2. Confirm whether installed Qt binaries use embedded versus system libraries; avoid treating every SBOM candidate as definitively shipped.
3. Assemble `Qt-Third-Party-Notices.txt` from verified originals, retaining copyright texts and licence conditions.
4. Confirm all complete licence texts (Qt LGPL-3.0 and relevant additional Qt terms, mpg123 LGPL-2.1, BSD notices) are embedded.
5. Make actual corresponding source, configuration/build details, and library replacement guidance available in the manner required by applicable licences; links to unrelated or newer source versions aren't enough.
6. Rebuild ZIP after all notices are final, test that exact ZIP and compare release-asset checksum.

**Source inventory:** user-provided `qt-license-inventory.txt`, 546 lines, 182 `PackageName` entries, including 59 names containing `_Attribution_`. This extract is not an independently checked full binary SBOM.
