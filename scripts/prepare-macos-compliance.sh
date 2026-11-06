#!/usr/bin/env bash
# Prepare Mac distribution licence inputs; runs on the macOS build machine.
# This collects upstream texts but does NOT certify release compliance.
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
OUT="$ROOT/dist/compliance-macos"
mkdir -p "$OUT"
download() {
  local url="$1" target="$2"
  echo "Fetching $target"
  curl --fail --location --silent --show-error --retry 2 "$url" -o "$OUT/$target.tmp"
  test -s "$OUT/$target.tmp"
  mv "$OUT/$target.tmp" "$OUT/$target"
}
# FSF canonical complete licence texts
download https://www.gnu.org/licenses/lgpl-3.0.txt LGPL-3.0.txt
download https://www.gnu.org/licenses/gpl-3.0.txt GPL-3.0.txt
download https://www.gnu.org/licenses/old-licenses/lgpl-2.1.txt LGPL-2.1.txt

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
missing=0
copy_licence libopenmpt libopenmpt-LICENSE.txt || missing=1
copy_licence libogg libogg-LICENSE.txt || missing=1
copy_licence libvorbis libvorbis-LICENSE.txt || missing=1

# Qt's official attribution documents depend on exactly which modules and
# embedded third-party components were deployed. Do NOT invent a placeholder.
echo
echo "Required manual verification:"
echo "  1. $OUT/Qt-Third-Party-Notices.txt: complete relevant Qt 6.12.0"
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
