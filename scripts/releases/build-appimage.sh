#!/usr/bin/env bash
# Build and package ofxRemoteUI Client as an AppImage.
# Usage: build-appimage.sh [output-directory]
# Optional: OFX_REMOTE_UI_PROJECT, LINUXDEPLOY, APPIMAGETOOL, APPIMAGE_RUNTIME, JOBS.
set -euo pipefail

script_dir=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)
repo_root=$(cd -- "$script_dir/../.." && pwd)
of_workspace=$(cd -- "$repo_root/../.." && pwd)
client_dir="$repo_root/ImguiClient"
project_dir=${OFX_REMOTE_UI_PROJECT:-"$of_workspace/Projects/ofxRemoteUIClient"}
binary="$project_dir/bin/ofxRemoteUIClient"
output_dir=${1:-"$client_dir/dist"}
cache_dir="${XDG_CACHE_HOME:-$HOME/.cache}/ofxremoteui-appimage-tools"

if [[ ${1:-} == --help || ${1:-} == -h ]]; then
    echo "Usage: $0 [output-directory]"
    echo "Builds the x86_64 Linux client first. Set OFX_REMOTE_UI_PROJECT to use another project directory."
    echo "Optional: set LINUXDEPLOY and APPIMAGETOOL to existing tool paths."
    exit 0
fi
[[ $(uname -s) == Linux && $(uname -m) == x86_64 ]] || { echo 'This script requires x86_64 Linux.' >&2; exit 1; }
for cmd in curl convert file make; do
    command -v "$cmd" >/dev/null || { echo "Missing required command: $cmd" >&2; exit 1; }
done
[[ -f $project_dir/Makefile ]] || { echo "Client project not found: $project_dir" >&2; exit 1; }
echo "Building ofxRemoteUI Client in $project_dir..."
make -C "$project_dir" -j"${JOBS:-$(nproc)}" Release
[[ -x $binary ]] || { echo "Build succeeded but executable not found: $binary" >&2; exit 1; }
file -b "$binary" | grep -q 'ELF 64-bit.*x86-64' || { echo "Not an x86_64 Linux executable: $binary" >&2; exit 1; }
[[ -d $client_dir/bin/data/fonts && -f $client_dir/assets/RemoteUI.png ]] || { echo 'App icon or fonts are missing.' >&2; exit 1; }

mkdir -p "$output_dir" "$cache_dir"
output_dir=$(cd -- "$output_dir" && pwd)
output="$output_dir/ofxRemoteUIClient-x86_64.AppImage"
[[ ! -e $output ]] || { echo "Output already exists: $output (move it or choose another directory)." >&2; exit 1; }

get_tool() {
    local destination=$1 url=$2
    if [[ ! -x $destination ]]; then
        echo "Downloading $(basename "$destination")..." >&2
        local temporary="$destination.download.$$"
        if ! curl -fL --retry 2 -o "$temporary" "$url"; then rm -f "$temporary"; return 1; fi
        chmod +x "$temporary"
        mv -f "$temporary" "$destination"
    fi
}
linuxdeploy=${LINUXDEPLOY:-"$cache_dir/linuxdeploy-x86_64.AppImage"}
appimagetool=${APPIMAGETOOL:-"$cache_dir/appimagetool-x86_64.AppImage"}
runtime=${APPIMAGE_RUNTIME:-"$cache_dir/runtime-x86_64"}
get_tool "$linuxdeploy" 'https://github.com/linuxdeploy/linuxdeploy/releases/download/continuous/linuxdeploy-x86_64.AppImage'
get_tool "$appimagetool" 'https://github.com/AppImage/appimagetool/releases/download/continuous/appimagetool-x86_64.AppImage'
get_tool "$runtime" 'https://github.com/AppImage/type2-runtime/releases/download/continuous/runtime-x86_64'

build_dir=$(mktemp -d "${TMPDIR:-/tmp}/ofxremoteui-appimage.XXXXXXXX")
trap 'rm -rf -- "$build_dir"' EXIT
appdir="$build_dir/AppDir"
mkdir -p "$appdir/usr/bin/data"
cp -R "$client_dir/bin/data/fonts" "$appdir/usr/bin/data/"
cp "$client_dir/assets/RemoteUI.png" "$appdir/usr/bin/data/RemoteUI.png"
convert "$client_dir/assets/RemoteUI.png" -resize 256x256 "$build_dir/ofxRemoteUIClient.png"
cat > "$build_dir/ofxRemoteUIClient.desktop" <<'DESKTOP'
[Desktop Entry]
Type=Application
Name=ofxRemoteUI Client
Comment=Connect to and edit ofxRemoteUI parameters
Exec=ofxRemoteUIClient
Icon=ofxRemoteUIClient
StartupWMClass=ofxRemoteUI Client
StartupNotify=false
Terminal=false
Categories=Utility;
DESKTOP

"$linuxdeploy" --appimage-extract-and-run --appdir "$appdir" --executable "$binary" \
    --desktop-file "$build_dir/ofxRemoteUIClient.desktop" --icon-file "$build_dir/ofxRemoteUIClient.png"
rm "$appdir/AppRun"
cat > "$appdir/AppRun" <<'APPRUN'
#!/bin/sh
set -eu
APPDIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
runtime_dir="${XDG_DATA_HOME:-$HOME/.local/share}/ofxRemoteUIClient/appimage-runtime"
mkdir -p "$runtime_dir/data"
# The current client writes settings next to its data, so run a writable copy.
# Refresh shipped assets but never overwrite user-created configuration files.
cp -R "$APPDIR/usr/bin/data/fonts" "$runtime_dir/data/"
cp "$APPDIR/usr/bin/data/RemoteUI.png" "$runtime_dir/data/RemoteUI.png"
cp "$APPDIR/usr/bin/ofxRemoteUIClient" "$runtime_dir/ofxRemoteUIClient.$$"
chmod 755 "$runtime_dir/ofxRemoteUIClient.$$"
mv -f "$runtime_dir/ofxRemoteUIClient.$$" "$runtime_dir/ofxRemoteUIClient"
export LD_LIBRARY_PATH="$APPDIR/usr/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
cd "$runtime_dir"
exec "$runtime_dir/ofxRemoteUIClient" "$@"
APPRUN
chmod +x "$appdir/AppRun"
ARCH=x86_64 "$appimagetool" --appimage-extract-and-run --runtime-file "$runtime" "$appdir" "$output"
echo "Created: $output"
