#!/usr/bin/env python3
"""Install Multipass on an ephemeral Linux CI runner and wait for its daemon."""

from __future__ import annotations

import shutil
import subprocess
import sys
import time
from pathlib import Path

SOCKET = Path("/var/snap/multipass/common/multipass_socket")


def install() -> None:
    if not shutil.which("snap"):
        subprocess.run(["sudo", "apt-get", "update"], check=True)
        subprocess.run(
            ["sudo", "apt-get", "install", "-y", "--allow-change-held-packages", "snapd"],
            check=True,
        )
        subprocess.run(["sudo", "systemctl", "enable", "--now", "snapd.socket"], check=True)
    subprocess.run(["sudo", "snap", "wait", "system", "seed.loaded"], check=True, timeout=300)
    subprocess.run(["sudo", "snap", "install", "multipass"], check=True)
    # Cached runner images can carry retired image manifests; refresh before launch.
    subprocess.run(["sudo", "snap", "refresh", "multipass"], check=True)
    for attempt in range(30):
        if SOCKET.is_socket():
            break
        if attempt == 29:
            raise TimeoutError(f"Multipass socket did not appear: {SOCKET}")
        time.sleep(2)
    subprocess.run(["sudo", "chmod", "a+rw", str(SOCKET)], check=True)
    subprocess.run(["multipass", "version"], check=True, timeout=30)
    for attempt in range(30):
        try:
            result = subprocess.run(
                ["multipass", "find"],
                check=False,
                timeout=15,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            if result.returncode == 0:
                return
        except subprocess.TimeoutExpired:
            pass
        if attempt < 29:
            time.sleep(5)
    # find refreshes all remotes; an obsolete secondary remote need not block release:.
    print(
        "::warning::Multipass catalog refresh did not succeed; the VM launch will validate the selected release image"
    )


def main() -> int:
    try:
        install()
        return 0
    except (OSError, subprocess.SubprocessError) as error:
        print(f"Multipass installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
