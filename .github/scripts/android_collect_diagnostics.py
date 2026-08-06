#!/usr/bin/env python3
"""Collect Android emulator failure diagnostics for upload as a CI artifact.

Copies build logs, runs adb dumps if an emulator is online, greps the logcat
for GStreamer-related errors, and includes AVD log/ini files.

Every step is best-effort — missing files or offline emulators must not abort
the collection, since the parent step already failed and this is salvage.
"""

from __future__ import annotations

import argparse
import json
import re
import shutil
import subprocess
import sys
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import gh_warning
from common.proc import run_captured

ADB_TIMEOUT_SHORT = 20
ADB_TIMEOUT_LONG = 30

GSTREAMER_LOG_PATTERN = re.compile(
    r"gst|gstreamer|libgst|GST_PLUGIN|GstVideoReceiver|qml6glsink|qgcvideosinkbin",
    re.IGNORECASE,
)
GSTREAMER_LOAD_ERROR_PATTERN = re.compile(
    r"dlopen.*gst|UnsatisfiedLinkError.*gst|cannot.*load.*gst",
    re.IGNORECASE,
)


def _run(cmd: list[str], timeout: int) -> subprocess.CompletedProcess[str]:
    """Run a command swallowing failures — diagnostics are salvage, not gates."""
    try:
        return run_captured(cmd, timeout=timeout)
    except (FileNotFoundError, subprocess.TimeoutExpired) as e:
        return subprocess.CompletedProcess(cmd, returncode=1, stdout="", stderr=str(e))


def _copy_if_exists(src: Path, dst: Path) -> bool:
    if not src.exists():
        return False
    try:
        shutil.copy2(src, dst)
        return True
    except OSError as e:
        gh_warning(f"Failed to copy {src} -> {dst}: {e}")
        return False


def collect_build_artifacts(out_dir: Path, build_dir: Path, boot_log: Path | None) -> None:
    """Copy build / boot / deployment logs into out_dir."""
    if boot_log is not None:
        _copy_if_exists(boot_log, out_dir / boot_log.name)

    for name in (
        "qgc-build.log",
        "qgc-build-retry.log",
        "android-QGroundControl-deployment-settings.json",
    ):
        _copy_if_exists(build_dir / name, out_dir / name)

    # Pretty-print the deployment-settings JSON for human review; keep going on parse error.
    settings = out_dir / "android-QGroundControl-deployment-settings.json"
    pretty = out_dir / "android-QGroundControl-deployment-settings.pretty.json"
    error = out_dir / "android-QGroundControl-deployment-settings.error.txt"
    if settings.exists():
        try:
            data = json.loads(settings.read_text(encoding="utf-8"))
            pretty.write_text(json.dumps(data, indent=2, sort_keys=True) + "\n", encoding="utf-8")
        except (json.JSONDecodeError, OSError) as e:
            error.write_text(str(e), encoding="utf-8")


def adb_available() -> bool:
    return shutil.which("adb") is not None


def emulator_online(devices_output: str) -> bool:
    return any(re.match(r"^emulator-\d+\s+device$", line) for line in devices_output.splitlines())


def collect_adb_diagnostics(out_dir: Path) -> None:
    """Run adb dumps if an emulator device is online; otherwise note why we skipped."""
    if not adb_available():
        (out_dir / "adb-skipped.txt").write_text(
            "adb not found; skipping adb diagnostics.\n", encoding="utf-8"
        )
        return

    _run(["adb", "start-server"], ADB_TIMEOUT_SHORT)
    devices = _run(["adb", "devices"], ADB_TIMEOUT_SHORT)
    devices_path = out_dir / "adb-devices.txt"
    devices_path.write_text((devices.stdout or "") + (devices.stderr or ""), encoding="utf-8")

    if not emulator_online(devices.stdout):
        (out_dir / "adb-skipped.txt").write_text(
            "No online emulator device found; skipping adb diagnostics.\n",
            encoding="utf-8",
        )
        return

    for cmd, fname in [
        (["adb", "-e", "logcat", "-d"], "adb-logcat-full.txt"),
        (["adb", "-e", "shell", "dumpsys", "activity"], "dumpsys-activity.txt"),
        (["adb", "-e", "shell", "getprop"], "getprop.txt"),
    ]:
        result = _run(cmd, ADB_TIMEOUT_LONG)
        (out_dir / fname).write_text(result.stdout, encoding="utf-8")

    extract_gstreamer_diagnostics(out_dir / "adb-logcat-full.txt", out_dir)


def extract_gstreamer_diagnostics(logcat_path: Path, out_dir: Path) -> None:
    """Filter the full logcat for GStreamer-relevant lines."""
    if not logcat_path.exists():
        return
    text = logcat_path.read_text(encoding="utf-8", errors="replace")
    lines = text.splitlines()
    gst_lines = [line for line in lines if GSTREAMER_LOG_PATTERN.search(line)]
    load_errors = [line for line in lines if GSTREAMER_LOAD_ERROR_PATTERN.search(line)]
    (out_dir / "gstreamer-logcat.txt").write_text("\n".join(gst_lines) + "\n", encoding="utf-8")
    (out_dir / "gstreamer-load-errors.txt").write_text(
        "\n".join(load_errors) + "\n", encoding="utf-8"
    )


def collect_avd_files(out_dir: Path, avd_home: Path) -> None:
    """Copy .log/.ini files under the AVD home into out_dir, preserving structure."""
    if not avd_home.is_dir():
        return
    for path in avd_home.rglob("*"):
        if not path.is_file() or path.suffix not in (".log", ".ini"):
            continue
        rel = path.relative_to(avd_home.parent)  # keep .android/avd/... prefix
        target = out_dir / rel
        target.parent.mkdir(parents=True, exist_ok=True)
        _copy_if_exists(path, target)


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--out-dir",
        type=Path,
        required=True,
        help="Directory to write diagnostics into (created if missing)",
    )
    parser.add_argument(
        "--build-dir",
        type=Path,
        required=True,
        help="CMake build directory to pull qgc-build*.log and deployment-settings.json from",
    )
    parser.add_argument(
        "--boot-log",
        type=Path,
        default=None,
        help="Path to the emulator boot test log (e.g. /tmp/qgc_emulator_boot.log)",
    )
    parser.add_argument(
        "--avd-home",
        type=Path,
        default=Path.home() / ".android" / "avd",
        help="AVD home directory (default: ~/.android/avd)",
    )
    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)
    args.out_dir.mkdir(parents=True, exist_ok=True)
    collect_build_artifacts(args.out_dir, args.build_dir, args.boot_log)
    collect_adb_diagnostics(args.out_dir)
    collect_avd_files(args.out_dir, args.avd_home)
    print(f"Diagnostics written to {args.out_dir}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
