#!/bin/sh
# Builds Game_of_Life-x86_64.AppImage in the current directory from an installed build:
#   cmake --install build --prefix <dir> --component gol
#   packaging/linux/make-appimage.sh <dir> <path to appimagetool>
# OpenGL and X11 are not bundled, every Linux desktop has them (and bundling OpenGL breaks GPU drivers).
set -e
installed=$1
appimagetool=$2
here=$(dirname "$0")
appdir=$(mktemp -d)/AppDir

mkdir -p "$appdir/usr/bin"
cp "$installed/gol" "$appdir/usr/bin/"
cp -r "$installed/res" "$appdir/usr/bin/" # The game looks for res next to its executable
cp "$here/gol.desktop" "$here/gol.png" "$appdir/"
ln -s usr/bin/gol "$appdir/AppRun"

ARCH=x86_64 "$appimagetool" --no-appstream "$appdir" Game_of_Life-x86_64.AppImage
