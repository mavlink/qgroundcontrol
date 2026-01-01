#! /usr/bin/env bash

set -euo pipefail

# Source build config (required - no fallbacks)
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
if [[ -f "$SCRIPT_DIR/read-config.sh" ]]; then
    source "$SCRIPT_DIR/read-config.sh"
else
    echo "Error: read-config.sh not found" >&2
    exit 1
fi

# Verify required variables are set
if [[ -z "${GSTREAMER_MACOS_VERSION:-}" ]]; then
    echo "Error: GSTREAMER_MACOS_VERSION must be set (check build-config.json)" >&2
    exit 1
fi

if ! command -v brew &> /dev/null
then
    # install Homebrew if not installed yet
    /bin/bash -c "$(curl --retry 3 --retry-delay 5 -fsSL https://raw.githubusercontent.com/Homebrew/install/master/install.sh)"
    eval "$(/usr/local/bin/brew shellenv)"
fi

brew update
brew install cmake ninja ccache git pkgconf create-dmg mold

# Install GStreamer
GST_URL=https://gstreamer.freedesktop.org/data/pkg/osx
GST_VERSION="$GSTREAMER_MACOS_VERSION"
GST_PKG=gstreamer-1.0-$GST_VERSION-universal.pkg
GST_DEV_PKG=gstreamer-1.0-devel-$GST_VERSION-universal.pkg
pushd "$TMPDIR" || exit
curl --retry 3 --retry-delay 5 -fSLO "$GST_URL/$GST_VERSION/$GST_DEV_PKG"
curl --retry 3 --retry-delay 5 -fSLO "$GST_URL/$GST_VERSION/$GST_PKG"
echo "Sudo may be required to install GStreamer"
sudo installer -pkg "$GST_PKG" -target /
sudo installer -pkg "$GST_DEV_PKG" -target /
rm "$TMPDIR/$GST_DEV_PKG"
rm "$TMPDIR/$GST_PKG"
popd || exit
