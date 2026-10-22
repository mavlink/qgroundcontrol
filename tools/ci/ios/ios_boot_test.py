#!/usr/bin/env python3
"""Boot the built QGC app in a disposable iOS simulator and verify its smoke test."""

from __future__ import annotations

import argparse
import json
import plistlib
import subprocess
from pathlib import Path
from typing import Any

DEFAULT_BOOT_TIMEOUT = 600
MAX_BOOT_TIMEOUT = 900
MAX_BOOT_ATTEMPTS = 2
DIAGNOSTIC_TIMEOUT = 30
CLEANUP_TIMEOUT = 30


def _record(log: Path, message: str) -> None:
    print(message, flush=True)
    with log.open("a", encoding="utf-8") as stream:
        stream.write(message)
        if not message.endswith("\n"):
            stream.write("\n")


def _decode(output: str | bytes | None) -> str:
    if isinstance(output, bytes):
        return output.decode("utf-8", errors="replace")
    return output or ""


def simctl(log: Path, *args: str, timeout: int = 120, stdout_only: bool = False) -> str:
    _record(log, f"Running simctl {' '.join(args)} (timeout: {timeout}s)")
    try:
        result = subprocess.run(
            ["xcrun", "simctl", *args],
            capture_output=True,
            text=True,
            errors="replace",
            timeout=timeout,
            check=True,
        )
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as exc:
        _record(log, f"{type(exc).__name__}: {exc}")
        if isinstance(exc, (subprocess.CalledProcessError, subprocess.TimeoutExpired)):
            _record(log, _decode(exc.stdout) + _decode(exc.stderr))
        raise
    output = _decode(result.stdout) + _decode(result.stderr)
    if output:
        _record(log, output)
    return _decode(result.stdout) if stdout_only else output


def _collect_diagnostics(log: Path, device: str) -> None:
    _record(log, f"Simulator state before cleanup of {device}:")
    try:
        simctl(log, "list", "devices", "--json", timeout=DIAGNOSTIC_TIMEOUT)
    except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError):
        _record(log, "Simulator state collection failed; retaining the original boot failure")


def _cleanup(log: Path, device: str) -> Exception | None:
    for command in ("shutdown", "delete"):
        try:
            simctl(log, command, device, timeout=CLEANUP_TIMEOUT)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError) as exc:
            _record(log, f"Simulator cleanup failed: {command} {device}")
            # Deletion can finish cleanup when shutdown fails.
            if command == "delete":
                return exc
    return None


def _validate_options(timeout: int, boot_timeout: int, boot_attempts: int) -> None:
    if timeout <= 0:
        raise ValueError("App timeout must be positive")
    if not 1 <= boot_timeout <= MAX_BOOT_TIMEOUT:
        raise ValueError(f"Simulator boot timeout must be between 1 and {MAX_BOOT_TIMEOUT}")
    if not 1 <= boot_attempts <= MAX_BOOT_ATTEMPTS:
        raise ValueError(f"Simulator boot attempts must be between 1 and {MAX_BOOT_ATTEMPTS}")


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


def boot_test(
    app: Path,
    log: Path,
    *,
    timeout: int = 300,
    boot_timeout: int = DEFAULT_BOOT_TIMEOUT,
    boot_attempts: int = MAX_BOOT_ATTEMPTS,
) -> None:
    _validate_options(timeout, boot_timeout, boot_attempts)
    with (app / "Info.plist").open("rb") as stream:
        bundle = plistlib.load(stream)["CFBundleIdentifier"]
    log.write_text("", encoding="utf-8")
    runtime, device_type = select_device(
        json.loads(simctl(log, "list", "devices", "available", "--json", stdout_only=True))
    )
    for attempt in range(1, boot_attempts + 1):
        _record(log, f"Simulator boot attempt {attempt}/{boot_attempts}: {runtime} ({device_type})")
        device = simctl(
            log, "create", "QGC CI boot test", device_type, runtime, stdout_only=True
        ).strip()
        ready = False
        failure = None
        try:
            simctl(log, "boot", device)
            simctl(log, "bootstatus", device, "-b", timeout=boot_timeout)
            ready = True
            simctl(log, "install", device, str(app), timeout=timeout)
            output = simctl(
                log,
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
            if "Simple boot test completed" not in output or "Simple boot test failed" in output:
                raise RuntimeError("QGC did not report a successful simple boot test")
        except (
            subprocess.CalledProcessError,
            subprocess.TimeoutExpired,
            OSError,
            RuntimeError,
        ) as exc:
            failure = exc
            _record(log, f"Simulator attempt {attempt}/{boot_attempts} failed: {exc}")
            if not ready:
                _collect_diagnostics(log, device)
        finally:
            cleanup_failure = _cleanup(log, device)

        if failure is None:
            if cleanup_failure is not None:
                raise cleanup_failure
            return
        if (
            ready
            or attempt == boot_attempts
            or cleanup_failure is not None
            or not isinstance(failure, (subprocess.CalledProcessError, subprocess.TimeoutExpired))
        ):
            raise failure
        _record(
            log,
            f"Retrying simulator cold boot with a fresh device after attempt {attempt}/{boot_attempts}",
        )


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
    parser.add_argument(
        "--boot-timeout",
        type=int,
        default=DEFAULT_BOOT_TIMEOUT,
        help=f"Simulator bootstatus deadline in seconds (1-{MAX_BOOT_TIMEOUT}, default: {DEFAULT_BOOT_TIMEOUT})",
    )
    parser.add_argument(
        "--boot-attempts",
        type=int,
        default=MAX_BOOT_ATTEMPTS,
        help=f"Maximum fresh simulator boot attempts (1-{MAX_BOOT_ATTEMPTS}, default: {MAX_BOOT_ATTEMPTS})",
    )
    args = parser.parse_args()
    try:
        _validate_options(args.timeout, args.boot_timeout, args.boot_attempts)
    except ValueError as exc:
        parser.error(str(exc))
    boot_test(
        args.app,
        args.log,
        timeout=args.timeout,
        boot_timeout=args.boot_timeout,
        boot_attempts=args.boot_attempts,
    )


if __name__ == "__main__":
    main()
