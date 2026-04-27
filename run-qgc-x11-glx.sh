#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

export QT_QPA_PLATFORM="${QT_QPA_PLATFORM:-xcb}"
export GST_GL_WINDOW="${GST_GL_WINDOW:-x11}"
export GST_GL_PLATFORM="${GST_GL_PLATFORM:-glx}"
export GST_GL_API="${GST_GL_API:-opengl}"

exec "${SCRIPT_DIR}/build/Debug/QGroundControl" "$@"
