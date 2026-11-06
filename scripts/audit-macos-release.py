#!/usr/bin/env python3
"""Audit Mod4Mac64 beta release assets without changing or publishing them.

Checks distribution structure, licence documents, source checksums, and
release source identification. Does not certify licence compliance.
"""
from __future__ import annotations
import hashlib
import os
from pathlib import Path
import subprocess
import sys
import zipfile

ROOT=Path(__file__).resolve().parent.parent
DIST=ROOT/"dist"
APP_ZIP=DIST/"Mod4Mac64-v0.9.0-beta.1-macos-arm64.zip"
SOURCE=DIST/"macos-source-assets"
COMPLIANCE=DIST/"compliance-macos"
NEEDED_SOURCES=("qt-everywhere-src-6.12.0.tar.xz","mpg123-1.33.7.tar.bz2")
NEEDED_LICENSES=(
    "LGPL-3.0.txt","GPL-3.0.txt","LGPL-2.1.txt",
    "Qt-Third-Party-Notices.txt","libopenmpt-LICENSE.txt",
    "libogg-LICENSE.txt","libvorbis-LICENSE.txt","mpg123-LICENSE.txt",
    "SOURCE-AND-REPLACEMENT-INSTRUCTIONS.txt",
)
results=[]
def check(label, ok, note=""):
    results.append((label, bool(ok),note))
    print(f"{'PASS' if ok else 'FAIL'}  {label}" + (f" — {note}" if note else ""))

def checksum(path):
    h=hashlib.sha256()
    with path.open("rb") as fp:
        for part in iter(lambda:fp.read(4*1024*1024),b""):
            h.update(part)
    return h.hexdigest()

def main():
    if not APP_ZIP.is_file():
        check("Mac ZIP exists",False,str(APP_ZIP))
        return 1
    check("Mac ZIP exists",True,f"{APP_ZIP.stat().st_size/1024/1024:.1f} MiB")
    check("Mac ZIP SHA256",True,checksum(APP_ZIP))
    try:
        with zipfile.ZipFile(APP_ZIP) as z:
            names=set(z.namelist())
            broken=z.testzip()
            check("ZIP data integrity",broken is None,str(broken or "OK"))
            prefix="Mod4Mac64.app/Contents/"
            licences=prefix+"Resources/Licenses/"
            for name in NEEDED_LICENSES:
                path=licences+name
                check("Bundled licence: "+name,path in names and z.getinfo(path).file_size>0 if path in names else False)
            check("Bundled original Qt attribution folder",
                  any(x.startswith(licences+"Qt-6.12.0-Original-Licenses/") and x.endswith("qt_attribution.json") for x in names))
            check("Qt framework present",any(x.startswith(prefix+"Frameworks/QtCore.framework/") for x in names))
            check("libmpg123 present",prefix+"Frameworks/libmpg123.0.dylib" in names)
            unwanted=[n for n in names if n.endswith(("libffmpegmediaplugin.dylib",
                    "libavcodec.dylib","libavformat.dylib","libavutil.dylib"))
                    or "/libavcodec." in n or "/libavformat." in n or "/libswresample." in n or "/libswscale." in n]
            check("No FFmpeg payload",not unwanted,", ".join(unwanted[:3]))
    except (zipfile.BadZipFile,OSError) as ex:
        check("ZIP readable",False,str(ex))
    for name in NEEDED_LICENSES:
        p=COMPLIANCE/name
        check("Local compliance file: "+name,p.is_file() and p.stat().st_size>0 if p.exists() else False)
    manifest=SOURCE/"SHA256SUMS.txt"
    entries={}
    if manifest.is_file():
        for line in manifest.read_text().splitlines():
            fields=line.split(maxsplit=1)
            if len(fields)==2:entries[fields[1].lstrip("*")]=fields[0].lower()
    for name in NEEDED_SOURCES:
        p=SOURCE/name
        valid=p.is_file() and name in entries and checksum(p)==entries[name]
        check("Source checksum: "+name,valid)
    instructions=COMPLIANCE/"SOURCE-AND-REPLACEMENT-INSTRUCTIONS.txt"
    if instructions.is_file():
        text=instructions.read_text(errors="replace")
        check("Source instructions identify Qt and mpg123",
              all(name in text for name in NEEDED_SOURCES))
        git_sha=subprocess.run(["git","rev-parse","HEAD"],cwd=ROOT,text=True,
                            capture_output=True,check=False).stdout.strip()
        if git_sha:
            check("Instructions identify current source checkout",
                  git_sha in text,f"checkout={git_sha[:12]} (does not prove binary was built from it)")
    failures=sum(not good for _,good,_ in results)
    print(f"\n{len(results)-failures} passed; {failures} failed.")
    print("This is an inventory/integrity check only, not legal clearance.")
    print("Still review source-to-binary correspondence, applicable notices,")
    print("build/patch information, LGPL replacement freedoms and public availability.")
    return 1 if failures else 0

if __name__=="__main__":
    sys.exit(main())
