#!/usr/bin/env python3
"""Produce factual source/replacement instructions for this local Mac beta.

This file only describes archives verified by the local SHA256 manifest.
It cannot establish that published GitHub assets exist or legal compliance.
"""
import hashlib, subprocess
from pathlib import Path

ROOT=Path(__file__).resolve().parent.parent
ASSETS=ROOT/"dist/macos-source-assets"
OUT=ROOT/"dist/compliance-macos/SOURCE-AND-REPLACEMENT-INSTRUCTIONS.txt"
required={"qt-everywhere-src-6.12.0.tar.xz","mpg123-1.33.7.tar.bz2"}
sums=ASSETS/"SHA256SUMS.txt"
if not sums.is_file():
    raise SystemExit(f"Missing {sums}")
read={}
for line in sums.read_text().splitlines():
    try:
        digest,name=line.split(maxsplit=1)
    except ValueError:
        continue
    read[name.lstrip("*")]=digest
for name in required:
    p=ASSETS/name
    if not p.is_file() or name not in read:
        raise SystemExit(f"Missing source asset/checksum for {name}")
    h=hashlib.file_digest(p.open("rb"),"sha256").hexdigest()
    if h.lower()!=read[name].lower():
        raise SystemExit(f"Checksum mismatch: {name}")

sha=subprocess.check_output(["git","-C",str(ROOT),"rev-parse","HEAD"],text=True).strip()
text=f"""Mod4Mac64 Beta 1 — corresponding source and LGPL library replacement

Application source:
  https://github.com/dlestrange/Mod4Win64
  Build checkout commit recorded when this notice was prepared: {sha}
  Planned beta release tag: mod4mac-v0.9.0-beta.1

The release must make the COMPLETE exact source tree publicly accessible
to every recipient under the application's MIT license. The version
corresponding to the signed app must be identified in the release notes.

Companion ORIGINAL source assets (publish with the binary release):
  qt-everywhere-src-6.12.0.tar.xz
  SHA256: {read['qt-everywhere-src-6.12.0.tar.xz']}
  mpg123-1.33.7.tar.bz2
  SHA256: {read['mpg123-1.33.7.tar.bz2']}

The distributed application contains dynamically linked Qt 6.12.0
frameworks and plugins and libmpg123.0.dylib (LGPL-2.1).
Other dynamically linked third-party libraries include libopenmpt,
libogg, libvorbis and libvorbisfile; their licences and notices are
included separately.

Replacement of LGPL-covered libraries:
  1. Unzip the binary release archive to obtain Mod4Mac64.app.
  2. The Qt frameworks are in Mod4Mac64.app/Contents/Frameworks;
     related Qt plugins are in Mod4Mac64.app/Contents/PlugIns.
     libmpg123.0.dylib is in Contents/Frameworks.
  3. Rebuild the replacement library from its corresponding source
     for arm64 macOS, ensuring a compatible dynamic ABI and install
     names. Preserve the framework directory layout and dylib names,
     and replace the matching framework or dylib in the app bundle.
  4. After changing any bundled code, re-sign the changed app locally
     (ad-hoc signature) and verify, for example:
       codesign --force --deep --sign - /path/to/Mod4Mac64.app
       codesign --verify --deep --strict /path/to/Mod4Mac64.app
     Gatekeeper may still require the user's explicit approval to open
     an ad-hoc signed app downloaded from the internet.

Additional build details / legal requirements:
  - Qt binaries were deployed using Qt 6.12.0 for Apple Silicon from
    the developer's installed Qt distribution using macdeployqt.
  - The downloaded Qt source is the ORIGINAL Qt source archive, not
    necessarily an identical record of the Qt Company's prebuilt binary
    build configuration. The distributor must supply any required
    corresponding scripts/patches/configuration or relinking details.
  - Homebrew's mpg123 prebuilt bottle may carry packaging patches or
    configure choices; verify corresponding source and installation
    information for the exact dylib before public release.
  - Do not apply technical restrictions that prevent recipients from
    exercising the modification/replacement rights granted by LGPL.
  - Archive source must remain accessible to recipients in an
    LGPL-compliant manner. Upstream URLs alone are not a complete offer.
  - This document is generated locally; public asset links and exact
    source/build correspondence must be checked before release.
"""
OUT.parent.mkdir(parents=True,exist_ok=True)
OUT.write_text(text)
print(f"Created {OUT}")
print("Source SHA256 values checked. Public upload and replacement verification remain required.")
