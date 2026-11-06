#!/usr/bin/env bash
# Prepare Mac distribution licence inputs; runs on the macOS build machine.
# This collects upstream texts but does NOT certify release compliance.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/dist/compliance-macos"
mkdir -p "$OUT"
download() {
  local url="$1" target="$2"
  if [[ -s "$OUT/$target" ]]; then
    echo "Already present: $target (skipping network download)"
    return 0
  fi
  echo "Fetching $target"
  if ! curl --fail --location --silent --show-error \
      --connect-timeout 8 --max-time 30 --retry 1 "$url" -o "$OUT/$target.tmp"; then
    rm -f "$OUT/$target.tmp"
    echo "Download failed: $target. Existing files were preserved." >&2
    return 1
  fi
  test -s "$OUT/$target.tmp"
  mv "$OUT/$target.tmp" "$OUT/$target"
}
# FSF canonical complete licence texts
missing=0
download https://www.gnu.org/licenses/lgpl-3.0.txt LGPL-3.0.txt || missing=1
download https://www.gnu.org/licenses/gpl-3.0.txt GPL-3.0.txt || missing=1
download https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt LGPL-2.1.txt || missing=1

# Use copyright-bearing texts from the exact installed Homebrew bottles if present.
copy_licence() {
  local formula="$1" name="$2" prefix candidate
  prefix="$(brew --prefix "$formula")"
  for candidate in "$prefix/LICENSE" "$prefix/LICENSE.txt" "$prefix/COPYING" \
      "$prefix/share/doc/$formula/COPYING" "$prefix/share/doc/$formula/LICENSE"; do
    if [[ -s "$candidate" ]]; then
      cp "$candidate" "$OUT/$name"
      echo "Copied $formula licence from $candidate"
      return
    fi
  done
  echo "No installed licence for $formula; requires manual review" >&2
  return 1
}
copy_licence libopenmpt libopenmpt-LICENSE.txt || missing=1
copy_licence mpg123 mpg123-LICENSE.txt || missing=1
copy_licence libogg libogg-LICENSE.txt || missing=1
copy_licence libvorbis libvorbis-LICENSE.txt || missing=1

# Copy the vendor-provided Qt 6.12.0 SBOM files for these deployed
# modules; SBOMs are useful evidence, NOT the required full notice texts.
QT_DIR="${QT_DIR:-$HOME/Qt/6.12.0/macos}"
QT_SBOM="$OUT/Qt-6.12.0-SBOM"
mkdir -p "$QT_SBOM"
for module in qtbase qtmultimedia qtsvg; do
  src="$QT_DIR/sbom/${module}-6.12.0.spdx"
  if [[ -s "$src" ]]; then
    cp "$src" "$QT_SBOM/"
    echo "Copied Qt SBOM: $module"
  else
    echo "Missing expected Qt SBOM: $src" >&2
    missing=1
  fi
done

# Qt's official attribution documents depend on exactly which modules and
# embedded third-party components were deployed. Do NOT invent a placeholder.
echo
echo "Required manual verification:"
echo "  1. Review Qt SBOMs in $QT_SBOM and generate/verify" 
echo "     $OUT/Qt-Third-Party-Notices.txt: complete relevant Qt 6.12.0"
echo "     third-party notices, based on deployed Qt modules/plugins."
echo "  2. $OUT/SOURCE-AND-REPLACEMENT-INSTRUCTIONS.txt: truthful public"
echo "     corresponding-source download and LGPL replacement instructions."
echo "  3. Verify all copied notices against corresponding source and licenses."
echo "  4. Supply any additional nested component texts and corresponding sources."
echo
echo "Files collected:"
find "$OUT" -maxdepth 1 -type f -print
if ((missing)); then
  echo "Some licence files were not found; collect them from exact upstream source versions." >&2
  exit 1
fi
