#!/usr/bin/env python3
"""Open /dev/kvm to every user on a nested-virtualization CI runner."""

from __future__ import annotations

import argparse
import subprocess
import sys
from pathlib import Path

KVM = Path("/dev/kvm")
RULE = Path("/etc/udev/rules.d/99-kvm4all.rules")
RULE_TEXT = 'KERNEL=="kvm", GROUP="kvm", MODE="0666", OPTIONS+="static_node=kvm"\n'
UDEV_TIMEOUT = 60


def enable() -> None:
    if not KVM.exists():
        raise FileNotFoundError(f"{KVM} is missing; the runner has no nested virtualization")
    subprocess.run(
        ["sudo", "tee", str(RULE)], input=RULE_TEXT, text=True, check=True, timeout=UDEV_TIMEOUT
    )
    subprocess.run(
        ["sudo", "udevadm", "control", "--reload-rules"], check=True, timeout=UDEV_TIMEOUT
    )
    # --settle: the next step must see the new mode, not race the udev event.
    subprocess.run(
        ["sudo", "udevadm", "trigger", "--settle", "--name-match=kvm"],
        check=True,
        timeout=UDEV_TIMEOUT,
    )


def main(argv: list[str] | None = None) -> int:
    argparse.ArgumentParser(description=__doc__).parse_args(argv)
    try:
        enable()
        return 0
    except (OSError, subprocess.SubprocessError) as error:
        print(f"Enabling KVM failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
