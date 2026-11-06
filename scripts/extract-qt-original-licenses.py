#!/usr/bin/env python3
"""Extract original Qt 6.12.0 licence texts and attribution JSON into a review archive.

This script deliberately does not claim the Qt notices are complete or mark
a public release as licence-cleared. Original source materials are preserved.
"""
import os
import tarfile
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DIST = ROOT / "dist"
ARCHIVE = DIST / "macos-source-assets/qt-everywhere-src-6.12.0.tar.xz"
OUT = DIST / "compliance-macos/Qt-6.12.0-Original-Licenses"
MODULES = ("qtbase", "qtmultimedia", "qtsvg")
PREFIX = "qt-everywhere-src-6.12.0/"

if not ARCHIVE.is_file():
    raise SystemExit(f"Missing Qt source: {ARCHIVE}")
OUT.mkdir(parents=True, exist_ok=True)

files = 0
totals = {}
with tarfile.open(ARCHIVE, mode="r:xz") as tar:
    for member in tar:
        if not member.isfile() or not member.name.startswith(PREFIX):
            continue
        relative = Path(member.name[len(PREFIX):])
        if not relative.parts:
            continue
        module = relative.parts[0]
        if module not in MODULES:
            continue
        selected = (len(relative.parts) >= 3 and relative.parts[1] == "LICENSES"
                    or relative.name == "qt_attribution.json")
        if not selected:
            continue
        # Avoid archive path traversal: only use elements of the expected tree.
        if any(part in ("..", ".") for part in relative.parts):
            raise SystemExit(f"Unsafe archive path: {member.name}")
        target = OUT / relative
        target.parent.mkdir(parents=True, exist_ok=True)
        src = tar.extractfile(member)
        if src is None:
            continue
        with src, target.open("wb") as dst:
            while chunk := src.read(1024 * 1024):
                dst.write(chunk)
        files += 1
        totals[module] = totals.get(module, 0) + 1

if not files:
    raise SystemExit("No matching licences found; check archive layout")
print(f"Extracted {files} ORIGINAL files to: {OUT}")
for module in MODULES:
    print(f"  {module}: {totals.get(module, 0)} files")
print("These files are source evidence, not a completed Qt notice or LGPL clearance.")
