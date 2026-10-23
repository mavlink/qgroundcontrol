#!/usr/bin/env python3
"""Install Multipass on an ephemeral Linux CI runner and wait for its daemon.

CI runs the Multipass client as root. A job process lives in the runner's own systemd
service cgroup with no user session, so for any other user `snap run` cannot move the
client into a snap scope and snap-confine refuses to start it (Multipass >= 1.17).

--allow-bridge-forwarding then lets guests reach the network: Docker sets the iptables FORWARD
policy to DROP, which blocks the Multipass bridge's NAT. (libvirt adds its own rules, so the
Vagrant job does not need it.)
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from pathlib import Path

SOCKET = Path("/var/snap/multipass/common/multipass_socket")
SOCKET_ATTEMPTS = 30
CATALOG_ATTEMPTS = 15


def install() -> None:
    if not shutil.which("snap"):
        # Shared apt update: retries and RunsOn's in-region EC2 mirror.
        apt_update = [sys.executable, str(Path(__file__).with_name("apt_install_retry.py"))]
        subprocess.run([*apt_update, "--update"], check=True)
        subprocess.run(
            ["sudo", "apt-get", "install", "-y", "--allow-change-held-packages", "snapd"],
            check=True,
        )
        subprocess.run(["sudo", "systemctl", "enable", "--now", "snapd.socket"], check=True)
    subprocess.run(["sudo", "snap", "wait", "system", "seed.loaded"], check=True, timeout=300)
    subprocess.run(["sudo", "snap", "install", "multipass"], check=True)
    # Cached runner images can carry retired image manifests; refresh before launch.
    subprocess.run(["sudo", "snap", "refresh", "multipass"], check=True)
    for attempt in range(SOCKET_ATTEMPTS):
        if SOCKET.is_socket():
            break
        if attempt == SOCKET_ATTEMPTS - 1:
            raise TimeoutError(f"Multipass socket did not appear: {SOCKET}")
        time.sleep(2)
    subprocess.run(["sudo", "multipass", "version"], check=True, timeout=30)
    for attempt in range(CATALOG_ATTEMPTS):
        try:
            result = subprocess.run(
                ["sudo", "multipass", "find"],
                check=False,
                timeout=15,
                stdout=subprocess.DEVNULL,
                stderr=subprocess.DEVNULL,
            )
            if result.returncode == 0:
                return
        except subprocess.TimeoutExpired:
            pass
        if attempt < CATALOG_ATTEMPTS - 1:
            time.sleep(5)
    # find refreshes all remotes; an obsolete secondary remote need not block release:.
    print(
        "::warning::Multipass catalog refresh did not succeed; the VM launch will validate the selected release image"
    )


def allow_bridge_forwarding() -> None:
    """Accept forwarded packets and enable IPv4 forwarding on this host."""
    print("Allowing forwarded traffic for the Multipass bridge")
    subprocess.run(["sudo", "iptables", "-P", "FORWARD", "ACCEPT"], check=True, timeout=60)
    subprocess.run(["sudo", "sysctl", "-w", "net.ipv4.ip_forward=1"], check=True, timeout=60)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--allow-bridge-forwarding",
        action="store_true",
        help="After installing, set the iptables FORWARD policy to ACCEPT and enable IPv4 "
        "forwarding (changes the host firewall; for ephemeral runners)",
    )
    args = parser.parse_args(argv)
    try:
        install()
        if args.allow_bridge_forwarding:
            allow_bridge_forwarding()
        return 0
    except (OSError, subprocess.SubprocessError) as error:
        print(f"Multipass installation failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
