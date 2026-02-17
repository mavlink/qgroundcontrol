#!/usr/bin/env python3
"""Run Android emulator boot smoke test for QGroundControl."""

from __future__ import annotations

import argparse
import re
import subprocess
import sys
import time
from pathlib import Path


def _decode(value: bytes) -> str:
    return value.decode("utf-8", errors="replace")


def run_command(cmd: list[str], check: bool = False) -> subprocess.CompletedProcess[bytes]:
    result = subprocess.run(cmd, capture_output=True)
    if check and result.returncode != 0:
        stdout = _decode(result.stdout)
        stderr = _decode(result.stderr)
        raise RuntimeError(
            f"Command failed ({result.returncode}): {' '.join(cmd)}\n"
            f"stdout:\n{stdout}\n"
            f"stderr:\n{stderr}"
        )
    return result


def wait_for_adb_ready(timeout: int) -> bool:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = run_command(["adb", "get-state"])
        if state.returncode == 0 and _decode(state.stdout).strip() == "device":
            boot_completed = run_command(["adb", "shell", "getprop", "sys.boot_completed"])
            if boot_completed.returncode == 0 and _decode(boot_completed.stdout).strip() == "1":
                return True
        time.sleep(2)
    return False


def install_with_retries(apk_path: Path, retries: int, retry_delay: int) -> bool:
    for attempt in range(1, retries + 1):
        result = run_command(["adb", "install", "-r", str(apk_path)])
        if result.returncode == 0:
            return True

        stdout = _decode(result.stdout).strip()
        stderr = _decode(result.stderr).strip()
        print(
            f"::warning::adb install attempt {attempt}/{retries} failed."
            f" stdout={stdout!r} stderr={stderr!r}"
        )
        if attempt < retries:
            time.sleep(retry_delay)
    return False


def read_logcat() -> str:
    result = run_command(["adb", "logcat", "-d"])
    if result.returncode != 0:
        return ""
    return _decode(result.stdout)


def write_log(path: Path, content: str) -> None:
    path.write_text(content, encoding="utf-8")


def print_log_group(content: str, pattern: re.Pattern[str], max_lines: int = 80) -> None:
    matches = [line for line in content.splitlines() if pattern.search(line)]
    print("::group::Application logcat")
    for line in matches[-max_lines:]:
        print(line)
    print("::endgroup::")


def app_running(package_name: str) -> bool:
    # Prefer pidof for speed, but fall back to ps parsing because some images
    # expose app processes with a suffix (for example ":qt") or missing pidof.
    for process_name in (package_name, f"{package_name}:qt"):
        result = run_command(["adb", "shell", "pidof", process_name])
        if result.returncode == 0 and _decode(result.stdout).strip():
            return True

    ps_result = run_command(["adb", "shell", "ps", "-A"])
    if ps_result.returncode != 0:
        return False

    package_prefix = f"{package_name}:"
    for line in _decode(ps_result.stdout).splitlines():
        if package_name in line or package_prefix in line:
            return True
    return False


def emit_failure(
    message: str,
    log_output: Path,
    log_pattern: re.Pattern[str],
    notice: str | None = None,
) -> int:
    print(f"::error::{message}")
    if notice:
        print(f"::notice::{notice}")
    logcat_content = read_logcat()
    write_log(log_output, logcat_content)
    print_log_group(logcat_content, log_pattern)
    return 1


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run Android emulator boot smoke test for QGroundControl."
    )
    parser.add_argument("--apk", required=True, type=Path, help="Path to APK file")
    parser.add_argument(
        "--package",
        default="org.mavlink.qgroundcontrol",
        help="Android package name",
    )
    parser.add_argument("--timeout", type=int, default=90, help="Timeout in seconds")
    parser.add_argument(
        "--stability-window",
        type=int,
        default=20,
        help="Seconds app must remain alive after launch",
    )
    parser.add_argument(
        "--log-output",
        type=Path,
        default=Path("/tmp/qgc_emulator_boot.log"),
        help="Path to store full logcat output",
    )
    parser.add_argument(
        "--adb-ready-timeout",
        type=int,
        default=120,
        help="Timeout in seconds for adb device and boot readiness",
    )
    parser.add_argument(
        "--install-retries",
        type=int,
        default=3,
        help="Number of retries for adb install",
    )
    parser.add_argument(
        "--install-retry-delay",
        type=int,
        default=4,
        help="Delay in seconds between adb install retries",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    package_escaped = re.escape(args.package)
    app_launch_pattern = re.compile(
        rf"(ActivityTaskManager|ActivityManager): (Displayed|Start proc).+{package_escaped}",
        re.IGNORECASE,
    )
    crash_signature_pattern = re.compile(
        r"FATAL EXCEPTION|Fatal signal|UnsatisfiedLinkError|dlopen failed",
        re.IGNORECASE,
    )
    crash_context_pattern = re.compile(
        rf"{package_escaped}|org\.qtproject|qtMainLoopThread|QGroundControl",
        re.IGNORECASE,
    )
    app_log_pattern = re.compile(
        rf"{package_escaped}|QGroundControl|qtMainLoopThread|org\.qtproject|qt\.qml|SDL",
        re.IGNORECASE,
    )
    crash_log_pattern = re.compile(
        rf"{package_escaped}|QGroundControl|qtMainLoopThread|org\.qtproject|qt\.qml|SDL|FATAL|Fatal|dlopen",
        re.IGNORECASE,
    )

    print(
        "Waiting for emulator boot completion "
        f"(timeout: {args.adb_ready_timeout}s) before install..."
    )
    if not wait_for_adb_ready(args.adb_ready_timeout):
        return emit_failure(
            f"Emulator did not reach ready state within {args.adb_ready_timeout}s",
            args.log_output,
            app_log_pattern,
        )

    if not install_with_retries(args.apk, args.install_retries, args.install_retry_delay):
        return emit_failure(
            f"adb install failed after {args.install_retries} attempts",
            args.log_output,
            app_log_pattern,
        )

    run_command(["adb", "shell", "am", "force-stop", args.package])
    run_command(["adb", "logcat", "-c"])
    run_command(
        [
            "adb",
            "shell",
            "monkey",
            "-p",
            args.package,
            "-c",
            "android.intent.category.LAUNCHER",
            "1",
        ],
        check=True,
    )

    print(f"Waiting for boot test to complete (timeout: {args.timeout}s)...")
    app_launched = False
    app_launched_at = 0
    app_seen_running_once = False
    consecutive_not_running = 0
    boot_test_passed = False

    for second in range(1, args.timeout + 1):
        logcat_content = read_logcat()
        saw_app_logs = bool(app_log_pattern.search(logcat_content))

        if "Simple boot test completed" in logcat_content:
            print(f"Boot test completed successfully in {second}s")
            boot_test_passed = True
            break

        is_running = app_running(args.package)
        if is_running:
            app_seen_running_once = True
            consecutive_not_running = 0

        launch_detected_in_log = bool(app_launch_pattern.search(logcat_content))
        if not app_launched and (launch_detected_in_log or is_running or saw_app_logs):
            app_launched = True
            app_launched_at = second
            if launch_detected_in_log:
                launch_source = "logcat marker"
            elif is_running:
                launch_source = "running process"
            else:
                launch_source = "application logs"
            print(
                f"App launch detected ({launch_source}); waiting for "
                f"{args.stability_window}s stability window or simple boot completion marker."
            )

        has_relevant_crash = any(
            crash_signature_pattern.search(line)
            and crash_context_pattern.search(line)
            for line in logcat_content.splitlines()
        )
        if has_relevant_crash:
            return emit_failure(
                "Crash detected during boot test",
                args.log_output,
                crash_log_pattern,
            )

        if app_launched:
            if app_seen_running_once and not is_running:
                consecutive_not_running += 1
            if app_seen_running_once and consecutive_not_running >= 5:
                return emit_failure(
                    "App process exited during stability check",
                    args.log_output,
                    crash_log_pattern,
                )

            if second - app_launched_at >= args.stability_window:
                print(
                    "Boot test passed: app remained running for "
                    f"{args.stability_window}s after launch."
                )
                boot_test_passed = True
                break

        if second == args.timeout:
            notice = None
            if app_launched:
                notice = "App launched but did not satisfy the stability/marker checks."
            return emit_failure(
                f"Boot test timed out after {args.timeout}s",
                args.log_output,
                app_log_pattern,
                notice=notice,
            )

        time.sleep(1)

    if not boot_test_passed:
        return emit_failure(
            "Boot test did not reach a passing condition",
            args.log_output,
            app_log_pattern,
        )

    final_log = read_logcat()
    write_log(args.log_output, final_log)
    print_log_group(final_log, app_log_pattern)
    print(f"Emulator boot test passed for {args.package}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
