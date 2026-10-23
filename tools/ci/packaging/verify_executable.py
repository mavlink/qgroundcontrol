#!/usr/bin/env python3
"""Verify QGroundControl executable with a boot test.

Resolves paths, sets permissions, and runs the binary with --simple-boot-test (offscreen, or under
xvfb-run for AppImages on Linux without a display). With --type dmg, attaches the disk image,
verifies the app bundle inside it, and detaches it.
"""

from __future__ import annotations

import argparse
import contextlib
import os
import platform
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
from pathlib import Path
from typing import TYPE_CHECKING

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.cmake import read_cache_var
from common.gh_actions import gh_error, gh_warning
from common.proc import output_text, run_captured

if TYPE_CHECKING:
    from collections.abc import Iterator

BOOT_TIMEOUT_EXIT_CODE = 124
DRAIN_TIMEOUT_SECONDS = 10
TOOL_TIMEOUT_SECONDS = 120
EXE_TYPES = ("binary", "appimage", "dmg", "exe")


def _setup_gstreamer_env(build_dir: Path) -> None:
    """Read the resolved GStreamer SDK root from CMakeCache and set plugin env vars.

    For dev builds (before cmake --install), GStreamer plugins haven't been
    copied into the app bundle yet. This lets the runtime find the
    auto-downloaded SDK's plugins.
    """
    cache = build_dir / "CMakeCache.txt"
    if not cache.is_file():
        return

    value = read_cache_var(str(cache), "QGC_GST_RESOLVED_ROOT")
    if not value:
        return

    gst_root = Path(value)
    plugin_dir = gst_root / "lib" / "gstreamer-1.0"
    if not plugin_dir.is_dir():
        return

    print(f"Setting GStreamer env from build cache: {gst_root}")
    plugin_str = str(plugin_dir)
    # SYSTEM_PATH replaces the default plugin dirs; GST_PLUGIN_PATH would only rescan the same dir.
    for var in ("GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0"):
        os.environ[var] = plugin_str

    scanner = gst_root / "libexec" / "gstreamer-1.0" / "gst-plugin-scanner"
    if platform.system() == "Windows":
        scanner = scanner.with_suffix(".exe")
    if scanner.is_file() and os.access(scanner, os.X_OK):
        scanner_str = str(scanner)
        os.environ["GST_PLUGIN_SCANNER"] = scanner_str
        os.environ["GST_PLUGIN_SCANNER_1_0"] = scanner_str

    lib_dir = str(gst_root / "lib")
    if platform.system() == "Darwin":
        existing = os.environ.get("DYLD_LIBRARY_PATH", "")
        os.environ["DYLD_LIBRARY_PATH"] = f"{lib_dir}:{existing}" if existing else lib_dir
    elif platform.system() == "Windows":
        bin_dir = str(gst_root / "bin")
        existing = os.environ.get("PATH", "")
        os.environ["PATH"] = f"{bin_dir};{existing}" if existing else bin_dir


def _run_tool(cmd: list[str]) -> subprocess.CompletedProcess[str]:
    """Run a helper command; a hang becomes a failed result instead of blocking the job."""
    try:
        return run_captured(cmd, timeout=TOOL_TIMEOUT_SECONDS)
    except subprocess.TimeoutExpired:
        message = f"timed out after {TOOL_TIMEOUT_SECONDS}s"
        return subprocess.CompletedProcess(cmd, BOOT_TIMEOUT_EXIT_CODE, stdout="", stderr=message)


def _verify_archs(binary_path: Path, expected: str) -> None:
    """Fail if the Mach-O binary's architectures differ from the expected set."""
    result = _run_tool(["lipo", "-archs", str(binary_path)])
    if result.returncode != 0:
        raise RuntimeError(f"lipo -archs failed for {binary_path}: {result.stderr.strip()}")
    actual = set(result.stdout.split())
    wanted = set(expected.split())
    if actual != wanted:
        raise RuntimeError(
            f"Architecture mismatch for {binary_path}: "
            f"expected {sorted(wanted)}, got {sorted(actual)}"
        )
    print(f"Verified architectures: {' '.join(sorted(actual))}")


def _verify_appimage_update_information(binary_path: Path, expected: str) -> None:
    """Verify embedded AppImage update metadata and its zsync sidecar."""
    result = _run_tool([str(binary_path), "--appimage-updateinformation"])
    if result.returncode != 0:
        raise RuntimeError(
            f"Failed to read AppImage update information from {binary_path}: "
            f"{result.stderr.strip()}"
        )

    actual = result.stdout.strip()
    if actual != expected:
        raise RuntimeError(
            f"Unexpected AppImage update information: {actual!r} (expected {expected!r})"
        )

    zsync_path = Path(f"{binary_path}.zsync")
    if not zsync_path.is_file() or zsync_path.stat().st_size == 0:
        raise RuntimeError(f"AppImage zsync sidecar is missing or empty: {zsync_path}")

    print(f"Verified AppImage update information: {actual}")


@contextlib.contextmanager
def _attached_dmg(dmg_path: Path) -> Iterator[Path]:
    """Attach a DMG read-only at a private mount point and detach it on exit."""
    if not dmg_path.is_file():
        raise RuntimeError(f"DMG file not found: {dmg_path}")
    mount_point = Path(tempfile.mkdtemp(prefix="qgc-dmg-", dir=os.environ.get("RUNNER_TEMP")))
    attach = ["hdiutil", "attach", "-nobrowse", "-readonly", "-mountpoint", str(mount_point)]
    result = _run_tool([*attach, str(dmg_path)])
    if result.returncode != 0:
        mount_point.rmdir()
        raise RuntimeError(f"Failed to mount DMG {dmg_path}: {result.stderr.strip()}")
    print(f"Mounted {dmg_path} at {mount_point}")
    try:
        yield mount_point
    finally:
        _detach_dmg(mount_point)


def _detach_dmg(mount_point: Path) -> None:
    """Detach a mounted DMG, forcing it when processes still hold it open."""

    def detach(*options: str) -> subprocess.CompletedProcess[str]:
        return _run_tool(["hdiutil", "detach", *options, str(mount_point)])

    result = detach()
    if result.returncode != 0:
        result = detach("-force")
    if result.returncode != 0:
        gh_warning(f"Failed to detach {mount_point}: {result.stderr.strip()}")
        return
    with contextlib.suppress(OSError):
        mount_point.rmdir()
    print(f"Detached {mount_point}")


def _bundle_executable(mount_point: Path) -> Path:
    """Return the executable of the single app bundle at the root of a mounted DMG."""
    bundles = sorted(mount_point.glob("*.app"))
    if len(bundles) != 1:
        raise RuntimeError(f"Expected one app bundle in {mount_point}, found {len(bundles)}")
    executable = bundles[0] / "Contents" / "MacOS" / bundles[0].stem
    if not executable.is_file():
        raise RuntimeError(f"App bundle executable not found: {executable}")
    return executable


def _kill_process_tree(proc: subprocess.Popen[str]) -> None:
    if os.name == "nt":
        proc.kill()
        return
    with contextlib.suppress(ProcessLookupError):
        os.killpg(proc.pid, signal.SIGKILL)


def _execute(cmd: list[str], env: dict[str, str], cwd: Path, timeout: int) -> tuple[int, str]:
    """Run *cmd* and return (exit code, combined output); exit code 124 means timeout."""
    with subprocess.Popen(
        cmd,
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        encoding="utf-8",
        errors="replace",
        # Own process group so a timeout also kills QGC when it runs under xvfb-run
        start_new_session=os.name != "nt",
    ) as proc:
        try:
            stdout, stderr = proc.communicate(timeout=timeout)
        except subprocess.TimeoutExpired:
            _kill_process_tree(proc)
            try:
                stdout, stderr = proc.communicate(timeout=DRAIN_TIMEOUT_SECONDS)
            except subprocess.TimeoutExpired as e:
                stdout, stderr = output_text(e.stdout), output_text(e.stderr)
            return BOOT_TIMEOUT_EXIT_CODE, stdout + stderr
        except BaseException:
            _kill_process_tree(proc)
            raise
        return proc.returncode, stdout + stderr


def _boot_command(binary: Path, *, headless: bool) -> tuple[list[str], dict[str, str]]:
    """Return the boot-test command and environment, adding a virtual display when needed."""
    cmd = [str(binary), "--simple-boot-test"]
    env = os.environ.copy()
    if headless:
        env.setdefault("QT_QPA_PLATFORM", "offscreen")
        print("Platform: offscreen (forced)")
    elif platform.system() not in ("Darwin", "Windows") and not os.environ.get("DISPLAY"):
        xvfb = shutil.which("xvfb-run")
        if xvfb:
            print("Running with xvfb-run (no DISPLAY)")
            return [xvfb, "-a", *cmd], env
        gh_warning("xvfb-run not found, using offscreen platform")
        env["QT_QPA_PLATFORM"] = "offscreen"
    return cmd, env


def _boot_test(binary: Path, *, cwd: Path, timeout: int, headless: bool) -> int:
    """Run QGroundControl --simple-boot-test and return its exit code (124 on timeout)."""
    if not binary.is_file():
        gh_error(f"Binary not found: {binary}")
        return 1
    print(f"Binary: {binary}")
    print(f"Timeout: {timeout}s")
    cmd, env = _boot_command(binary, headless=headless)
    exit_code, output = _execute(cmd, env, cwd, timeout)
    print(output)
    if exit_code == BOOT_TIMEOUT_EXIT_CODE:
        gh_error(f"Boot test timed out after {timeout}s")
        return exit_code
    if exit_code == 0:
        print("Boot test passed")
    else:
        gh_error(f"Boot test failed (exit code: {exit_code})")
    return exit_code


def _verify(binary_path: Path, args: argparse.Namespace) -> int:
    """Boot-test binary_path and return the boot test exit code."""
    work_dir = Path(args.working_dir) if args.working_dir else binary_path.parent

    if not work_dir.is_dir():
        gh_error(f"Working directory not found: {work_dir}")
        return 1

    if args.build_dir:
        _setup_gstreamer_env(Path(args.build_dir))

    if args.expect_archs and platform.system() == "Darwin":
        try:
            _verify_archs(binary_path, args.expect_archs)
        except RuntimeError as error:
            gh_error(str(error))
            return 1

    binary_name = binary_path.name
    headless = True

    # AppImages typically don't ship the "offscreen" Qt platform plugin.
    if args.exe_type == "appimage" or binary_name.endswith(".AppImage"):
        headless = False

    exe = work_dir / binary_name
    if os.name != "nt":
        with contextlib.suppress(OSError):
            exe.chmod(exe.stat().st_mode | stat.S_IEXEC)
    run_binary = exe.resolve()

    if args.expected_appimage_update_information:
        try:
            _verify_appimage_update_information(
                run_binary, args.expected_appimage_update_information
            )
        except RuntimeError as error:
            gh_error(str(error))
            return 1

    return _boot_test(run_binary, cwd=work_dir, timeout=args.timeout, headless=headless)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--binary-path", required=True, help="Executable, AppImage, or DMG")
    parser.add_argument("--working-dir", default="")
    parser.add_argument("--build-dir", default="")
    parser.add_argument("--type", choices=EXE_TYPES, default="binary", dest="exe_type")
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--expect-archs", default="")
    parser.add_argument("--expected-appimage-update-information", default="")
    args = parser.parse_args(argv)

    if args.exe_type != "dmg":
        return _verify(Path(args.binary_path), args)

    try:
        with _attached_dmg(Path(args.binary_path)) as mount_point:
            return _verify(_bundle_executable(mount_point), args)
    except RuntimeError as error:
        gh_error(str(error))
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
