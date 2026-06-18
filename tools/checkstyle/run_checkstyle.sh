#!/usr/bin/env bash
# Run Checkstyle over first-party QGroundControl Android Java with android/checkstyle.xml.
#
# Self-contained for pre-commit + CI: resolves a pinned, checksum-verified Checkstyle jar from
# (1) $CHECKSTYLE_JAR, (2) the local cache, or (3) a one-time download to the cache. The CI
# pre-commit job sets up no Java toolchain of its own, so this script must not assume one beyond a
# `java` on PATH (ubuntu-latest ships a Temurin JDK).
#
# Usage: run_checkstyle.sh <file.java> [<file.java> ...]   # pre-commit passes the staged files
set -euo pipefail

readonly VERSION="10.21.0"
readonly SHA256="911fee0b8a8495f0d8a168815e91bb8abb3f497c96a46d8b78e433a516bc88e7"
readonly JAR_NAME="checkstyle-${VERSION}-all.jar"
readonly URL="https://github.com/checkstyle/checkstyle/releases/download/checkstyle-${VERSION}/${JAR_NAME}"

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/../.." && pwd)"
readonly SCRIPT_DIR REPO_ROOT
readonly CONFIG="${REPO_ROOT}/android/checkstyle.xml"
readonly CACHE_DIR="${XDG_CACHE_HOME:-${HOME}/.cache}/qgc-tools"

[ "$#" -eq 0 ] && exit 0  # pre-commit invoked us with no matching files

verify() { echo "${SHA256}  $1" | sha256sum --check --status; }

resolve_jar() {
    if [ -n "${CHECKSTYLE_JAR:-}" ] && [ -f "${CHECKSTYLE_JAR}" ] && verify "${CHECKSTYLE_JAR}"; then
        printf '%s' "${CHECKSTYLE_JAR}"
        return
    fi
    local cached="${CACHE_DIR}/${JAR_NAME}"
    if [ -f "${cached}" ] && verify "${cached}"; then
        printf '%s' "${cached}"
        return
    fi
    mkdir -p "${CACHE_DIR}"
    local tmp="${cached}.tmp.$$"
    if command -v curl >/dev/null 2>&1; then
        curl -fsSL "${URL}" -o "${tmp}"
    else
        wget -qO "${tmp}" "${URL}"
    fi
    if ! verify "${tmp}"; then
        rm -f "${tmp}"
        echo "checkstyle: downloaded jar failed SHA-256 check (expected ${SHA256})" >&2
        exit 1
    fi
    mv -f "${tmp}" "${cached}"
    printf '%s' "${cached}"
}

if ! command -v java >/dev/null 2>&1; then
    echo "checkstyle: 'java' not found on PATH (need a JRE/JDK to run Checkstyle)" >&2
    exit 1
fi

JAR="$(resolve_jar)"
exec java -jar "${JAR}" -c "${CONFIG}" "$@"
