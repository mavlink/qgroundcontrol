#!/usr/bin/env python3
"""Install apt packages with bounded retries and a runner-owned download cache."""

from __future__ import annotations

import argparse
import os
import subprocess
import sys
import time
from pathlib import Path

MAX_ATTEMPTS = 3
UPDATE_TIMEOUT = 300
INSTALL_TIMEOUT = 1200
KILL_GRACE = 30


def _positive_seconds(value: str) -> int:
    seconds = int(value)
    if seconds <= 0:
        raise argparse.ArgumentTypeError("timeout must be positive")
    return seconds


def _retry(options: list[str], operation: list[str], timeout: int, label: str) -> bool:
    # timeout must run under sudo: an unprivileged Python timeout cannot kill
    # root-owned apt processes. Mirror timeouts also bound stalled connections.
    command = [
        "sudo",
        "timeout",
        "-k",
        str(KILL_GRACE),
        str(timeout),
        "apt-get",
        *options,
        *operation,
    ]
    for attempt in range(1, MAX_ATTEMPTS + 1):
        if subprocess.run(command, check=False).returncode == 0:
            return True
        if attempt < MAX_ATTEMPTS:
            print(f"::warning::{label} attempt {attempt} failed (stall/timeout or error); retrying")
            time.sleep(attempt * 15)
    return False


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--update", action="store_true")
    parser.add_argument("--no-install-recommends", action="store_true")
    parser.add_argument("--autoclean", action="store_true")
    parser.add_argument("--install-timeout", type=_positive_seconds, default=INSTALL_TIMEOUT)
    parser.add_argument("--label", default="apt packages")
    parser.add_argument("packages", nargs="+")
    args = parser.parse_args(argv)

    cache_dir = Path.home() / ".cache/qgc-apt-archives"
    partial_dir = cache_dir / "partial"
    options = [
        "-o",
        "Acquire::Retries=3",
        "-o",
        "Acquire::http::Timeout=30",
        "-o",
        "Acquire::https::Timeout=30",
        "-o",
        "DPkg::Lock::Timeout=300",
        "-o",
        f"Dir::Cache::Archives={cache_dir}",
        "-o",
        "APT::Keep-Downloaded-Packages=true",
    ]
    try:
        cache_dir.mkdir(parents=True, exist_ok=True)
        try:
            subprocess.run(["sudo", "mkdir", "-p", str(partial_dir)], check=True)
            apt_user = subprocess.run(
                ["id", "_apt"], stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL, check=False
            )
            # Match apt's sandbox-owned partial directory to avoid root downloads.
            if apt_user.returncode == 0:
                subprocess.run(["sudo", "chown", "_apt:root", str(partial_dir)], check=True)
            subprocess.run(["sudo", "chmod", "700", str(partial_dir)], check=True)

            if args.update and not _retry(options, ["update", "-qq"], UPDATE_TIMEOUT, "apt update"):
                print(f"::error::apt update failed after {MAX_ATTEMPTS} attempts", file=sys.stderr)
                return 1
            operation = ["install", "-y"]
            if args.no_install_recommends:
                operation.append("--no-install-recommends")
            if not _retry(
                options, [*operation, *args.packages], args.install_timeout, f"{args.label} install"
            ):
                print(
                    f"::error::{args.label} installation failed after {MAX_ATTEMPTS} attempts",
                    file=sys.stderr,
                )
                return 1
            return 0
        finally:
            try:
                if args.autoclean:
                    subprocess.run(["sudo", "apt-get", *options, "autoclean", "-qq"], check=False)
            finally:
                # Restore cache ownership even after failure so a retry can reuse downloads.
                subprocess.run(["sudo", "rm", "-rf", str(partial_dir)], check=True)
                subprocess.run(
                    ["sudo", "chown", "-R", f"{os.getuid()}:{os.getgid()}", str(cache_dir)],
                    check=True,
                )
    except (OSError, subprocess.CalledProcessError) as error:
        print(f"::error::apt installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
