#!/usr/bin/env bash
#
# Create GStreamer archive and optionally upload to S3
#
# Usage:
#   gstreamer-archive.sh --platform linux --arch x86_64 --version 1.24.0 [--simulator] [--upload-s3]
#
# Environment variables for S3 upload:
#   AWS_ACCESS_KEY_ID, AWS_SECRET_ACCESS_KEY, AWS_DEFAULT_REGION
#

set -euo pipefail

# Defaults
PLATFORM=""
ARCH=""
VERSION=""
PREFIX=""
SIMULATOR="false"
UPLOAD_S3="false"
OUTPUT_DIR="${RUNNER_TEMP:-.}"

usage() {
    cat >&2 <<EOF
Usage: $(basename "$0") [OPTIONS]

Options:
  --platform PLATFORM   Target platform (linux, macos, windows, android, ios)
  --arch ARCH           Architecture (x86_64, aarch64, arm64, etc.)
  --version VERSION     GStreamer version
  --prefix PREFIX       Installation prefix (default: auto-detect)
  --simulator           iOS simulator build (default: false)
  --upload-s3           Upload to S3 (requires AWS credentials)
  --output-dir DIR      Output directory for archive (default: \$RUNNER_TEMP or .)
  -h, --help            Show this help
EOF
    exit 1
}

# Parse arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        --platform) PLATFORM="$2"; shift 2 ;;
        --arch) ARCH="$2"; shift 2 ;;
        --version) VERSION="$2"; shift 2 ;;
        --prefix) PREFIX="$2"; shift 2 ;;
        --simulator) SIMULATOR="true"; shift ;;
        --upload-s3) UPLOAD_S3="true"; shift ;;
        --output-dir) OUTPUT_DIR="$2"; shift 2 ;;
        -h|--help) usage ;;
        *) echo "Unknown option: $1" >&2; usage ;;
    esac
done

# Validate required arguments
[[ -z "$PLATFORM" ]] && { echo "Error: --platform required" >&2; usage; }
[[ -z "$ARCH" ]] && { echo "Error: --arch required" >&2; usage; }
[[ -z "$VERSION" ]] && { echo "Error: --version required" >&2; usage; }

# Determine prefix path if not specified
if [[ -z "$PREFIX" ]]; then
    if [[ "$PLATFORM" == "windows" ]]; then
        arch="$ARCH"
        [[ "$arch" == "x86_64" ]] && arch="x64"
        PREFIX="C:/gstreamer-$arch"
    else
        PREFIX="${RUNNER_TEMP:-/tmp}/gstreamer"
    fi
fi

# Normalize arch for naming
arch="$ARCH"
[[ "$PLATFORM" == "windows" && "$arch" == "x86_64" ]] && arch="x64"

# Build archive name
case "$PLATFORM" in
    linux)   name="gstreamer-1.0-linux-$arch-$VERSION" ;;
    macos)   name="gstreamer-1.0-macos-$arch-$VERSION" ;;
    windows) name="gstreamer-1.0-msvc-$arch-$VERSION" ;;
    android) name="gstreamer-1.0-android-$arch-$VERSION" ;;
    ios)
        suffix="$arch"
        [[ "$SIMULATOR" == "true" ]] && suffix="$arch-simulator"
        name="gstreamer-1.0-ios-$suffix-$VERSION"
        ;;
    *) echo "Error: Unknown platform: $PLATFORM" >&2; exit 1 ;;
esac

echo "Creating archive: $name"
echo "  Platform: $PLATFORM"
echo "  Arch: $arch"
echo "  Version: $VERSION"
echo "  Prefix: $PREFIX"

# Create archive
if [[ "$PLATFORM" == "windows" ]]; then
    archive_path="$OUTPUT_DIR/$name.zip"
    powershell -Command "Compress-Archive -Path '$PREFIX' -DestinationPath '$archive_path' -CompressionLevel Optimal"
    ext="zip"
else
    archive_path="$OUTPUT_DIR/$name.tar.xz"
    parent_dir=$(dirname "$PREFIX")
    folder_name=$(basename "$PREFIX")
    tar -C "$parent_dir" -cJf "$archive_path" "$folder_name"
    ext="tar.xz"
fi

echo "Created archive: $archive_path"
ls -lh "$archive_path"

# Output for GitHub Actions
if [[ -n "${GITHUB_OUTPUT:-}" ]]; then
    {
        echo "name=$name"
        echo "path=$archive_path"
        echo "ext=$ext"
    } >> "$GITHUB_OUTPUT"
fi

# Upload to S3 if requested
if [[ "$UPLOAD_S3" == "true" ]]; then
    if [[ -z "${AWS_ACCESS_KEY_ID:-}" ]]; then
        echo "Warning: AWS credentials not set, skipping S3 upload" >&2
        exit 0
    fi

    s3_path="s3://qgroundcontrol/dependencies/gstreamer/$PLATFORM/$VERSION"

    # Install AWS CLI if needed (Windows)
    if ! command -v aws &> /dev/null; then
        if [[ "${RUNNER_OS:-}" == "Windows" ]]; then
            curl -o "$OUTPUT_DIR/AWSCLIV2.msi" "https://awscli.amazonaws.com/AWSCLIV2.msi"
            msiexec.exe /i "$OUTPUT_DIR/AWSCLIV2.msi" /quiet
            export PATH="/c/Program Files/Amazon/AWSCLIV2:$PATH"
        else
            echo "Error: AWS CLI not found" >&2
            exit 1
        fi
    fi

    aws s3 cp "$archive_path" "$s3_path/$name.$ext" --acl public-read
    echo "Uploaded to: $s3_path/$name.$ext"
fi

# Write summary if in GitHub Actions
if [[ -n "${GITHUB_STEP_SUMMARY:-}" ]]; then
    [[ "$PLATFORM" == "ios" && "$SIMULATOR" == "true" ]] && arch="$arch-simulator"

    cat >> "$GITHUB_STEP_SUMMARY" << EOF
## GStreamer ${PLATFORM^} Build Complete

| Property | Value |
|----------|-------|
| Version | $VERSION |
| Architecture | $arch |
| Archive | $name.$ext |
EOF

    if [[ "$UPLOAD_S3" == "true" && -n "${AWS_ACCESS_KEY_ID:-}" ]]; then
        echo "| S3 Path | dependencies/gstreamer/$PLATFORM/$VERSION/ |" >> "$GITHUB_STEP_SUMMARY"
    fi
fi
