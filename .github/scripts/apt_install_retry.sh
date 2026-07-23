#!/usr/bin/env bash
#
# Retry-wrapped apt-get install that downloads .debs into the cached archive
# directory (~/.cache/qgc-apt-archives, saved/restored by actions/cache in
# .github/actions/install-dependencies/action.yml).
#
# Slow apt mirrors can stall downloads past any reasonable budget. A
# shell-level retry loop with `timeout` is used instead of nick-fields/retry:
# that action cannot kill a root-owned (sudo) process on timeout
# ("Error: kill EPERM"), which turns a stalled attempt into a hard step
# failure with no retry. Acquire::*::Timeout aborts stalled mirror
# connections; `timeout` under sudo runs as root and can reliably kill apt.
#
# Usage: apt_install_retry.sh [options] PACKAGE...
#   --update                  run apt-get update before each install attempt
#   --no-install-recommends   pass --no-install-recommends to apt-get install
#   --autoclean               prune superseded .debs from the cache dir afterwards
#   --install-timeout SECS    per-attempt install timeout (default: 1200)
#   --label NAME              label used in warning/error annotations
set -euo pipefail

MAX_ATTEMPTS=3
UPDATE_TIMEOUT=300   # seconds per apt-get update attempt
INSTALL_TIMEOUT=1200 # seconds per apt-get install attempt (~80 min worst case over 3 attempts incl. update timeouts, kill grace, and backoff)
KILL_GRACE=30        # SIGKILL grace period after SIGTERM

run_update=false
autoclean=false
label="apt packages"
install_args=()

while [[ $# -gt 0 ]]; do
  case "$1" in
    --update) run_update=true; shift ;;
    --no-install-recommends) install_args+=(--no-install-recommends); shift ;;
    --autoclean) autoclean=true; shift ;;
    --install-timeout) INSTALL_TIMEOUT="$2"; shift 2 ;;
    --label) label="$2"; shift 2 ;;
    --) shift; break ;;
    -*) echo "::error::unknown option: $1" >&2; exit 2 ;;
    *) break ;;
  esac
done
packages=("$@")
if [[ ${#packages[@]} -eq 0 ]]; then
  echo "::error::no packages given to $(basename "$0")" >&2
  exit 2
fi

APT_CACHE_DIR="${HOME}/.cache/qgc-apt-archives"
mkdir -p "${APT_CACHE_DIR}"
sudo mkdir -p "${APT_CACHE_DIR}/partial"
# Match the stock /var/cache/apt/archives layout (partial/ is _apt:root 0700):
# APT downloads via the _apt sandbox user, and a root-owned 0755 partial/ would
# force noisy "unsandboxed as root" fallback downloads.
if id _apt &> /dev/null; then
  sudo chown _apt:root "${APT_CACHE_DIR}/partial"
fi
sudo chmod 700 "${APT_CACHE_DIR}/partial"
APT_OPTS=(
  -o Acquire::Retries=3
  -o Acquire::http::Timeout=30
  -o Acquire::https::Timeout=30
  -o DPkg::Lock::Timeout=300
  -o "Dir::Cache::Archives=${APT_CACHE_DIR}"
  -o APT::Keep-Downloaded-Packages=true
)

ok=false
for attempt in $(seq 1 "${MAX_ATTEMPTS}"); do
  if { [[ "${run_update}" != "true" ]] ||
       sudo timeout -k "${KILL_GRACE}" "${UPDATE_TIMEOUT}" apt-get "${APT_OPTS[@]}" update -qq; } &&
     sudo timeout -k "${KILL_GRACE}" "${INSTALL_TIMEOUT}" apt-get "${APT_OPTS[@]}" install -y "${install_args[@]}" "${packages[@]}"; then
    ok=true
    break
  fi
  echo "::warning::${label} install attempt ${attempt} failed (stall/timeout or error); retrying"
  sleep $((attempt * 15))
done

if [[ "${autoclean}" == "true" ]]; then
  # Prune superseded .deb versions so the cached directory doesn't grow unboundedly.
  sudo apt-get "${APT_OPTS[@]}" autoclean -qq || true
fi

# Drop root-owned bits so actions/cache (runner user) can save the directory.
# Keep this BEFORE the failure check: the cache post step saves even when the
# calling step fails, letting a retried job reuse already-downloaded .debs.
sudo rm -rf "${APT_CACHE_DIR}/partial"
sudo chown -R "$(id -u):$(id -g)" "${APT_CACHE_DIR}"

if [[ "${ok}" != "true" ]]; then
  echo "::error::${label} installation failed after ${MAX_ATTEMPTS} attempts"
  exit 1
fi
