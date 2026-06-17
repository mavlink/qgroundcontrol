# shellcheck shell=sh
# Sourceable retry helper: `retry <cmd> [args...]` — 3 attempts, linear backoff.
# POSIX sh compatible (sourced from Dockerfile RUN steps before SHELL is bash).
retry() {
    attempt=1
    rc=0
    while [ "${attempt}" -le 3 ]; do
        if "$@"; then
            return 0
        fi
        rc=$?
        if [ "${attempt}" -lt 3 ]; then
            echo "==> attempt ${attempt}/3 failed (rc=${rc}); retrying in ${attempt}0s" >&2
            sleep "${attempt}0"
        fi
        attempt=$((attempt + 1))
    done
    return "${rc}"
}
