#!/usr/bin/env python3
"""Verify QGroundControl executable with a boot test.

Resolves paths, sets permissions, and runs the binary with --simple-boot-test (offscreen, or under
xvfb-run for AppImages on Linux without a display). With --type dmg, attaches the disk image,
verifies the app bundle inside it, and detaches it.

A failed boot keeps its diagnostics in a per-package directory, named by the diagnostics-dir and
diagnostics-name step outputs: the output, stacks captured before a timeout kill, macOS crash
reports, and on Windows QGC's crash minidumps (QGC_CRASH_DUMP_DIR) with the executable's PDB.
"""

from __future__ import annotations

import argparse
import contextlib
import hashlib
import os
import platform
import re
import shutil
import signal
import stat
import subprocess
import sys
import tempfile
import time
from pathlib import Path
from typing import TYPE_CHECKING

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.cmake import read_cache_var
from common.format import format_bytes, format_duration
from common.gh_actions import (
    env_default,
    gh_error,
    gh_group,
    gh_warning,
    is_ci,
    write_github_output,
    write_step_summary,
)
from common.markdown import code_block, code_span, summary_section
from common.proc import output_text, run_captured, wait_until

if TYPE_CHECKING:
    from collections.abc import Callable, Iterator

BOOT_TIMEOUT_EXIT_CODE = 124
DRAIN_TIMEOUT_SECONDS = 10
TOOL_TIMEOUT_SECONDS = 120
STACK_TIMEOUT_SECONDS = 30
CRASH_REPORT_WAIT_SECONDS = 15
CRASH_REPORT_HEAD_LINES = 60
OUTPUT_TAIL_LINES = 40
LAST_LINE_CHARS = 300
EXE_TYPES = ("binary", "appimage", "dmg", "exe")
# Startup phases worth seeing when a boot is slow, hangs or crashes; each line carries QGC's elapsed time.
# QGC's own category filter decides debug output, so these go through --logging rather than QT_LOGGING_RULES.
BOOT_LOGGING_CATEGORIES = "API.QGCApplication,Video.VideoManager,Video.GStreamer"
SHIPPED_PLUGIN_DIR = "boot-test-gstreamer-plugins"
DIAGNOSTICS_DIR = "boot-test-diagnostics"
# Exit codes of a Windows process ended by an unhandled exception.
NTSTATUS_NAMES = {
    0x80000003: "STATUS_BREAKPOINT",
    0xC0000005: "STATUS_ACCESS_VIOLATION",
    0xC000001D: "STATUS_ILLEGAL_INSTRUCTION",
    0xC0000094: "STATUS_INTEGER_DIVIDE_BY_ZERO",
    0xC00000FD: "STATUS_STACK_OVERFLOW",
    0xC0000135: "STATUS_DLL_NOT_FOUND",
    0xC0000139: "STATUS_ENTRYPOINT_NOT_FOUND",
    0xC0000142: "STATUS_DLL_INIT_FAILED",
    0xC0000374: "STATUS_HEAP_CORRUPTION",
    0xC0000409: "STATUS_STACK_BUFFER_OVERRUN",
    0xC0000602: "STATUS_FAIL_FAST_EXCEPTION",
    0xE06D7363: "unhandled C++ exception",
}
# MiniDumpWithUnloadedModules | MiniDumpWithThreadInfo, as in QGC's own crash dumps (Platform.cc).
MINIDUMP_TYPE = 0x20 | 0x1000
PROCESS_QUERY_INFORMATION = 0x0400
PROCESS_VM_READ = 0x0010


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
    shipped = read_cache_var(str(cache), "QGC_GST_SHIPPED_PLUGINS")
    if shipped and platform.system() == "Windows":
        plugin_dir = _stage_shipped_plugins(build_dir, shipped)
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


def _stage_shipped_plugins(build_dir: Path, shipped: str) -> Path:
    """Copy the plugins the package ships into one directory, for a boot test that loads only those.

    The whole Windows SDK holds ~270 plugins: a cold registry scan of them takes ~40 s, and some the
    package doesn't ship hang or crash QGC on exit. Their DLL dependencies load from the SDK's bin/.
    """
    stage = build_dir / SHIPPED_PLUGIN_DIR
    shutil.rmtree(stage, ignore_errors=True)
    stage.mkdir(parents=True)
    plugins = [Path(path) for path in shipped.split(";") if path]
    for plugin in plugins:
        shutil.copy2(plugin, stage / plugin.name)
    print(f"Loading only the {len(plugins)} GStreamer plugins the package ships")
    return stage


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


def _describe_exit_code(code: int) -> str:
    """Return *code* with its signal or NTSTATUS name: ``-11 SIGSEGV``, ``0xC0000005 STATUS_...``."""
    if code < 0:
        try:
            return f"{code} {signal.Signals(-code).name}"
        except ValueError:
            return str(code)
    if code > 0xFF:
        status = code & 0xFFFFFFFF
        return f"0x{status:08X} {NTSTATUS_NAMES.get(status, '')}".rstrip()
    return str(code)


def _step_exit_code(code: int) -> int:
    """Return *code* when a shell can report it, else 1.

    Python 3.10 on Windows exits with -1 for a SystemExit code above 2**31, such as 0xC0000005, and
    the step's MSYS bash reports any exit status of 0xC0000000 or more except a few as 127.
    """
    return code if 0 <= code <= 0xFF else 1


def _diagnostics_location(package: Path) -> tuple[Path, str]:
    """Return the package's diagnostics directory and an artifact name unique in the workflow run.

    A job can boot-test several packages and matrix legs share the job name, so the name adds the
    runner's OS and architecture and a digest of the package path.
    """
    digest = hashlib.sha256(str(package.absolute()).encode()).hexdigest()[:8]
    parts = (
        "boot-test",
        os.environ.get("GITHUB_JOB") or "local",
        os.environ.get("RUNNER_OS") or platform.system(),
        os.environ.get("RUNNER_ARCH") or platform.machine(),
        package.name,
        digest,
        os.environ.get("GITHUB_RUN_ATTEMPT") or "1",
    )
    name = re.sub(r"[^\w.-]+", "_", "-".join(parts))
    root = Path(os.environ.get("RUNNER_TEMP") or tempfile.gettempdir()) / DIAGNOSTICS_DIR
    return root / name, name


def _prepare_diagnostics(package: Path) -> Path:
    """Create an empty diagnostics directory and name it and its artifact in the step outputs."""
    directory, name = _diagnostics_location(package)
    shutil.rmtree(directory, ignore_errors=True)
    directory.mkdir(parents=True)
    write_github_output({"diagnostics-dir": str(directory), "diagnostics-name": name})
    return directory


def _boot_pid(session: int, binary: Path) -> int:
    """Return QGC's process in a Linux boot session, where xvfb-run and AppImages wrap it.

    The AppImage runtime's own executable is the package, so the shortest executable name that
    starts the package name is QGC's; without a match the session leader is the best guess.
    """
    candidates: list[tuple[int, int]] = []
    for entry in Path("/proc").iterdir():
        if not entry.name.isdigit():
            continue
        try:
            if os.getsid(int(entry.name)) != session:
                continue
            name = Path(os.readlink(entry / "exe")).name
        except OSError:
            continue
        if name and binary.name.startswith(name):
            candidates.append((len(name), int(entry.name)))
    return min(candidates)[1] if candidates else session


def _privileged(cmd: list[str]) -> list[str]:
    """Prefix passwordless sudo in CI: Yama's ptrace_scope keeps gdb from attaching to a sibling."""
    if is_ci() and os.name != "nt" and os.geteuid() != 0 and shutil.which("sudo"):
        return ["sudo", "-n", *cmd]
    return cmd


def _stack_command(system: str, pid: int, path: Path) -> list[str] | None:
    if system == "Darwin" and shutil.which("sample"):
        return ["sample", str(pid), "3", "-file", str(path)]
    if system == "Linux" and shutil.which("gdb"):
        return ["gdb", "-batch", "-p", str(pid), "-ex", "thread apply all bt"]
    if system == "Linux" and shutil.which("eu-stack"):
        return ["eu-stack", "-p", str(pid)]
    return None


def _write_minidump(pid: int, path: Path) -> str:
    """Write a minidump of the running process *pid* with dbghelp's MiniDumpWriteDump."""
    if sys.platform != "win32":
        return "Minidumps need Windows"
    import ctypes
    import msvcrt
    from ctypes import wintypes

    kernel32 = ctypes.WinDLL("kernel32", use_last_error=True)
    dbghelp = ctypes.WinDLL("dbghelp", use_last_error=True)
    kernel32.OpenProcess.restype = wintypes.HANDLE
    kernel32.OpenProcess.argtypes = (wintypes.DWORD, wintypes.BOOL, wintypes.DWORD)
    kernel32.CloseHandle.argtypes = (wintypes.HANDLE,)
    dbghelp.MiniDumpWriteDump.restype = wintypes.BOOL
    dbghelp.MiniDumpWriteDump.argtypes = (
        wintypes.HANDLE,
        wintypes.DWORD,
        wintypes.HANDLE,
        ctypes.c_int,
        ctypes.c_void_p,
        ctypes.c_void_p,
        ctypes.c_void_p,
    )
    process = kernel32.OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, False, pid)
    if not process:
        return f"OpenProcess({pid}) failed: {ctypes.WinError(ctypes.get_last_error())}"
    try:
        with path.open("wb") as stream:
            handle = msvcrt.get_osfhandle(stream.fileno())
            written = dbghelp.MiniDumpWriteDump(
                process, pid, handle, MINIDUMP_TYPE, None, None, None
            )
            error = ctypes.get_last_error()
    finally:
        kernel32.CloseHandle(process)
    if not written:
        path.unlink(missing_ok=True)
        return f"MiniDumpWriteDump failed: {ctypes.WinError(error)}"
    return f"Wrote minidump {path} ({format_bytes(path.stat().st_size)})"


def _capture_stacks(session: int, binary: Path, diagnostics: Path) -> str:
    """Record where a hung boot test is stuck before it is killed, best effort; return the report.

    macOS samples the process, Linux prints every thread's backtrace with gdb (else eu-stack), and
    Windows writes a minidump; each stops after STACK_TIMEOUT_SECONDS.
    """
    system = platform.system()
    pid = _boot_pid(session, binary) if system == "Linux" else session
    try:
        if system == "Windows":
            return _write_minidump(pid, diagnostics / f"hang-{pid}.dmp")
        path = diagnostics / f"stacks-{pid}.txt"
        cmd = _stack_command(system, pid, path)
        if cmd is None:
            return "No stack capture tool found (sample, gdb, or eu-stack)"
        print(f"Capturing stacks of process {pid}: {' '.join(cmd)}")
        result = run_captured(_privileged(cmd), timeout=STACK_TIMEOUT_SECONDS)
        if path.is_file():
            return path.read_text(encoding="utf-8", errors="replace")
        report = result.stdout + result.stderr
        path.write_text(report, encoding="utf-8")
        return report
    except (OSError, subprocess.TimeoutExpired) as error:
        return f"Stack capture failed: {error}"


def _collect_crash_reports(binary: Path, diagnostics: Path, *, since: float, crashed: bool) -> None:
    """Copy this boot test's macOS crash reports and print the start of the newest one.

    ReportCrash writes the report a few seconds after a crash, so a signal death waits for it.
    """
    reports_dir = Path.home() / "Library" / "Logs" / "DiagnosticReports"

    def find() -> list[Path]:
        reports = [
            path
            for suffix in (".ips", ".crash")
            for path in reports_dir.glob(f"{binary.name}*{suffix}")
            if path.stat().st_mtime >= since
        ]
        return sorted(reports, key=lambda path: path.stat().st_mtime)

    try:
        reports = wait_until(
            find,
            timeout=CRASH_REPORT_WAIT_SECONDS if crashed else 0,
            interval=1,
            description="a crash report",
        )
    except TimeoutError:
        print(f"No {binary.name} crash report in {reports_dir}")
        return
    for report in reports:
        shutil.copy2(report, diagnostics / report.name)
    newest = reports[-1]
    lines = newest.read_text(encoding="utf-8", errors="replace").splitlines()
    with gh_group(f"Crash report {newest.name} (first {CRASH_REPORT_HEAD_LINES} lines)"):
        print("\n".join(lines[:CRASH_REPORT_HEAD_LINES]))


def _collect_windows_dumps(binary: Path, diagnostics: Path) -> None:
    """Keep the executable's PDB beside the minidumps that QGC or a timeout wrote."""
    dumps = sorted(diagnostics.glob("*.dmp"))
    if not dumps:
        print("QGC wrote no crash dump")
        return
    print(f"Minidumps: {', '.join(dump.name for dump in dumps)}")
    pdb = binary.with_suffix(".pdb")
    if pdb.is_file():
        shutil.copy2(pdb, diagnostics / pdb.name)
    else:
        print(f"No {pdb.name} beside {binary.name}; symbolize with the build's PDB")


def _save_failure_diagnostics(
    binary: Path, output: str, diagnostics: Path, *, exit_code: int, started: float
) -> None:
    """Keep the output, crash reports, and dumps of a failed boot for the diagnostics artifact."""
    try:
        (diagnostics / "boot-output.txt").write_text(output, encoding="utf-8")
        system = platform.system()
        if system == "Darwin":
            _collect_crash_reports(binary, diagnostics, since=started, crashed=exit_code < 0)
        elif system == "Windows":
            _collect_windows_dumps(binary, diagnostics)
    except OSError as error:
        gh_warning(f"Boot test diagnostics incomplete: {error}")


def _execute(
    cmd: list[str],
    env: dict[str, str],
    cwd: Path,
    timeout: int,
    on_timeout: Callable[[int], None] | None = None,
) -> tuple[int, str]:
    """Run *cmd* and return (exit code, combined output); exit code 124 means timeout.

    *on_timeout* gets the process ID of a hung command before it is killed.
    """
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
            try:
                if on_timeout is not None:
                    on_timeout(proc.pid)
            finally:
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
    cmd = [str(binary), "--simple-boot-test", f"--logging:{BOOT_LOGGING_CATEGORIES}"]
    env = os.environ.copy()
    # A Windows GUI app logs to the debugger rather than stderr unless forced
    env.setdefault("QT_FORCE_STDERR_LOGGING", "1")
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


def _boot_test(
    binary: Path, *, cwd: Path, timeout: int, headless: bool, diagnostics: Path
) -> tuple[int, str]:
    """Run QGroundControl --simple-boot-test; return its exit code (124 on timeout) and output."""
    if not binary.is_file():
        gh_error(f"Binary not found: {binary}")
        return 1, ""
    print(f"Binary: {binary}")
    print(f"Timeout: {timeout}s")
    cmd, env = _boot_command(binary, headless=headless)
    if platform.system() == "Windows":
        env["QGC_CRASH_DUMP_DIR"] = str(diagnostics)
    stacks: list[str] = []
    started = time.time()
    start = time.monotonic()
    exit_code, output = _execute(
        cmd,
        env,
        cwd,
        timeout,
        lambda pid: stacks.append(_capture_stacks(pid, binary, diagnostics)),
    )
    elapsed = time.monotonic() - start
    if exit_code == 0:
        with gh_group("Boot output"):
            print(output)
        print(f"Boot test passed in {elapsed:.1f}s")
        return exit_code, output
    print(output)
    for report in stacks:
        with gh_group("Stacks before the timeout kill"):
            print(report)
    # Filesystem timestamps can trail the clock that started the boot by up to a second.
    _save_failure_diagnostics(binary, output, diagnostics, exit_code=exit_code, started=started - 1)
    if exit_code == BOOT_TIMEOUT_EXIT_CODE:
        gh_error(f"Boot test timed out after {timeout}s")
        return exit_code, output
    gh_error(f"Boot test failed (exit code {_describe_exit_code(exit_code)}) after {elapsed:.1f}s")
    return exit_code, output


def _verify(binary_path: Path, args: argparse.Namespace, diagnostics: Path) -> tuple[int, str]:
    """Boot-test binary_path from its directory; return the boot test's exit code and output."""
    work_dir = binary_path.parent

    if not work_dir.is_dir():
        gh_error(f"Working directory not found: {work_dir}")
        return 1, ""

    if args.build_dir:
        _setup_gstreamer_env(Path(args.build_dir))

    if args.expect_archs and platform.system() == "Darwin":
        try:
            _verify_archs(binary_path, args.expect_archs)
        except RuntimeError as error:
            gh_error(str(error))
            return 1, ""

    # AppImages typically don't ship the "offscreen" Qt platform plugin.
    headless = args.exe_type != "appimage" and not binary_path.name.endswith(".AppImage")

    if os.name != "nt":
        with contextlib.suppress(OSError):
            binary_path.chmod(binary_path.stat().st_mode | stat.S_IEXEC)
    run_binary = binary_path.resolve()

    if args.expected_appimage_update_information:
        try:
            _verify_appimage_update_information(
                run_binary, args.expected_appimage_update_information
            )
        except RuntimeError as error:
            gh_error(str(error))
            return 1, ""

    return _boot_test(
        run_binary,
        cwd=work_dir,
        timeout=args.timeout,
        headless=headless,
        diagnostics=diagnostics,
    )


def _summary(
    package: Path, args: argparse.Namespace, code: int, output: str, elapsed: float
) -> str:
    """Return the Boot test job-summary section; a failure adds the end of QGC's output."""
    if code == 0:
        result = "passed"
    elif code == BOOT_TIMEOUT_EXIT_CODE:
        result = f"timed out after {args.timeout}s"
    else:
        result = f"failed (exit code {_describe_exit_code(code)})"
    facts: list[tuple[str, object]] = [
        ("Result", result),
        ("Package", code_span(package.name)),
        ("Type", args.exe_type),
        ("Size", format_bytes(package.stat().st_size) if package.is_file() else ""),
        ("Duration", format_duration(elapsed)),
    ]
    collapsed: list[tuple[str, str]] = []
    lines = output.strip().splitlines()
    if code != 0 and lines:
        facts.append(("Last QGC log line", code_span(lines[-1][:LAST_LINE_CHARS])))
        tail = lines[-OUTPUT_TAIL_LINES:]
        collapsed.append((f"Last {len(tail)} lines of output", code_block("\n".join(tail))))
    return summary_section("Boot test", facts, collapsed=collapsed)


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    # Defaults come from the verify-executable action's INPUT_* step environment.
    parser.add_argument(
        "--binary-path",
        default=env_default("INPUT_BINARY_PATH"),
        help="Executable, AppImage, or DMG; the boot test runs in its directory",
    )
    parser.add_argument("--build-dir", default=env_default("INPUT_BUILD_DIR"))
    parser.add_argument(
        "--type", choices=EXE_TYPES, default=env_default("INPUT_TYPE", "binary"), dest="exe_type"
    )
    parser.add_argument("--timeout", type=int, default=60)
    parser.add_argument("--expect-archs", default=env_default("INPUT_EXPECT_ARCHS"))
    parser.add_argument(
        "--expected-appimage-update-information",
        default=env_default("INPUT_EXPECTED_APPIMAGE_UPDATE_INFORMATION"),
    )
    args = parser.parse_args(argv)
    if not args.binary_path:
        parser.error("--binary-path (or INPUT_BINARY_PATH) is required")
    if args.exe_type not in EXE_TYPES:
        parser.error(f"invalid INPUT_TYPE {args.exe_type!r}; choose from {', '.join(EXE_TYPES)}")

    package = Path(args.binary_path)
    diagnostics = _prepare_diagnostics(package)
    start = time.monotonic()
    output = ""
    if args.exe_type != "dmg":
        code, output = _verify(package, args, diagnostics)
    else:
        try:
            with _attached_dmg(package) as mount_point:
                code, output = _verify(_bundle_executable(mount_point), args, diagnostics)
        except RuntimeError as error:
            gh_error(str(error))
            code = 1
    write_step_summary(_summary(package, args, code, output, time.monotonic() - start))
    return _step_exit_code(code)


if __name__ == "__main__":
    raise SystemExit(main())
