#!/usr/bin/env bash
# Fetch exact upstream source archives required for the Mod4Mac64 Mac beta.
# This creates ORIGINAL source assets and SHA256 manifest, not release clearance.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
DIR="$ROOT/dist/macos-source-assets"
mkdir -p "$DIR"
fetch() {
    local url="$1" file="$2" md5="$3" out="$DIR/$file"
    if [[ -s "$out" ]] && echo "$md5  $out" | md5sum -c >/dev/null 2>&1; then
        echo "Verified existing $file"
        return
    fi
    # macOS ships md5 rather than md5sum.
    if [[ -s "$out" ]] && [[ "$(md5 -q "$out")" == "$md5" ]]; then
        echo "Verified existing $file"
        return
    fi
    echo "Downloading $file (may be large; resume supported)..."
    curl --fail --location --show-error --retry 2 --connect-timeout 15 \
        --continue-at - "$url" -o "$out"
    if [[ "$(md5 -q "$out")" != "$md5" ]]; then
        echo "Checksum mismatch: $file; do NOT distribute" >&2
        exit 1
    fi
    echo "Verified $file"
}
# Official Qt 6.12.0 full corresponding source, including QtBase,
# QtMultimedia, QtSVG and their third-party directories / licence texts.
# ~423 MB. MD5 from published archive metadata; retain SHA256 manifest.
fetch "https://download.qt.io/archive/qt/6.12/6.12.0/single/qt-everywhere-src-6.12.0.tar.xz" \
      "qt-everywhere-src-6.12.0.tar.xz" "ac677cc64158740f257fd37a5b4c5ffb"
# Exact matching mpg123 1.33.7 source, ~1.1 MB.
fetch "https://downloads.sourceforge.net/mpg123/mpg123-1.33.7.tar.bz2" \
      "mpg123-1.33.7.tar.bz2" "46ceab9bfb0f04e739f010b88de5943d"
(cd "$DIR" && shasum -a 256 *.tar.* > SHA256SUMS.txt)
echo "Source assets:"
ls -lh "$DIR"
echo
echo "PUBLISHING NOTE: These archives alone do not discharge every LGPL obligation."
echo "Confirm they correspond to the distributed binaries, any patches and"
echo "build configuration, and the required library replacement instructions."
echo "Upload the verified archives and SHA256SUMS.txt alongside the binary ZIP."
