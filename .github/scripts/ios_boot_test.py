#!/usr/bin/env python3
"""Boot the built QGC app in a disposable iOS simulator and verify its smoke test."""

from __future__ import annotations

import argparse
import json
import plistlib
import subprocess
from pathlib import Path
from typing import Any


def simctl(*args: str, timeout: int = 120, check: bool = True) -> str:
    print(f"Running simctl {' '.join(args)} (timeout: {timeout}s)", flush=True)
    result = subprocess.run(
        ["xcrun", "simctl", *args], capture_output=True, text=True, timeout=timeout, check=check
    )
    return result.stdout + result.stderr


def select_device(devices: dict[str, Any]) -> tuple[str, str]:
    for runtime, candidates in sorted(devices["devices"].items(), reverse=True):
        if ".iOS-" not in runtime:
            continue
        for device in candidates:
            if device.get("isAvailable") and device["name"].startswith("iPhone"):
                return runtime, device["deviceTypeIdentifier"]
    raise RuntimeError(
        "No available iPhone simulator runtime; install the pinned Xcode iOS runtime"
    )


def boot_test(app: Path, log: Path, *, timeout: int = 300) -> None:
    if timeout <= 0:
        raise ValueError("Boot timeout must be positive")
    with (app / "Info.plist").open("rb") as stream:
        bundle = plistlib.load(stream)["CFBundleIdentifier"]
    runtime, device_type = select_device(
        json.loads(simctl("list", "devices", "available", "--json"))
    )
    device = simctl("create", "QGC CI boot test", device_type, runtime).strip()
    try:
        simctl("boot", device)
        simctl("bootstatus", device, "-b", timeout=300)
        simctl("install", device, str(app), timeout=timeout)
        output = simctl(
            "launch",
            "--console",
            "--terminate-running-process",
            device,
            bundle,
            "--simple-boot-test",
            "--logging",
            "Main",
            "--log-output",
            timeout=timeout,
        )
        log.write_text(output, encoding="utf-8")
        print(output)
        if "Simple boot test completed" not in output or "Simple boot test failed" in output:
            raise RuntimeError("QGC did not report a successful simple boot test")
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
        output = (
            (exc.stdout or b"") if isinstance(exc.stdout, bytes) else (exc.stdout or "").encode()
        )
        error = (
            (exc.stderr or b"") if isinstance(exc.stderr, bytes) else (exc.stderr or "").encode()
        )
        log.write_bytes(output + error)
        raise
    finally:
        try:
            simctl("shutdown", device, check=False)
        finally:
            simctl("delete", device, check=False)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument(
        "--timeout",
        type=int,
        default=300,
        help="Deadline for each app installation and startup phase in seconds (default: 300)",
    )
    args = parser.parse_args()
    boot_test(args.app, args.log, timeout=args.timeout)


if __name__ == "__main__":
    main()
