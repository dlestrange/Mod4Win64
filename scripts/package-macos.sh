#!/bin/bash
# Package the Apple Silicon Mod4Mac64 beta outside the source/build tree.
# Requires: Qt macdeployqt, Homebrew libopenmpt, dylibbundler, codesign.
# This produces a LOCAL TEST BUILD, not a Developer ID signed/notarised release.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${BUILD_DIR:-$ROOT/out/build/macos-arm64}"
QT_DIR="${QT_DIR:-$HOME/Qt/6.12.0/macos}"
VERSION="${VERSION:-v0.9.0-beta.1}"
DIST_DIR="${DIST_DIR:-$ROOT/dist}"
APP_NAME=Mod4Mac64.app
APP="$DIST_DIR/$APP_NAME"
SOURCE_APP="$BUILD_DIR/$APP_NAME"
ZIP="$DIST_DIR/Mod4Mac64-$VERSION-macos-arm64.zip"

for command in ditto brew dylibbundler otool codesign file shasum; do
    command -v "$command" >/dev/null || { echo "Missing $command" >&2; exit 1; }
done
[[ -x "$QT_DIR/bin/macdeployqt" ]] || { echo "Missing macdeployqt: $QT_DIR/bin/macdeployqt" >&2; exit 1; }
[[ -d "$SOURCE_APP" ]] || { echo "Build the app first: $SOURCE_APP" >&2; exit 1; }
OPENMPT="$(brew --prefix libopenmpt)/lib/libopenmpt.dylib"
[[ -f "$OPENMPT" ]] || { echo "Missing libopenmpt at $OPENMPT" >&2; exit 1; }

mkdir -p "$DIST_DIR"
rm -rf "$APP"
rm -f "$ZIP"
ditto "$SOURCE_APP" "$APP"
"$QT_DIR/bin/macdeployqt" "$APP" -no-strip
# Mod4Mac64 uses QAudioSink/QMediaDevices and dynamically loaded libopenmpt,
# not Qt's media-player/recording backend. Qt's deployment tool includes the
# unused FFmpeg plugin and shared libraries; omit them from this test package.
# Revalidate audio, module playback, and portability after this change.
echo "Removing unused Qt FFmpeg media backend from test package..."
rm -f "$APP/Contents/PlugIns/multimedia/libffmpegmediaplugin.dylib"
for ffmpeg_lib in libavcodec.*.dylib libavformat.*.dylib libavutil.*.dylib libswresample.*.dylib libswscale.*.dylib; do
    rm -f "$APP/Contents/Frameworks/"/$ffmpeg_lib
done
if find "$APP/Contents" -type f \( -name 'libffmpegmediaplugin.dylib' -o -name 'libavcodec.*.dylib' -o -name 'libavformat.*.dylib' -o -name 'libavutil.*.dylib' -o -name 'libswresample.*.dylib' -o -name 'libswscale.*.dylib' \) | grep -q .; then
    echo "FFmpeg binaries remain in package; aborting." >&2
    exit 1
fi
mkdir -p "$APP/Contents/Frameworks"
cp -L "$OPENMPT" "$APP/Contents/Frameworks/libopenmpt.dylib"
dylibbundler -b -x "$APP/Contents/Frameworks/libopenmpt.dylib" \
    -d "$APP/Contents/Frameworks" -p "@loader_path/" -s /opt/homebrew/lib

# Homebrew gives libopenmpt an absolute install name. dylibbundler fixes its
# dependencies, but not necessarily the dylib's own identity, which otool -L
# also reports. Replace it with a relocatable install name before final signing.
install_name_tool -id "@rpath/libopenmpt.dylib" \
    "$APP/Contents/Frameworks/libopenmpt.dylib"

# Audit all bundled Mach-O libraries (including Qt plug-ins) for developer-
# machine runtime paths. A clean audit is necessary but not sufficient:
# always test from a fresh user account / another Mac as well.
echo "Auditing embedded runtime dependencies..."
leaks=0
while IFS= read -r -d '' file; do
    if file "$file" | grep -q 'Mach-O'; then
        if otool -L "$file" | tail -n +2 | grep -E '/opt/homebrew/|/usr/local/Cellar/|/Users/[^/]+/Qt/'; then
            echo "Unbundled dependency referenced by: $file" >&2
            leaks=1
        fi
    fi
done < <(find "$APP" -type f -print0)
if [[ "$leaks" -ne 0 ]]; then
    echo "Packaging stopped: fix the remaining developer-machine paths." >&2
    exit 1
fi

# Ad-hoc signing is for local/limited technical testing only. It does NOT
# produce a notarised, normal-Gatekeeper-friendly public distribution.
codesign --force --deep --sign - "$APP"
codesign --verify --deep --strict --verbose=2 "$APP"
ditto -c -k --sequesterRsrc --keepParent "$APP" "$ZIP"
shasum -a 256 "$ZIP"
echo "Created: $ZIP"
echo "IMPORTANT: Not Developer ID signed or notarised."
echo "IMPORTANT: Include third-party notices and meet LGPL source obligations before publishing."
