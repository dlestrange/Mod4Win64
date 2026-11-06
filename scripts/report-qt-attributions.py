#!/usr/bin/env python3
"""Create a scoped, auditable report from the Qt 6.12.0 SPDX JSON SBOMs.

This is a candidate list, not proof of inclusion in the app or legal clearance.
Only uses local Qt files; requires Python 3 standard library.
"""
import json
import os
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
OUT = ROOT / "dist" / "compliance-macos"
QT = Path(os.environ.get("QT_DIR", str(Path.home() / "Qt/6.12.0/macos")))
MODULES = ("qtbase", "qtmultimedia", "qtsvg")
# Prefixes for shipped Qt frameworks, and the plugins previously inventoried.
PREFIXES = (
    "Core_", "Gui_", "Network_", "DBus_", "QCocoaIntegrationPlugin_",
    "Multimedia_", "BundledPcre2", "BundledLibpng", "BundledLibjpeg",
    "BundledFreetype", "BundledHarfbuzz", "BundledTLSF",
)
# Qt build-level components require confirmation they are embedded in a
# deployed binary; include them rather than risk silently dropping notices.
def candidate(name):
    return ("_Attribution_" in name or name.startswith(PREFIXES)
            or name in {"BundledPcre2", "BundledLibpng", "BundledLibjpeg",
                        "BundledFreetype", "BundledHarfbuzz", "BundledTLSF"})

def clean(value):
    if value is None:
        return "Not supplied"
    if isinstance(value, list):
        return ", ".join(map(str, value)) or "Not supplied"
    return str(value).strip() or "Not supplied"

def main():
    dest = OUT / "Qt-ATTRIBUTION-CANDIDATES.md"
    rows = []
    errors = []
    for module in MODULES:
        path = QT / "sbom" / f"{module}-6.12.0.spdx.json"
        if not path.is_file():
            errors.append(f"Missing {path}")
            continue
        doc = json.loads(path.read_text(encoding="utf-8"))
        for p in doc.get("packages", []):
            name = p.get("name", "")
            if candidate(name):
                rows.append((module, name, clean(p.get("licenseDeclared")),
                             clean(p.get("licenseConcluded")),
                             clean(p.get("copyrightText")),
                             clean(p.get("licenseInfoFromFiles")),
                             clean(p.get("downloadLocation"))))
    lines = [
        "# Qt 6.12.0 bundled third-party attribution candidates",
        "",
        "**NOT A PUBLIC RELEASE NOTICE OR LEGAL CLEARANCE.**",
        "Generated from the installed Qt 6.12.0 SPDX JSON files.",
        "Candidates may include components not embedded in Mod4Mac64.",
        "The shipped bundle still needs a binary-level review.",
        "NOASSERTION is not a licence grant; verify copyright and licence",
        "terms from upstream Qt attribution sources for every applicable item.",
        "",
        f"Candidates: {len(rows)}",
        "",
    ]
    for module, name, declared, concluded, copyright, filelicences, download in rows:
        lines += [
            f"## {name} ({module})",
            f"Declared: {declared}",
            f"Concluded: {concluded}",
            f"From files: {filelicences}",
            f"Source/download: {download}",
            "Copyright:",
            "~~~text",
            copyright.replace("~~~", "---"),
            "~~~",
            "",
        ]
    if errors:
        lines += ["## Input errors", *errors, ""]
    dest.parent.mkdir(parents=True, exist_ok=True)
    dest.write_text("\n".join(lines), encoding="utf-8")
    print(f"Report created: {dest}")
    print(f"Candidate entries: {len(rows)}")
    for err in errors:
        print(err)
    if errors:
        raise SystemExit(1)
    if not rows:
        raise SystemExit("No matching entries: inspect Qt SPDX JSON structure.")

if __name__ == "__main__":
    main()
