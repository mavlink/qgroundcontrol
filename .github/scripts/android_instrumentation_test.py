#!/usr/bin/env python3
"""Install and run Android instrumentation (androidTest) tests for QGroundControl.

Invoked as a single line from the emulator-runner ``script:`` block, which
executes each script line as its own ``sh -c`` and therefore cannot host a
multi-line shell ``if``/``fi`` block. A no-op (exit 0) when ``--androidtest-apk``
is empty so the caller needs no shell conditional.
"""

from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.proc import run_bytes

# STATUS_CODE -1/-2 are per-test error/failure; session `INSTRUMENTATION_CODE: -1` is RESULT_OK — don't match it.
_FAILURE_PATTERN = re.compile(
    r"^INSTRUMENTATION_STATUS_CODE: -[12]\b|FAILURES!!!|Process crashed",
    re.MULTILINE,
)
_PASS_PATTERN = re.compile(r"^OK \([1-9][0-9]* test", re.MULTILINE)
# Also matches clean native System.exit (no Java exception), which `am instrument` reports as a crash.
_FATAL_LINE = re.compile(
    r"FATAL EXCEPTION|SIGSEGV|SIGABRT|UnsatisfiedLinkError|abort message|"
    r"VM exiting with result|System\.exit called|Crash of app"
)


def _decode(value: bytes) -> str:
    return value.decode("utf-8", errors="replace")


def _capture_crash_logcat(package: str) -> str:
    """Drain logcat now — the emulator often goes offline before the diagnostics step runs."""
    sections: list[str] = []

    crash = _decode(run_bytes(["adb", "logcat", "-b", "crash", "-d", "-v", "time"]).stdout).strip()
    if crash:
        sections.append("===== crash buffer =====\n" + crash)

    app_tag = package.rsplit(".", 1)[
        -1
    ]  # logcat truncates the process name to e.g. ".qgroundcontrol"
    lines = _decode(run_bytes(["adb", "logcat", "-d", "-v", "time"]).stdout).splitlines()
    relevant = [ln for ln in lines if app_tag in ln or _FATAL_LINE.search(ln)]
    if relevant:
        sections.append(
            f"===== main buffer ({app_tag} + fatal) =====\n" + "\n".join(relevant[-200:])
        )

    return "\n\n".join(sections)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Run Android instrumentation tests for QGroundControl."
    )
    parser.add_argument(
        "--androidtest-apk",
        default="",
        help="Path to the androidTest APK. Empty string is a no-op (exit 0).",
    )
    parser.add_argument(
        "--package",
        default="org.mavlink.qgroundcontrol",
        help="Android application package id under test",
    )
    parser.add_argument(
        "--runner",
        default="org.mavlink.qgroundcontrol.QGCTestRunner",
        help="Instrumentation test runner class",
    )
    parser.add_argument(
        "--timeout",
        type=int,
        default=600,
        help="Timeout in seconds for the am instrument run",
    )
    parser.add_argument(
        "--log-output",
        type=Path,
        default=Path("/tmp/qgc_instrumentation.log"),
        help="Path to store full am instrument output",
    )
    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if not args.androidtest_apk:
        print("::notice::No instrumentation APK provided; skipping androidTest run.")
        return 0

    print(f"Installing instrumentation APK: {args.androidtest_apk}")
    install = run_bytes(["adb", "install", "-r", args.androidtest_apk])
    if install.returncode != 0:
        print(
            f"::error::adb install of instrumentation APK failed: {_decode(install.stderr).strip()}"
        )
        return 1

    run_bytes(["adb", "shell", "am", "force-stop", args.package])
    run_bytes(["adb", "logcat", "-c"])

    target = f"{args.package}.test/{args.runner}"
    print(f"Running instrumentation: {target}")
    result = run_bytes(
        ["adb", "shell", "am", "instrument", "-w", "-r", target],
        timeout=args.timeout,
    )
    output = _decode(result.stdout) + _decode(result.stderr)
    print(output)

    failed = bool(_FAILURE_PATTERN.search(output))
    passed = bool(_PASS_PATTERN.search(output))

    if failed or not passed:
        crash = _capture_crash_logcat(args.package)
        if crash:
            output += "\n\n===== logcat (crash excerpt) =====\n" + crash
            print("::group::logcat crash excerpt")
            print(crash)
            print("::endgroup::")

    args.log_output.write_text(output, encoding="utf-8")

    if failed:
        print("::error::Android instrumentation tests failed.")
        return 1
    if not passed:
        print(
            "::error::Android instrumentation tests did not report a passing run with at least one test."
        )
        return 1

    print("Android instrumentation tests passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
