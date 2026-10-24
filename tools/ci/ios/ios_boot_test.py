#!/usr/bin/env python3
"""Boot the built QGC app in a disposable iOS simulator and verify its smoke test.

Simulator failures (creating, booting, installing, or a launch that never starts QGC) are retried
on a fresh device; QGC failures are not. A cleanup failure after a passing test only warns. Every
failed attempt saves the simulator's and CoreSimulator's recent logs and QGC's crash reports beside
the log. A failure ends in an error annotation naming the phase and attempt, and every run writes a
Boot test job-summary section.
"""

from __future__ import annotations

import argparse
import json
import os
import plistlib
import re
import shutil
import subprocess
import time
import traceback
from dataclasses import dataclass
from pathlib import Path
from typing import Any

DEFAULT_APP_TIMEOUT = 600
DEFAULT_BOOT_TIMEOUT = 600
# The workflow step stops after 30 minutes; the rest is left for diagnostics, cleanup, and the summary.
DEFAULT_TOTAL_TIMEOUT = 1500
MAX_BOOT_TIMEOUT = 900
MAX_BOOT_ATTEMPTS = 2
DIAGNOSTIC_TIMEOUT = 30
LOG_SHOW_TIMEOUT = 60
CLEANUP_TIMEOUT = 60
LOG_WINDOW = "10m"
SUMMARY_TAIL_LINES = 40
CRASH_REPORT_DIR = Path("~/Library/Logs/DiagnosticReports")
SIMULATOR_LOG_PREDICATE = (
    'process BEGINSWITH "QGroundControl" OR process == "SpringBoard" OR process == "installd"'
    ' OR subsystem BEGINSWITH "com.apple.CoreSimulator"'
)
HOST_LOG_PREDICATE = (
    'subsystem BEGINSWITH "com.apple.CoreSimulator" OR process BEGINSWITH "CoreSimulator"'
)


@dataclass
class Progress:
    """How far the boot test got, for its error annotation and job summary."""

    phase: str = "setup"
    attempt: int = 0
    attempts: int = 0
    simulator: str = ""


def _record(log: Path, message: str) -> None:
    print(message, flush=True)
    with log.open("a", encoding="utf-8") as stream:
        stream.write(message)
        if not message.endswith("\n"):
            stream.write("\n")


def _annotate(level: str, message: str) -> None:
    # Workflow commands end at a newline, so percent-encode line breaks (and % itself).
    escaped = message.replace("%", "%25").replace("\r", "%0D").replace("\n", "%0A")
    print(f"::{level}::{escaped}", flush=True)


def _warn(log: Path, message: str) -> None:
    _record(log, message)
    _annotate("warning", message)


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


def _save_output(log: Path, command: list[str], path: Path) -> None:
    """Run a diagnostic *command* into *path*, keeping its bulk out of the log."""
    _record(log, f"Saving {' '.join(command[:4])} ... to {path.name}")
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            errors="replace",
            timeout=LOG_SHOW_TIMEOUT,
            check=False,
        )
        output = _decode(result.stdout) + _decode(result.stderr)
        status = f"exit code {result.returncode}"
    except subprocess.TimeoutExpired as exc:
        output = _decode(exc.stdout) + _decode(exc.stderr)
        status = f"timed out after {LOG_SHOW_TIMEOUT}s"
    except OSError as exc:
        _record(log, f"{command[0]} failed: {exc}")
        return
    path.write_text(output, encoding="utf-8")
    _record(log, f"Saved {path.name} ({status}, {len(output.splitlines())} lines)")


def _copy_crash_reports(log: Path, name: str, since: float) -> None:
    """Copy the app's crash reports written since *since* (the simulator's apps run on the host)."""
    reports_dir = CRASH_REPORT_DIR.expanduser()
    try:
        reports = sorted(
            path
            for suffix in (".ips", ".crash")
            for path in reports_dir.glob(f"{name}*{suffix}")
            if path.stat().st_mtime >= since
        )
        for report in reports:
            shutil.copy2(report, log.with_name(f"{log.stem}-{report.name}"))
            _record(log, f"Saved crash report {report.name}")
    except OSError as exc:
        _record(log, f"Crash report collection failed: {exc}")


def _log_show(predicate: str) -> list[str]:
    return ["log", "show", "--last", LOG_WINDOW, "--style", "compact", "--predicate", predicate]


def _collect_diagnostics(
    log: Path, name: str, device: str | None, attempt: int, since: float
) -> None:
    """Save simulator state, the last minutes of simulator and CoreSimulator logs, crash reports."""
    prefix = f"{log.stem}-attempt{attempt}"
    if device is not None:
        _record(log, f"Simulator state before cleanup of {device}:")
        try:
            simctl(log, "list", "devices", "--json", timeout=DIAGNOSTIC_TIMEOUT)
        except (subprocess.CalledProcessError, subprocess.TimeoutExpired, OSError):
            _record(log, "Simulator state collection failed; retaining the original failure")
        _save_output(
            log,
            ["xcrun", "simctl", "spawn", device, *_log_show(SIMULATOR_LOG_PREDICATE)],
            log.with_name(f"{prefix}-simulator.log"),
        )
    _save_output(log, _log_show(HOST_LOG_PREDICATE), log.with_name(f"{prefix}-coresimulator.log"))
    _copy_crash_reports(log, name, since)


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


def _is_simulator_failure(phase: str, failure: Exception) -> bool:
    if not isinstance(failure, (subprocess.CalledProcessError, subprocess.TimeoutExpired)):
        return False
    if phase in ("boot", "install"):
        return True
    # QGC logs within a second of starting, so a silent launch timeout means it never ran.
    return (
        isinstance(failure, subprocess.TimeoutExpired)
        and not (_decode(failure.stdout) + _decode(failure.stderr)).strip()
    )


def _validate_options(
    timeout: int,
    boot_timeout: int,
    boot_attempts: int,
    total_timeout: int = DEFAULT_TOTAL_TIMEOUT,
) -> None:
    if timeout <= 0:
        raise ValueError("App timeout must be positive")
    if not 1 <= boot_timeout <= MAX_BOOT_TIMEOUT:
        raise ValueError(f"Simulator boot timeout must be between 1 and {MAX_BOOT_TIMEOUT}")
    if not 1 <= boot_attempts <= MAX_BOOT_ATTEMPTS:
        raise ValueError(f"Simulator boot attempts must be between 1 and {MAX_BOOT_ATTEMPTS}")
    if total_timeout <= 0:
        raise ValueError("Total timeout must be positive")


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
    timeout: int = DEFAULT_APP_TIMEOUT,
    boot_timeout: int = DEFAULT_BOOT_TIMEOUT,
    boot_attempts: int = MAX_BOOT_ATTEMPTS,
    total_timeout: int = DEFAULT_TOTAL_TIMEOUT,
    progress: Progress | None = None,
) -> None:
    """Run the smoke test; *total_timeout* caps every attempt's simulator commands together."""
    _validate_options(timeout, boot_timeout, boot_attempts, total_timeout)
    progress = progress if progress is not None else Progress()
    progress.attempts = boot_attempts
    deadline = time.monotonic() + total_timeout

    def within_budget(limit: int) -> int:
        return max(1, min(limit, int(deadline - time.monotonic())))

    with (app / "Info.plist").open("rb") as stream:
        bundle = plistlib.load(stream)["CFBundleIdentifier"]
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_text("", encoding="utf-8")
    runtime, device_type = select_device(
        json.loads(simctl(log, "list", "devices", "available", "--json", stdout_only=True))
    )
    progress.simulator = f"{device_type.rsplit('.', 1)[-1]} ({runtime.rsplit('.', 1)[-1]})"
    for attempt in range(1, boot_attempts + 1):
        _record(log, f"Simulator boot attempt {attempt}/{boot_attempts}: {runtime} ({device_type})")
        progress.attempt = attempt
        progress.phase = phase = "boot"
        started = time.time()
        device: str | None = None
        failure: Exception | None = None
        cleanup_failure: Exception | None = None
        try:
            device = simctl(
                log,
                "create",
                "QGC CI boot test",
                device_type,
                runtime,
                timeout=within_budget(120),
                stdout_only=True,
            ).strip()
            simctl(log, "boot", device, timeout=within_budget(120))
            simctl(log, "bootstatus", device, "-b", timeout=within_budget(boot_timeout))
            progress.phase = phase = "install"
            simctl(log, "install", device, str(app), timeout=within_budget(timeout))
            progress.phase = phase = "launch"
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
                timeout=within_budget(timeout),
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
            _record(
                log, f"Simulator attempt {attempt}/{boot_attempts} failed during {phase}: {exc}"
            )
            # Filesystem timestamps can trail the clock that started the attempt by up to a second.
            _collect_diagnostics(log, app.stem, device, attempt, started - 1)
        finally:
            if device is not None:
                cleanup_failure = _cleanup(log, device)

        if failure is None:
            if cleanup_failure is not None:
                _warn(
                    log,
                    f"iOS simulator cleanup failed after a successful boot test: {cleanup_failure}",
                )
            return
        if attempt == boot_attempts or not _is_simulator_failure(phase, failure):
            raise failure
        if cleanup_failure is not None:
            _record(log, "Not retrying because the failed simulator could not be deleted")
            raise failure
        if time.monotonic() >= deadline:
            _record(log, f"Not retrying because the {total_timeout}s budget is spent")
            raise failure
        _record(
            log,
            f"Retrying with a fresh simulator after the {phase} failure of attempt "
            f"{attempt}/{boot_attempts}",
        )


def _format_duration(seconds: float) -> str:
    whole = round(seconds)
    if whole < 60:
        return f"{whole}s"
    minutes, secs = divmod(whole, 60)
    return f"{minutes}m {secs:02d}s"


def _cell(value: str) -> str:
    return value.replace("|", "\\|").replace("\r", " ").replace("\n", " ")


def _summary(app: Path, log: Path, progress: Progress, result: str, elapsed: float) -> str:
    """Return the Boot test job-summary section; a failure adds the end of the boot log."""
    facts = [
        ("Result", result),
        ("App", f"`{app.name}`" if "`" not in app.name else app.name),
        ("Simulator", progress.simulator),
        ("Attempts", f"{progress.attempt}/{progress.attempts}" if progress.attempt else ""),
        ("Duration", _format_duration(elapsed)),
    ]
    rows = [f"| {name} | {_cell(value)} |" for name, value in facts if value]
    parts = ["### Boot test", "| Item | Value |\n| --- | --- |\n" + "\n".join(rows)]
    lines = log.read_text(encoding="utf-8", errors="replace").splitlines() if log.is_file() else []
    if result != "passed" and lines:
        tail = "\n".join(lines[-SUMMARY_TAIL_LINES:])
        longest = max((len(run) for run in re.findall(r"`+", tail)), default=0)
        fence = "`" * max(3, longest + 1)
        parts.append(
            f"<details><summary>Last {min(len(lines), SUMMARY_TAIL_LINES)} lines of the boot "
            f"log</summary>\n\n{fence}\n{tail}\n{fence}\n\n</details>"
        )
    return "\n\n".join(parts) + "\n\n"


def _write_summary(markdown: str) -> None:
    path = os.environ.get("GITHUB_STEP_SUMMARY")
    if path:
        with open(path, "a", encoding="utf-8") as stream:
            stream.write(markdown)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--app", type=Path, required=True)
    parser.add_argument(
        "--log",
        type=Path,
        required=True,
        help="Boot log; failed attempts save their diagnostics beside it",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=DEFAULT_APP_TIMEOUT,
        help=f"Deadline for each app installation and startup phase in seconds (default: {DEFAULT_APP_TIMEOUT})",
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
    parser.add_argument(
        "--total-timeout",
        type=int,
        default=DEFAULT_TOTAL_TIMEOUT,
        help=f"Budget for every attempt's simulator commands in seconds (default: {DEFAULT_TOTAL_TIMEOUT})",
    )
    args = parser.parse_args(argv)
    try:
        _validate_options(args.timeout, args.boot_timeout, args.boot_attempts, args.total_timeout)
    except ValueError as exc:
        parser.error(str(exc))
    progress = Progress()
    start = time.monotonic()
    try:
        boot_test(
            args.app,
            args.log,
            timeout=args.timeout,
            boot_timeout=args.boot_timeout,
            boot_attempts=args.boot_attempts,
            total_timeout=args.total_timeout,
            progress=progress,
        )
    except Exception as exc:
        traceback.print_exc()
        where = progress.phase
        if progress.attempt:
            where += f" (attempt {progress.attempt}/{progress.attempts})"
        _annotate(
            "error", f"iOS simulator boot test failed during {where}: {type(exc).__name__}: {exc}"
        )
        result = f"failed during {where}"
        _write_summary(_summary(args.app, args.log, progress, result, time.monotonic() - start))
        return 1
    _write_summary(_summary(args.app, args.log, progress, "passed", time.monotonic() - start))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
