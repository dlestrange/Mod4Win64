#!/usr/bin/env python3
"""Assemble original Qt attribution metadata and corresponding licence texts.

Includes originals from qtbase, qtmultimedia, and qtsvg, conservatively,
rather than trying to assert which implementation details were linked.
Not legal certification. Requires verified original Qt source extraction.
"""
from pathlib import Path
ROOT = Path(__file__).resolve().parent.parent
BASE = ROOT / "dist/compliance-macos"
ORIG = BASE / "Qt-6.12.0-Original-Licenses"
TARGET = BASE / "Qt-Third-Party-Notices.txt"
MODULES = ("qtbase", "qtmultimedia", "qtsvg")
parts = [
    "Mod4Mac64 — Qt 6.12.0 third-party attribution and licence texts",
    "=" * 65,
    "These are original attribution metadata and licence files extracted from",
    "the verified qt-everywhere-src-6.12.0 source archive. Qt modules:",
    ", ".join(MODULES) + ". Extra non-shipped components may be included.",
    "SPDX alternatives/conditions must be interpreted for the shipped binaries.",
    "See the companion full source archive and application source release.",
    "",
]
licenses = 0
attributions = 0
for module in MODULES:
    directory = ORIG / module
    if not directory.is_dir():
        raise SystemExit(f"Missing original Qt module directory: {directory}")
    files = sorted(directory.rglob("qt_attribution.json"))
    licence_files = sorted((directory / "LICENSES").glob("*.txt"))
    if not licence_files:
        raise SystemExit(f"Missing Qt licence files: {directory}/LICENSES")
    parts += ["", f"######## {module}: Qt upstream attribution records ########", ""]
    for f in files:
        data = f.read_text(encoding="utf-8")
        # Qt attribution files are source evidence, not guaranteed valid
        # strict JSON. Preserve text exactly instead of parsing/reformatting:
        # some upstream records contain literal control characters.
        parts += [f"----- BEGIN {f.relative_to(ORIG)} -----", data,
                  f"----- END {f.relative_to(ORIG)} -----", ""]
        attributions += 1
    parts += ["", f"######## {module}: original Qt licence texts ########", ""]
    for f in licence_files:
        parts += [f"----- BEGIN {f.relative_to(ORIG)} -----",
                  f.read_text(encoding="utf-8"),
                  f"----- END {f.relative_to(ORIG)} -----", ""]
        licenses += 1
if not attributions:
    raise SystemExit("No original Qt attributions found; cannot produce notices")
TARGET.write_text("\n".join(parts), encoding="utf-8")
print(f"Wrote original Qt attribution notices: {TARGET}")
print(f"Original attribution JSON files: {attributions}; licence texts: {licenses}")
print("Review actual binary inclusion and any additional file-level notices before publishing.")
