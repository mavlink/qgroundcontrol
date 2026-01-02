#!/usr/bin/env bash
#
# Install and configure ccache with signature verification
#
# Usage:
#   install-ccache.sh [--version VERSION] [--arch ARCH] [--config-path PATH]
#
# Outputs (for GitHub Actions):
#   version, arch, max_size
#

set -euo pipefail

# Defaults - pin version for reproducibility
CCACHE_VERSION="4.12.2"
CCACHE_ARCH=""
CCACHE_CONF_PATH=""
INSTALL_BINARY="false"

# Public key from https://ccache.dev/download.html (v4.12+)
CCACHE_MINISIGN_KEY="RWQX7yXbBedVfI4PNx6FLdFXu9GHUFsr28s4BVGxm4BeybtnX3P06saF"

usage() {
    cat >&2 <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --version VERSION     ccache version (default: $CCACHE_VERSION)
  --arch ARCH           Architecture: x86_64 or aarch64 (default: auto-detect)
  --config-path PATH    Path to ccache.conf for max_size
  --install             Install ccache binary (Linux only)
  --output-only         Only output config, don't install
  -h, --help            Show this help

Outputs (GITHUB_OUTPUT):
  version   - ccache version
  arch      - Architecture
  max_size  - Max cache size from config
EOF
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --version) CCACHE_VERSION="$2"; shift 2 ;;
        --arch) CCACHE_ARCH="$2"; shift 2 ;;
        --config-path) CCACHE_CONF_PATH="$2"; shift 2 ;;
        --install) INSTALL_BINARY="true"; shift ;;
        --output-only) INSTALL_BINARY="false"; shift ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

# Validate version format
if [[ ! "$CCACHE_VERSION" =~ ^[0-9]+\.[0-9]+(\.[0-9]+)?$ ]]; then
    echo "Error: Invalid ccache version format: $CCACHE_VERSION" >&2
    exit 1
fi

# Auto-detect architecture
if [[ -z "$CCACHE_ARCH" ]]; then
    case "$(uname -m)" in
        x86_64|amd64) CCACHE_ARCH="x86_64" ;;
        aarch64|arm64) CCACHE_ARCH="aarch64" ;;
        *) CCACHE_ARCH="x86_64" ;;
    esac
fi

# Read max_size from config if provided
CCACHE_MAX_SIZE="2G"
if [[ -n "$CCACHE_CONF_PATH" && -f "$CCACHE_CONF_PATH" ]]; then
    # Single awk pass to extract value
    extracted=$(awk -F'=' '/^max_size[[:space:]]*=/{gsub(/[[:space:]]/,"",$2); print $2; exit}' "$CCACHE_CONF_PATH")
    CCACHE_MAX_SIZE="${extracted:-2G}"
elif [[ -n "$CCACHE_CONF_PATH" ]]; then
    echo "Warning: ccache config not found at $CCACHE_CONF_PATH, using default max_size" >&2
fi

echo "ccache configuration:"
echo "  Version: $CCACHE_VERSION"
echo "  Arch: $CCACHE_ARCH"
echo "  Max Size: $CCACHE_MAX_SIZE"

# Output for GitHub Actions
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    {
        echo "version=$CCACHE_VERSION"
        echo "arch=$CCACHE_ARCH"
        echo "max_size=$CCACHE_MAX_SIZE"
    } >> "$GITHUB_OUTPUT"
fi

# Install ccache binary if requested (Linux only)
if [[ "$INSTALL_BINARY" == "true" ]]; then
    if [[ "$(uname)" != "Linux" ]]; then
        echo "Warning: Binary installation only supported on Linux" >&2
        exit 0
    fi

    # Check if already installed
    if command -v ccache &> /dev/null; then
        installed_version=$(ccache --version | head -1 | grep -oE '[0-9]+\.[0-9]+(\.[0-9]+)?')
        if [[ "$installed_version" == "$CCACHE_VERSION" ]]; then
            echo "ccache $CCACHE_VERSION already installed"
            exit 0
        fi
    fi

    echo "Installing ccache $CCACHE_VERSION..."

    ARCHIVE="ccache-${CCACHE_VERSION}-linux-${CCACHE_ARCH}.tar.xz"
    TEMP_DIR=$(mktemp -d)
    trap 'rm -rf "$TEMP_DIR"' EXIT

    cd "$TEMP_DIR"

    # Download ccache and signature
    wget --quiet --tries=3 --waitretry=5 \
        "https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/${ARCHIVE}"
    wget --quiet --tries=3 --waitretry=5 \
        "https://github.com/ccache/ccache/releases/download/v${CCACHE_VERSION}/${ARCHIVE}.minisig"

    # Download and extract minisign
    MINISIGN_VERSION="0.11"
    wget --quiet --tries=3 --waitretry=5 \
        "https://github.com/jedisct1/minisign/releases/download/${MINISIGN_VERSION}/minisign-${MINISIGN_VERSION}-linux.tar.gz"
    tar -xzf "minisign-${MINISIGN_VERSION}-linux.tar.gz"

    # Use appropriate minisign binary for architecture
    if [[ "$CCACHE_ARCH" == "aarch64" ]]; then
        MINISIGN_BIN="./minisign-linux/aarch64/minisign"
    else
        MINISIGN_BIN="./minisign-linux/x86_64/minisign"
    fi

    # Verify signature
    "$MINISIGN_BIN" -Vm "$ARCHIVE" -P "$CCACHE_MINISIGN_KEY"

    # Extract and install
    tar -xf "$ARCHIVE"
    sudo cp "ccache-${CCACHE_VERSION}-linux-${CCACHE_ARCH}/ccache" /usr/local/bin/ccache
    sudo chmod +x /usr/local/bin/ccache

    echo "ccache $CCACHE_VERSION installed successfully"
    ccache --version
fi
