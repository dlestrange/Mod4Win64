#!/usr/bin/env python3
"""Assemble Qt notice *review material* from installed Qt 6.12.0 SPDX JSON.

Do not treat this output as licence clearance: verify every entry against
the corresponding Qt source and deployed components.
"""
from __future__ import annotations
import json, os, re, textwrap
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
DEST = ROOT / "dist/compliance-macos"
QT = Path(os.environ.get("QT_DIR", str(Path.home() / "Qt/6.12.0/macos")))
MODULES = ("qtbase", "qtmultimedia", "qtsvg")
SKIP_PREFIXES = ("Test_", "TestInternals", "FFmpeg", "BundledResonanceAudio",
                 "Platform_Attribution_", "Bootstrap_")
SKIP_NAMES = {"BundledLibjpeg16bits", "BundledLibjpeg12bits",
              "FreetypePrivate_Attribution_freetype-zlib",
              "FreetypePrivate_Attribution_freetype-bdf",
              "FreetypePrivate_Attribution_freetype-pcf"}

def is_candidate(name: str) -> bool:
    if name.startswith(SKIP_PREFIXES) or name in SKIP_NAMES:
        return False
    return ("_Attribution_" in name or name.startswith("Bundled"))

def render():
    lines = [
        "Mod4Mac64 — Qt 6.12.0 third-party notices (REVIEW COPY)",
        "======================================================",
        "",
        "Generated from official Qt 6.12.0 installed SPDX SBOMs.",
        "Copyright statements and SPDX concluded licences reproduced",
        "from these SBOMs. This document does NOT include the complete",
        "text for every listed licence and is NOT release ready.",
        "Component presence must be checked against the final app bundle.",
        "The Qt 6.12.0 source archives and their LICENSES directories",
        "must be supplied/reviewed separately.",
        "",
    ]
    included = 0
    all_licences = set()
    for module in MODULES:
        path = QT / "sbom" / f"{module}-6.12.0.spdx.json"
        if not path.is_file():
            raise SystemExit(f"Missing source SBOM: {path}")
        doc = json.loads(path.read_text(encoding="utf-8"))
        lines += ["", f"### Qt module: {module}", ""]
        for pkg in doc.get("packages", []):
            name = pkg.get("name", "")
            if not is_candidate(name):
                continue
            license_expr = pkg.get("licenseConcluded") or "NOASSERTION"
            c = pkg.get("copyrightText") or "NOASSERTION"
            source = pkg.get("downloadLocation") or "NOASSERTION"
            included += 1
            all_licences.add(license_expr)
            lines += [f"Component: {name}", f"License: {license_expr}",
                      f"Source: {source}", "Copyright:", str(c), "",
                      "--------------------------------------------------", ""]
    lines += ["", "Licences referenced — full original terms still required:", ""]
    lines += [f"- {x}" for x in sorted(all_licences)]
    target = DEST / "Qt-Third-Party-Notices.REVIEW.txt"
    DEST.mkdir(parents=True, exist_ok=True)
    target.write_text("\n".join(lines) + "\n", encoding="utf-8")
    print(f"Created {target}")
    print(f"Review entries: {included}. Not a legally complete notice file.")
    print("Next: verify the source and applicable licence texts before release.")

if __name__ == "__main__":
    render()
