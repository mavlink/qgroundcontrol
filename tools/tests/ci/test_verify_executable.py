"""Tests for verify_executable.py."""

from __future__ import annotations

import argparse
import os
import shutil
import signal
import subprocess
import sys
import time
from pathlib import Path

import pytest
from ci.packaging import verify_executable
from ci.packaging.verify_executable import (
    BOOT_TIMEOUT_EXIT_CODE,
    _boot_command,
    _boot_test,
    _execute,
    _setup_gstreamer_env,
    _verify,
    _verify_appimage_update_information,
)

from .._helpers import load_action, load_workflow

pytestmark = pytest.mark.windows


@pytest.fixture(autouse=True)
def runner_temp(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> Path:
    """Keep diagnostics under the test's temporary directory; never wait for crash reports."""
    path = tmp_path / "runner-temp"
    path.mkdir()
    monkeypatch.setenv("RUNNER_TEMP", str(path))
    monkeypatch.setattr(verify_executable, "CRASH_REPORT_WAIT_SECONDS", 0)
    return path


class TestSetupGstreamerEnv:
    """Tests for _setup_gstreamer_env function."""

    def test_no_cache_file(self, tmp_path, monkeypatch):
        """Returns without setting env if CMakeCache.txt is missing."""
        monkeypatch.delenv("GST_PLUGIN_SYSTEM_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_SYSTEM_PATH" not in os.environ

    def test_no_gstreamer_in_cache(self, tmp_path, monkeypatch):
        """Returns without setting env if no resolved GStreamer root in cache."""
        cache = tmp_path / "CMakeCache.txt"
        cache.write_text("CMAKE_BUILD_TYPE:STRING=Release\n")
        monkeypatch.delenv("GST_PLUGIN_SYSTEM_PATH", raising=False)
        _setup_gstreamer_env(tmp_path)
        assert "GST_PLUGIN_SYSTEM_PATH" not in os.environ

    def test_sets_plugin_path(self, tmp_path, monkeypatch):
        """Points the GStreamer system plugin path at the SDK when its plugin dir exists."""
        gst_root = tmp_path / "gstreamer"
        plugin_dir = gst_root / "lib" / "gstreamer-1.0"
        plugin_dir.mkdir(parents=True)

        cache = tmp_path / "CMakeCache.txt"
        cache.write_text(f"GStreamer_ROOT_DIR:PATH=\nQGC_GST_RESOLVED_ROOT:INTERNAL={gst_root}\n")

        for var in ("GST_PLUGIN_SYSTEM_PATH", "GST_PLUGIN_SYSTEM_PATH_1_0"):
            monkeypatch.delenv(var, raising=False)
        _setup_gstreamer_env(tmp_path)

        assert os.environ["GST_PLUGIN_SYSTEM_PATH"] == str(plugin_dir)
        assert os.environ["GST_PLUGIN_SYSTEM_PATH_1_0"] == str(plugin_dir)

    @pytest.mark.parametrize(("system", "staged"), [("Windows", True), ("Darwin", False)])
    def test_windows_loads_only_shipped_plugins(self, tmp_path, monkeypatch, system, staged):
        """On Windows the boot test loads only the plugins the package ships, not the whole SDK."""
        gst_root = tmp_path / "gstreamer"
        plugin_dir = gst_root / "lib" / "gstreamer-1.0"
        plugin_dir.mkdir(parents=True)
        for name in ("gstcoreelements.dll", "gstrtp.dll", "gstwebrtc.dll"):
            (plugin_dir / name).write_bytes(name.encode())
        shipped = ";".join(str(plugin_dir / name) for name in ("gstcoreelements.dll", "gstrtp.dll"))
        (tmp_path / "CMakeCache.txt").write_text(
            f"QGC_GST_RESOLVED_ROOT:INTERNAL={gst_root}\nQGC_GST_SHIPPED_PLUGINS:INTERNAL={shipped}\n"
        )
        stage = tmp_path / verify_executable.SHIPPED_PLUGIN_DIR
        stage.mkdir()
        (stage / "stale.dll").write_text("left over from an earlier run")
        monkeypatch.setattr(verify_executable.platform, "system", lambda: system)
        for var in (
            "GST_PLUGIN_SYSTEM_PATH",
            "GST_PLUGIN_SYSTEM_PATH_1_0",
            "PATH",
            "DYLD_LIBRARY_PATH",
        ):
            monkeypatch.delenv(var, raising=False)

        _setup_gstreamer_env(tmp_path)

        expected = stage if staged else plugin_dir
        assert os.environ["GST_PLUGIN_SYSTEM_PATH"] == str(expected)
        assert os.environ["GST_PLUGIN_SYSTEM_PATH_1_0"] == str(expected)
        if staged:
            assert sorted(path.name for path in stage.iterdir()) == [
                "gstcoreelements.dll",
                "gstrtp.dll",
            ]
            assert (stage / "gstrtp.dll").read_bytes() == b"gstrtp.dll"


def test_verify_appimage_update_information(tmp_path, monkeypatch):
    appimage = tmp_path / "QGroundControl-x86_64.AppImage"
    update_information = (
        "gh-releases-zsync|mavlink|qgroundcontrol|latest|QGroundControl-x86_64.AppImage.zsync"
    )
    (tmp_path / f"{appimage.name}.zsync").write_text("zsync metadata")
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout=f"{update_information}\n", stderr=""
        ),
    )

    _verify_appimage_update_information(appimage, update_information)


def test_verify_appimage_update_information_rejects_mismatch_or_missing_sidecar(
    tmp_path, monkeypatch
):
    appimage = tmp_path / "QGroundControl-x86_64.AppImage"
    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout="unexpected\n", stderr=""
        ),
    )

    with pytest.raises(RuntimeError, match="Unexpected AppImage update information"):
        _verify_appimage_update_information(appimage, "expected")

    monkeypatch.setattr(
        subprocess,
        "run",
        lambda *args, **kwargs: subprocess.CompletedProcess(
            args[0], 0, stdout="expected\n", stderr=""
        ),
    )
    with pytest.raises(RuntimeError, match="zsync sidecar is missing or empty"):
        _verify_appimage_update_information(appimage, "expected")


def _process_gone(pid: int) -> bool:
    try:
        os.kill(pid, 0)
    except ProcessLookupError:
        return True
    if not Path("/proc").is_dir():
        return False
    try:
        state = Path(f"/proc/{pid}/stat").read_text().split(")")[-1].split()[0]
    except FileNotFoundError:
        # Reaped between the liveness probe and the read.
        return True
    return state == "Z"


class TestExecute:
    def test_returns_exit_code_and_combined_output(self, tmp_path: Path) -> None:
        code = "import sys; print('out'); print('err', file=sys.stderr); sys.exit(3)"
        exit_code, output = _execute([sys.executable, "-c", code], dict(os.environ), tmp_path, 60)
        assert exit_code == 3
        assert "out" in output
        assert "err" in output

    def test_runs_in_working_directory(self, tmp_path: Path) -> None:
        code = "import os; print(os.getcwd())"
        exit_code, output = _execute([sys.executable, "-c", code], dict(os.environ), tmp_path, 60)
        assert exit_code == 0
        assert Path(output.strip()).resolve() == tmp_path.resolve()

    def test_invalid_utf8_output_is_replaced(self, tmp_path: Path) -> None:
        code = "import sys; sys.stdout.buffer.write(b'bad \\xff byte')"
        exit_code, output = _execute([sys.executable, "-c", code], dict(os.environ), tmp_path, 60)
        assert exit_code == 0
        assert output == "bad \ufffd byte"

    @pytest.mark.skipif(os.name == "nt", reason="process groups are POSIX-only")
    def test_timeout_kills_grandchildren(self, tmp_path: Path) -> None:
        # Mirrors xvfb-run: the boot-tested binary is a grandchild of the launched process
        pid_file = tmp_path / "grandchild.pid"
        script = 'sleep 60 & echo $! > "$1"; echo started; sleep 60'
        exit_code, output = _execute(
            ["sh", "-c", script, "sh", str(pid_file)], dict(os.environ), tmp_path, 3
        )
        assert exit_code == BOOT_TIMEOUT_EXIT_CODE
        assert "started" in output
        grandchild = int(pid_file.read_text())
        deadline = time.monotonic() + 5
        while not _process_gone(grandchild) and time.monotonic() < deadline:
            time.sleep(0.05)
        assert _process_gone(grandchild)


def test_tool_timeout_becomes_failed_result(monkeypatch):
    def hang(cmd, **kwargs):
        assert kwargs["timeout"] == verify_executable.TOOL_TIMEOUT_SECONDS
        raise subprocess.TimeoutExpired(cmd, kwargs["timeout"])

    monkeypatch.setattr(subprocess, "run", hang)
    appimage = Path("QGroundControl-x86_64.AppImage")

    with pytest.raises(RuntimeError, match=r"update information .*: timed out after 120s"):
        _verify_appimage_update_information(appimage, "expected")


def test_type_rejects_unknown_values(capsys):
    with pytest.raises(SystemExit) as exc:
        verify_executable.main(["--binary-path", "QGC", "--type", "apk"])
    assert exc.value.code == 2
    assert "invalid choice: 'apk'" in capsys.readouterr().err


BOOT_BINARY = Path("/opt/qgc/QGroundControl")
BOOT_LOGGING = f"--logging:{verify_executable.BOOT_LOGGING_CATEGORIES}"
PLAIN_BOOT = [str(BOOT_BINARY), "--simple-boot-test", BOOT_LOGGING]


class TestBootCommand:
    @pytest.fixture(autouse=True)
    def _clean_env(self, monkeypatch: pytest.MonkeyPatch) -> None:
        monkeypatch.delenv("QT_QPA_PLATFORM", raising=False)
        monkeypatch.delenv("DISPLAY", raising=False)
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Linux")

    def test_headless_forces_offscreen_without_overriding(self, monkeypatch) -> None:
        cmd, env = _boot_command(BOOT_BINARY, headless=True)
        assert cmd == PLAIN_BOOT
        assert env["QT_QPA_PLATFORM"] == "offscreen"

        monkeypatch.setenv("QT_QPA_PLATFORM", "minimal")
        assert _boot_command(BOOT_BINARY, headless=True)[1]["QT_QPA_PLATFORM"] == "minimal"

    def test_linux_without_display_uses_xvfb_run(self, monkeypatch) -> None:
        monkeypatch.setattr(verify_executable.shutil, "which", lambda _: "/usr/bin/xvfb-run")
        cmd, env = _boot_command(BOOT_BINARY, headless=False)
        assert cmd == ["/usr/bin/xvfb-run", "-a", *PLAIN_BOOT]
        assert "QT_QPA_PLATFORM" not in env

    def test_linux_without_display_or_xvfb_falls_back_to_offscreen(
        self, monkeypatch, capsys
    ) -> None:
        monkeypatch.setattr(verify_executable.shutil, "which", lambda _: None)
        monkeypatch.setenv("QT_QPA_PLATFORM", "xcb")
        cmd, env = _boot_command(BOOT_BINARY, headless=False)
        assert cmd == PLAIN_BOOT
        assert env["QT_QPA_PLATFORM"] == "offscreen"
        assert "::warning::xvfb-run not found" in capsys.readouterr().out

    @pytest.mark.parametrize("system", ["Darwin", "Windows"])
    def test_non_linux_runs_binary_directly(self, monkeypatch, system) -> None:
        monkeypatch.setattr(verify_executable.platform, "system", lambda: system)
        cmd, env = _boot_command(BOOT_BINARY, headless=False)
        assert cmd == PLAIN_BOOT
        assert "QT_QPA_PLATFORM" not in env

    def test_linux_with_display_runs_binary_directly(self, monkeypatch) -> None:
        monkeypatch.setenv("DISPLAY", ":0")
        assert _boot_command(BOOT_BINARY, headless=False)[0] == PLAIN_BOOT

    def test_forces_startup_log_to_stderr_without_overriding(self, monkeypatch) -> None:
        monkeypatch.delenv("QT_FORCE_STDERR_LOGGING", raising=False)
        assert _boot_command(BOOT_BINARY, headless=True)[1]["QT_FORCE_STDERR_LOGGING"] == "1"

        monkeypatch.setenv("QT_FORCE_STDERR_LOGGING", "0")
        assert _boot_command(BOOT_BINARY, headless=True)[1]["QT_FORCE_STDERR_LOGGING"] == "0"


class TestBootTest:
    def test_missing_binary_fails(self, tmp_path, capsys) -> None:
        missing = tmp_path / "missing"
        assert _boot_test(
            missing, cwd=tmp_path, timeout=5, headless=True, diagnostics=tmp_path
        ) == (
            1,
            "",
        )
        assert "::error::Binary not found" in capsys.readouterr().out

    @pytest.mark.parametrize(
        ("result", "expected"),
        [
            ((0, "booted\n"), ["booted", "Boot test passed"]),
            ((2, "crashed\n"), ["crashed", "::error::Boot test failed (exit code 2)"]),
            (
                (3221225477, "QGC: unhandled SEH\n"),
                ["::error::Boot test failed (exit code 0xC0000005 STATUS_ACCESS_VIOLATION)"],
            ),
            ((-11, "segfault\n"), ["::error::Boot test failed (exit code -11 SIGSEGV)"]),
        ],
    )
    def test_prints_output_and_result(self, tmp_path, monkeypatch, capsys, result, expected):
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        monkeypatch.setattr(verify_executable, "_execute", lambda *args: result)
        assert _boot_test(binary, cwd=tmp_path, timeout=5, headless=True, diagnostics=tmp_path) == (
            result
        )
        out = capsys.readouterr().out
        assert all(line in out for line in expected)
        assert (tmp_path / "boot-output.txt").exists() == (result[0] != 0)

    def test_passing_output_is_folded(self, tmp_path, monkeypatch, capsys) -> None:
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        monkeypatch.setattr(verify_executable, "_execute", lambda *args: (0, "startup log\n"))
        assert (
            _boot_test(binary, cwd=tmp_path, timeout=5, headless=True, diagnostics=tmp_path)[0] == 0
        )
        out = capsys.readouterr().out
        assert (
            out.index("::group::Boot output") < out.index("startup log") < out.index("::endgroup::")
        )
        assert out.index("::endgroup::") < out.index("Boot test passed in ")

    def test_timeout_prints_partial_output_before_error(
        self, tmp_path, monkeypatch, capsys
    ) -> None:
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        monkeypatch.setattr(
            verify_executable, "_execute", lambda *args: (BOOT_TIMEOUT_EXIT_CODE, "log before hang")
        )
        assert _boot_test(binary, cwd=tmp_path, timeout=5, headless=True, diagnostics=tmp_path) == (
            124,
            "log before hang",
        )
        out = capsys.readouterr().out
        assert out.index("log before hang") < out.index("::error::Boot test timed out after 5s")

    def test_timeout_captures_stacks_before_the_kill(self, tmp_path, monkeypatch, capsys) -> None:
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        captured = []

        def hang(cmd, env, cwd, timeout, on_timeout):
            on_timeout(4321)
            return BOOT_TIMEOUT_EXIT_CODE, "GStreamer plugin scan\n"

        def capture(pid, target, diagnostics):
            captured.append((pid, target, diagnostics))
            return "Thread 1 (LWP 4321): #0 poll ()"

        monkeypatch.setattr(verify_executable, "_execute", hang)
        monkeypatch.setattr(verify_executable, "_capture_stacks", capture)
        _boot_test(binary, cwd=tmp_path, timeout=5, headless=True, diagnostics=tmp_path)

        assert captured == [(4321, binary, tmp_path)]
        out = capsys.readouterr().out
        assert (
            out.index("GStreamer plugin scan")
            < out.index("::group::Stacks before the timeout kill")
            < out.index("Thread 1 (LWP 4321)")
            < out.index("::error::Boot test timed out")
        )
        assert (tmp_path / "boot-output.txt").read_text() == "GStreamer plugin scan\n"

    @pytest.mark.parametrize(("system", "dump_dir"), [("Windows", True), ("Linux", False)])
    def test_windows_points_qgc_crash_dumps_at_the_diagnostics(
        self, tmp_path, monkeypatch, system, dump_dir
    ) -> None:
        binary = tmp_path / "QGroundControl.exe"
        binary.write_text("binary")
        envs = []
        monkeypatch.delenv("QGC_CRASH_DUMP_DIR", raising=False)
        monkeypatch.setattr(verify_executable.platform, "system", lambda: system)
        monkeypatch.setattr(
            verify_executable, "_execute", lambda cmd, env, *args: envs.append(env) or (0, "")
        )
        diagnostics = tmp_path / "diagnostics"
        _boot_test(binary, cwd=tmp_path, timeout=5, headless=True, diagnostics=diagnostics)
        assert envs[0].get("QGC_CRASH_DUMP_DIR") == (str(diagnostics) if dump_dir else None)
        assert "QGC_CRASH_DUMP_DIR" not in os.environ


@pytest.mark.parametrize(
    ("exe_type", "name", "headless"),
    [("binary", "QGroundControl", True), ("appimage", "QGroundControl-x86_64.AppImage", False)],
)
def test_verify_boot_tests_executable_in_its_directory(
    tmp_path, monkeypatch, exe_type, name, headless
):
    work_dir = tmp_path / "work"
    work_dir.mkdir()
    binary = work_dir / name
    binary.write_text("binary")
    binary.chmod(0o644)
    calls = []
    monkeypatch.setattr(
        verify_executable,
        "_boot_test",
        lambda *args, **kwargs: calls.append((args, kwargs)) or (0, ""),
    )
    args = argparse.Namespace(
        build_dir="",
        expect_archs="",
        exe_type=exe_type,
        expected_appimage_update_information="",
        timeout=60,
    )

    assert _verify(binary, args, tmp_path) == (0, "")

    assert calls == [
        (
            (binary.resolve(),),
            {"cwd": work_dir, "timeout": 60, "headless": headless, "diagnostics": tmp_path},
        )
    ]
    if os.name != "nt":
        assert os.access(binary, os.X_OK)


def test_action_inputs_are_argument_defaults(tmp_path, monkeypatch, gh_summary):
    binary = tmp_path / "QGroundControl-x86_64.AppImage"
    binary.write_text("binary")
    inputs = {
        "INPUT_BINARY_PATH": str(binary),
        "INPUT_TYPE": "appimage",
        "INPUT_BUILD_DIR": str(tmp_path / "build"),
        "INPUT_EXPECT_ARCHS": "arm64",
        "INPUT_EXPECTED_APPIMAGE_UPDATE_INFORMATION": "zsync|https://example.test/latest.zsync",
    }
    for name, value in inputs.items():
        monkeypatch.setenv(name, value)
    verified = []
    monkeypatch.setattr(
        verify_executable,
        "_verify",
        lambda path, args, diagnostics: verified.append(args) or (0, ""),
    )

    assert verify_executable.main([]) == 0

    (args,) = verified
    assert (args.binary_path, args.exe_type, args.build_dir) == (
        str(binary),
        "appimage",
        inputs["INPUT_BUILD_DIR"],
    )
    assert args.expect_archs == "arm64"
    assert (
        args.expected_appimage_update_information
        == inputs["INPUT_EXPECTED_APPIMAGE_UPDATE_INFORMATION"]
    )
    assert "| Package | `QGroundControl-x86_64.AppImage` |" in gh_summary.read_text(
        encoding="utf-8"
    )


@pytest.mark.parametrize(
    ("inputs", "message"),
    [
        ({}, "--binary-path (or INPUT_BINARY_PATH) is required"),
        ({"INPUT_BINARY_PATH": "QGC", "INPUT_TYPE": "apk"}, "invalid INPUT_TYPE 'apk'"),
    ],
)
def test_missing_or_invalid_action_inputs_fail(monkeypatch, capsys, inputs, message):
    for name, value in inputs.items():
        monkeypatch.setenv(name, value)
    with pytest.raises(SystemExit) as exc:
        verify_executable.main([])
    assert exc.value.code == 2
    assert message in capsys.readouterr().err


def test_callers_pass_only_declared_inputs():
    """The boot test runs in the binary's directory, so no caller passes a working directory."""
    inputs = set(load_action("verify-executable")["inputs"])
    assert "working-dir" not in inputs
    for workflow in ("linux.yml", "macos.yml", "windows.yml"):
        for job in load_workflow(workflow)["jobs"].values():
            for step in job.get("steps", []):
                if step.get("uses") == "./.github/actions/verify-executable":
                    assert set(step["with"]) <= inputs, step["name"]


class FakeDmgTools:
    """Fake hdiutil/lipo/boot test that records commands and mounts a bundle on attach."""

    def __init__(self, *, attach_rc=0, detach_rcs=(0,), boot_rc=0, archs="x86_64h arm64"):
        self.calls: list[list[str]] = []
        self.boots: list[tuple[list[str], dict[str, str]]] = []
        self.attach_rc = attach_rc
        self.detach_rcs = list(detach_rcs)
        self.boot_rc = boot_rc
        self.archs = archs
        self.mount_point: Path | None = None

    def __call__(self, cmd, **kwargs):
        cmd = [str(part) for part in cmd]
        self.calls.append(cmd)
        if cmd[:2] == ["hdiutil", "attach"]:
            self.mount_point = Path(cmd[cmd.index("-mountpoint") + 1])
            if self.attach_rc == 0:
                macos = self.mount_point / "QGroundControl.app" / "Contents" / "MacOS"
                macos.mkdir(parents=True)
                (macos / "QGroundControl").write_text("binary")
            return subprocess.CompletedProcess(cmd, self.attach_rc, stdout="", stderr="no image")
        if cmd[:2] == ["hdiutil", "detach"]:
            rc = self.detach_rcs.pop(0)
            if rc == 0 and self.mount_point is not None:
                shutil.rmtree(self.mount_point / "QGroundControl.app")
            return subprocess.CompletedProcess(cmd, rc, stdout="", stderr="resource busy")
        if cmd[0] == "lipo":
            return subprocess.CompletedProcess(cmd, 0, stdout=f"{self.archs}\n", stderr="")
        raise AssertionError(f"unexpected command: {cmd}")

    def execute(self, cmd, env, cwd, timeout, on_timeout=None):
        self.calls.append(cmd)
        self.boots.append((cmd, env))
        return self.boot_rc, ""

    def commands(self, program: str) -> list[list[str]]:
        return [cmd for cmd in self.calls if cmd[0] == program]


def _run_dmg_main(monkeypatch, tmp_path, tools: FakeDmgTools, *, create_dmg=True):
    dmg = tmp_path / "QGroundControl.dmg"
    if create_dmg:
        dmg.write_bytes(b"dmg")
    monkeypatch.delenv("QT_QPA_PLATFORM", raising=False)
    monkeypatch.setattr(subprocess, "run", tools)
    monkeypatch.setattr(verify_executable, "_execute", tools.execute)
    monkeypatch.setattr(verify_executable.platform, "system", lambda: "Darwin")
    return verify_executable.main(
        ["--binary-path", str(dmg), "--type", "dmg", "--expect-archs", "x86_64h arm64"]
    )


def test_dmg_verifies_bundle_inside_image_and_detaches(monkeypatch, tmp_path, gh_summary):
    tools = FakeDmgTools()

    assert _run_dmg_main(monkeypatch, tmp_path, tools) == 0
    summary = gh_summary.read_text(encoding="utf-8")
    assert summary.startswith("### Boot test\n\n| Item | Value |")
    assert "| Result | passed |\n| Package | `QGroundControl.dmg` |\n| Type | dmg |" in summary

    attach, detach = tools.commands("hdiutil")
    assert attach[:4] == ["hdiutil", "attach", "-nobrowse", "-readonly"]
    assert attach[-1] == str(tmp_path / "QGroundControl.dmg")
    assert tools.mount_point is not None
    assert tools.mount_point.parent == tmp_path / "runner-temp"
    executable = tools.mount_point / "QGroundControl.app" / "Contents" / "MacOS" / "QGroundControl"
    assert tools.commands("lipo") == [["lipo", "-archs", str(executable)]]
    ((boot, env),) = tools.boots
    assert boot == [str(executable.resolve()), "--simple-boot-test", BOOT_LOGGING]
    assert env["QT_QPA_PLATFORM"] == "offscreen"
    assert detach == ["hdiutil", "detach", str(tools.mount_point)]
    assert tools.calls.index(boot) < tools.calls.index(detach)
    assert not tools.mount_point.exists()


@pytest.mark.parametrize(
    ("tools", "expected_code", "expected_boots"),
    [(FakeDmgTools(boot_rc=3), 3, 1), (FakeDmgTools(archs="arm64"), 1, 0)],
    ids=["boot-failure", "arch-mismatch"],
)
def test_dmg_failures_still_detach(
    monkeypatch, tmp_path, tools, expected_code, expected_boots, gh_summary
):
    assert _run_dmg_main(monkeypatch, tmp_path, tools) == expected_code
    assert f"| Result | failed (exit code {expected_code}) |" in gh_summary.read_text(
        encoding="utf-8"
    )
    assert len(tools.boots) == expected_boots
    assert tools.calls[-1][:2] == ["hdiutil", "detach"]


def test_dmg_detach_retries_with_force_then_warns(monkeypatch, tmp_path, capsys):
    tools = FakeDmgTools(detach_rcs=(1, 1))

    assert _run_dmg_main(monkeypatch, tmp_path, tools) == 0

    detaches = [cmd for cmd in tools.commands("hdiutil") if cmd[1] == "detach"]
    assert [cmd[2:-1] for cmd in detaches] == [[], ["-force"]]
    assert "::warning::Failed to detach" in capsys.readouterr().out


@pytest.mark.parametrize("create_dmg", [True, False], ids=["attach-failure", "missing-dmg"])
def test_dmg_mount_failures_report_errors_without_detaching(
    monkeypatch, tmp_path, capsys, create_dmg
):
    tools = FakeDmgTools(attach_rc=1)

    assert _run_dmg_main(monkeypatch, tmp_path, tools, create_dmg=create_dmg) == 1

    out = capsys.readouterr().out
    assert not tools.boots
    assert not [cmd for cmd in tools.calls if cmd[:2] == ["hdiutil", "detach"]]
    if create_dmg:
        assert "::error::Failed to mount DMG" in out
        assert not list((tmp_path / "runner-temp").glob("qgc-dmg-*"))
    else:
        assert "::error::DMG file not found" in out
        assert not tools.calls


def test_macos_release_verifies_the_packaged_dmg_in_one_step():
    workflow = load_workflow("macos.yml")
    steps = workflow["jobs"]["build"]["steps"]
    assert "hdiutil" not in str(steps)
    (dmg,) = (step for step in steps if step.get("name") == "Verify DMG executable")
    assert dmg["uses"] == "./.github/actions/verify-executable"
    assert dmg["with"] == {
        "binary-path": "${{ runner.temp }}/build/QGroundControl.dmg",
        "type": "dmg",
        "expect-archs": "x86_64h arm64",
    }


@pytest.mark.parametrize(
    ("code", "expected"),
    [
        (3221225477, "0xC0000005 STATUS_ACCESS_VIOLATION"),
        (0xC0000409, "0xC0000409 STATUS_STACK_BUFFER_OVERRUN"),
        (0xC0000374, "0xC0000374 STATUS_HEAP_CORRUPTION"),
        (0xC0000123, "0xC0000123"),
        (-11, "-11 SIGSEGV"),
        (-200, "-200"),
        (3, "3"),
    ],
)
def test_describe_exit_code_names_signals_and_ntstatus(code, expected):
    assert verify_executable._describe_exit_code(code) == expected


def _run_main(monkeypatch, tmp_path, result, *args):
    binary = tmp_path / "QGroundControl"
    binary.write_text("binary")
    monkeypatch.setattr(verify_executable, "_verify", lambda *_: result)
    return verify_executable.main(["--binary-path", str(binary), *args])


@pytest.mark.parametrize(
    ("code", "step_code"),
    [(0, 0), (2, 2), (BOOT_TIMEOUT_EXIT_CODE, BOOT_TIMEOUT_EXIT_CODE), (-11, 1), (3221225477, 1)],
)
def test_step_exits_with_a_code_the_shell_can_report(monkeypatch, tmp_path, code, step_code):
    """0xC0000005 overflowed Python 3.10's exit status on Windows, which MSYS bash reported as 127."""
    assert _run_main(monkeypatch, tmp_path, (code, "")) == step_code


def test_failure_summary_names_the_crash_and_shows_the_end_of_the_output(
    monkeypatch, tmp_path, gh_summary
):
    lines = [f"line {number}" for number in range(1, 51)]
    output = "\n".join(
        [*lines, "QGC: unhandled SEH 0xC0000005 at 0x7FF6 in QGroundControl.exe+0x1A2B"]
    )
    assert _run_main(monkeypatch, tmp_path, (3221225477, output)) == 1

    summary = gh_summary.read_text(encoding="utf-8")
    assert "| Result | failed (exit code 0xC0000005 STATUS_ACCESS_VIOLATION) |" in summary
    assert (
        "| Last QGC log line | `QGC: unhandled SEH 0xC0000005 at 0x7FF6 in QGroundControl.exe+0x1A2B` |"
        in summary
    )
    details = summary[summary.index("<details><summary>Last 40 lines of output</summary>") :]
    assert "line 12\n" in details
    assert "line 11\n" not in details
    assert details.rstrip().endswith("</details>")


@pytest.mark.parametrize(
    ("result", "expected"),
    [((0, "booted\n"), "| Result | passed |"), ((124, ""), "| Result | timed out after 60s |")],
)
def test_summary_adds_output_only_to_failures_with_output(
    monkeypatch, tmp_path, gh_summary, result, expected
):
    _run_main(monkeypatch, tmp_path, result)
    summary = gh_summary.read_text(encoding="utf-8")
    assert expected in summary
    assert "Last QGC log line" not in summary
    assert "<details>" not in summary


def test_main_outputs_a_fresh_diagnostics_directory(monkeypatch, tmp_path, runner_temp, gh_output):
    monkeypatch.setenv("GITHUB_JOB", "build")
    monkeypatch.setenv("RUNNER_OS", "macOS")
    monkeypatch.setenv("RUNNER_ARCH", "ARM64")
    monkeypatch.setenv("GITHUB_RUN_ATTEMPT", "2")
    stale_dir, _ = verify_executable._diagnostics_location(tmp_path / "QGroundControl")
    stale_dir.mkdir(parents=True)
    (stale_dir / "stale.dmp").write_text("earlier run")
    seen = []
    monkeypatch.setattr(
        verify_executable,
        "_verify",
        lambda path, args, diagnostics: seen.append(diagnostics) or (0, ""),
    )
    binary = tmp_path / "QGroundControl"
    binary.write_text("binary")

    assert verify_executable.main(["--binary-path", str(binary)]) == 0

    outputs = dict(line.split("=", 1) for line in gh_output.read_text().splitlines())
    diagnostics = Path(outputs["diagnostics-dir"])
    assert seen == [diagnostics]
    assert diagnostics.parent == runner_temp / verify_executable.DIAGNOSTICS_DIR
    assert diagnostics.is_dir()
    assert not any(diagnostics.iterdir())
    name = outputs["diagnostics-name"]
    assert diagnostics.name == name
    assert name.startswith("boot-test-build-macOS-ARM64-QGroundControl-")
    assert name.endswith("-2")


def test_diagnostics_names_differ_per_package_and_runner(monkeypatch, tmp_path):
    def name(package, arch):
        monkeypatch.setenv("RUNNER_ARCH", arch)
        return verify_executable._diagnostics_location(package)[1]

    release = tmp_path / "Release" / "QGroundControl app"
    debug = tmp_path / "Debug" / "QGroundControl app"
    names = {name(release, "X64"), name(debug, "X64"), name(release, "ARM64")}
    assert len(names) == 3
    assert all(" " not in item for item in names)


@pytest.mark.skipif(os.name == "nt", reason="liveness probe uses POSIX signals")
def test_execute_runs_timeout_hook_before_killing(tmp_path: Path) -> None:
    alive = []

    def capture(pid):
        # A process killed before the hook would be a zombie well within this pause.
        time.sleep(0.3)
        alive.append((pid, _process_gone(pid)))

    exit_code, output = _execute(
        ["sh", "-c", "echo started; sleep 60"], dict(os.environ), tmp_path, 2, capture
    )
    assert exit_code == BOOT_TIMEOUT_EXIT_CODE
    assert "started" in output
    ((pid, gone),) = alive
    assert not gone
    deadline = time.monotonic() + 5
    while not _process_gone(pid) and time.monotonic() < deadline:
        time.sleep(0.05)
    assert _process_gone(pid)


@pytest.mark.skipif(os.name == "nt", reason="liveness probe uses POSIX signals")
def test_execute_kills_even_when_the_timeout_hook_fails(tmp_path: Path) -> None:
    pids = []

    def fail(pid):
        pids.append(pid)
        raise RuntimeError("stack capture broke")

    with pytest.raises(RuntimeError, match="stack capture broke"):
        _execute(["sleep", "60"], dict(os.environ), tmp_path, 1, fail)
    deadline = time.monotonic() + 5
    while not _process_gone(pids[0]) and time.monotonic() < deadline:
        time.sleep(0.05)
    assert _process_gone(pids[0])


class TestCaptureStacks:
    @pytest.fixture
    def tools(self, monkeypatch):
        """Fake which() and subprocess.run for stack tools; returns the recorded commands."""
        available = {"sample", "gdb", "eu-stack", "sudo"}
        calls = []

        def run(cmd, **kwargs):
            calls.append((cmd, kwargs["timeout"]))
            if "-file" in cmd:
                Path(cmd[cmd.index("-file") + 1]).write_text("Call graph:\n  main\n")
                return subprocess.CompletedProcess(cmd, 0, stdout="Sampling...\n", stderr="")
            return subprocess.CompletedProcess(
                cmd, 0, stdout="Thread 1 (LWP 77):\n#0 poll ()\n", stderr=""
            )

        monkeypatch.setattr(
            verify_executable.shutil,
            "which",
            lambda name: f"/usr/bin/{name}" if name in available else None,
        )
        monkeypatch.setattr(subprocess, "run", run)
        monkeypatch.setattr(verify_executable, "_boot_pid", lambda session, binary: 77)
        monkeypatch.setattr(verify_executable.os, "geteuid", lambda: 1000, raising=False)
        return available, calls

    def test_macos_samples_into_the_diagnostics(self, tmp_path, monkeypatch, tools):
        _, calls = tools
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Darwin")
        report = verify_executable._capture_stacks(55, Path("QGroundControl"), tmp_path)
        path = tmp_path / "stacks-55.txt"
        assert calls == [
            (["sample", "55", "3", "-file", str(path)], verify_executable.STACK_TIMEOUT_SECONDS)
        ]
        assert report == path.read_text() == "Call graph:\n  main\n"

    @pytest.mark.skipif(os.name == "nt", reason="sudo applies to POSIX runners")
    @pytest.mark.parametrize("ci", [True, False])
    def test_linux_runs_gdb_on_qgc_with_sudo_in_ci(self, tmp_path, monkeypatch, tools, ci):
        _, calls = tools
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Linux")
        if ci:
            monkeypatch.setenv("CI", "true")
        report = verify_executable._capture_stacks(55, Path("QGroundControl"), tmp_path)
        gdb = ["gdb", "-batch", "-p", "77", "-ex", "thread apply all bt"]
        assert calls == [([*(["sudo", "-n"] if ci else []), *gdb], 30)]
        assert (
            report == (tmp_path / "stacks-77.txt").read_text() == "Thread 1 (LWP 77):\n#0 poll ()\n"
        )

    def test_linux_falls_back_to_eu_stack_then_reports_no_tool(self, tmp_path, monkeypatch, tools):
        available, calls = tools
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Linux")
        available.discard("gdb")
        verify_executable._capture_stacks(55, Path("QGroundControl"), tmp_path)
        assert calls[0][0][-3:] == ["eu-stack", "-p", "77"]
        available.discard("eu-stack")
        assert "No stack capture tool" in verify_executable._capture_stacks(
            55, Path("QGroundControl"), tmp_path
        )
        assert len(calls) == 1

    def test_tool_timeout_is_reported_not_raised(self, tmp_path, monkeypatch, tools):
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Linux")

        def hang(cmd, **kwargs):
            raise subprocess.TimeoutExpired(cmd, kwargs["timeout"])

        monkeypatch.setattr(subprocess, "run", hang)
        report = verify_executable._capture_stacks(55, Path("QGroundControl"), tmp_path)
        assert report.startswith("Stack capture failed: ")
        assert "timed out after 30 seconds" in report

    def test_windows_writes_a_minidump(self, tmp_path, monkeypatch, tools):
        _, calls = tools
        dumps = []
        monkeypatch.setattr(verify_executable.platform, "system", lambda: "Windows")
        monkeypatch.setattr(
            verify_executable,
            "_write_minidump",
            lambda pid, path: dumps.append((pid, path)) or "Wrote minidump",
        )
        assert verify_executable._capture_stacks(55, Path("QGroundControl.exe"), tmp_path) == (
            "Wrote minidump"
        )
        assert dumps == [(55, tmp_path / "hang-55.dmp")]
        assert not calls


@pytest.mark.skipif(not Path("/proc/self/exe").exists(), reason="needs Linux /proc")
def test_boot_pid_finds_qgc_inside_its_wrappers(tmp_path):
    """xvfb-run and the AppImage runtime start QGC; gdb must attach to QGC, not the wrapper."""
    with subprocess.Popen(["sh", "-c", "sleep 60 & wait"], start_new_session=True) as wrapper:
        try:
            appimage = Path("sleep-x86_64.AppImage")
            deadline = time.monotonic() + 5
            pid = wrapper.pid
            while pid == wrapper.pid and time.monotonic() < deadline:
                pid = verify_executable._boot_pid(wrapper.pid, appimage)
                time.sleep(0.05)
            assert pid != wrapper.pid
            assert Path(os.readlink(f"/proc/{pid}/exe")).name == "sleep"
            assert verify_executable._boot_pid(wrapper.pid, Path("QGroundControl")) == wrapper.pid
        finally:
            os.killpg(wrapper.pid, signal.SIGKILL)


def _write_report(path: Path, lines: int, mtime: float) -> None:
    path.write_text("".join(f"report line {number}\n" for number in range(1, lines + 1)))
    os.utime(path, (mtime, mtime))


def test_macos_failure_keeps_new_crash_reports_and_prints_the_newest(tmp_path, monkeypatch, capsys):
    home = tmp_path / "home"
    reports = home / "Library" / "Logs" / "DiagnosticReports"
    reports.mkdir(parents=True)
    monkeypatch.setenv("HOME", str(home))
    monkeypatch.setenv("USERPROFILE", str(home))
    now = time.time()
    _write_report(reports / "QGroundControl-2026-10-01.ips", 5, now - 3600)
    _write_report(reports / "QGroundControl-2026-10-23-1.crash", 5, now - 5)
    _write_report(reports / "QGroundControl-2026-10-23-2.ips", 100, now)
    _write_report(reports / "Finder-2026-10-23.ips", 5, now)
    diagnostics = tmp_path / "diagnostics"
    diagnostics.mkdir()
    monkeypatch.setattr(verify_executable.platform, "system", lambda: "Darwin")

    verify_executable._save_failure_diagnostics(
        Path("QGroundControl"), "boot output", diagnostics, exit_code=-11, started=now - 60
    )

    assert sorted(path.name for path in diagnostics.iterdir()) == [
        "QGroundControl-2026-10-23-1.crash",
        "QGroundControl-2026-10-23-2.ips",
        "boot-output.txt",
    ]
    out = capsys.readouterr().out
    group = out[out.index("::group::Crash report QGroundControl-2026-10-23-2.ips") :]
    assert "report line 60\n" in group
    assert "report line 61" not in group


def test_macos_without_a_crash_report_says_so(tmp_path, monkeypatch, capsys):
    monkeypatch.setenv("HOME", str(tmp_path))
    monkeypatch.setenv("USERPROFILE", str(tmp_path))
    monkeypatch.setattr(verify_executable, "CRASH_REPORT_WAIT_SECONDS", 0)
    monkeypatch.setattr(verify_executable.platform, "system", lambda: "Darwin")
    verify_executable._save_failure_diagnostics(
        Path("QGroundControl"), "", tmp_path, exit_code=-6, started=0
    )
    assert "No QGroundControl crash report" in capsys.readouterr().out


@pytest.mark.parametrize(("dump", "pdb"), [(True, True), (True, False), (False, True)])
def test_windows_failure_keeps_the_pdb_beside_dumps(tmp_path, monkeypatch, capsys, dump, pdb):
    build = tmp_path / "Release"
    build.mkdir()
    binary = build / "QGroundControl.exe"
    binary.write_text("exe")
    if pdb:
        (build / "QGroundControl.pdb").write_text("symbols")
    diagnostics = tmp_path / "diagnostics"
    diagnostics.mkdir()
    if dump:
        (diagnostics / "QGC-crash-42.dmp").write_bytes(b"MDMP")
    monkeypatch.setattr(verify_executable.platform, "system", lambda: "Windows")

    verify_executable._save_failure_diagnostics(
        binary, "QGC: unhandled SEH", diagnostics, exit_code=3221225477, started=0
    )

    assert (diagnostics / "QGroundControl.pdb").exists() == (dump and pdb)
    out = capsys.readouterr().out
    assert ("Minidumps: QGC-crash-42.dmp" in out) == dump
    assert ("QGC wrote no crash dump" in out) == (not dump)


@pytest.mark.skipif(sys.platform != "win32", reason="MiniDumpWriteDump is Windows-only")
def test_write_minidump_of_a_running_process(tmp_path):
    with subprocess.Popen([sys.executable, "-c", "import time; time.sleep(60)"]) as child:
        try:
            path = tmp_path / "hang.dmp"
            assert verify_executable._write_minidump(child.pid, path).startswith("Wrote minidump")
            assert path.read_bytes()[:4] == b"MDMP"
        finally:
            child.kill()


def test_minidump_errors_are_reported_not_raised(tmp_path, monkeypatch):
    """The timeout hook must not raise: the hung process is killed after it either way."""

    def fail(pid, path):
        raise FileNotFoundError("dbghelp.dll")

    monkeypatch.setattr(verify_executable.platform, "system", lambda: "Windows")
    monkeypatch.setattr(verify_executable, "_write_minidump", fail)
    assert verify_executable._capture_stacks(55, Path("QGroundControl.exe"), tmp_path) == (
        "Stack capture failed: dbghelp.dll"
    )
