"""Tests for verify_executable.py."""

from __future__ import annotations

import argparse
import os
import shutil
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

from .._helpers import load_workflow

pytestmark = pytest.mark.windows


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
        assert _boot_test(tmp_path / "missing", cwd=tmp_path, timeout=5, headless=True) == 1
        assert "::error::Binary not found" in capsys.readouterr().out

    @pytest.mark.parametrize(
        ("result", "expected"),
        [
            ((0, "booted\n"), ["booted", "Boot test passed"]),
            ((2, "crashed\n"), ["crashed", "::error::Boot test failed (exit code: 2)"]),
        ],
    )
    def test_prints_output_and_result(self, tmp_path, monkeypatch, capsys, result, expected):
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        monkeypatch.setattr(verify_executable, "_execute", lambda *args: result)
        assert _boot_test(binary, cwd=tmp_path, timeout=5, headless=True) == result[0]
        out = capsys.readouterr().out
        assert all(line in out for line in expected)

    def test_passing_output_is_folded(self, tmp_path, monkeypatch, capsys) -> None:
        binary = tmp_path / "QGroundControl"
        binary.write_text("binary")
        monkeypatch.setattr(verify_executable, "_execute", lambda *args: (0, "startup log\n"))
        assert _boot_test(binary, cwd=tmp_path, timeout=5, headless=True) == 0
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
        assert _boot_test(binary, cwd=tmp_path, timeout=5, headless=True) == 124
        out = capsys.readouterr().out
        assert out.index("log before hang") < out.index("::error::Boot test timed out after 5s")


@pytest.mark.parametrize(
    ("exe_type", "name", "headless"),
    [("binary", "QGroundControl", True), ("appimage", "QGroundControl-x86_64.AppImage", False)],
)
def test_verify_boot_tests_executable_in_working_dir(
    tmp_path, monkeypatch, exe_type, name, headless
):
    work_dir = tmp_path / "work"
    work_dir.mkdir()
    binary = work_dir / name
    binary.write_text("binary")
    binary.chmod(0o644)
    calls = []
    monkeypatch.setattr(
        verify_executable, "_boot_test", lambda *args, **kwargs: calls.append((args, kwargs)) or 0
    )
    args = argparse.Namespace(
        working_dir=str(work_dir),
        build_dir="",
        expect_archs="",
        exe_type=exe_type,
        expected_appimage_update_information="",
        timeout=60,
    )

    assert _verify(tmp_path / "elsewhere" / name, args) == 0

    assert calls == [((binary.resolve(),), {"cwd": work_dir, "timeout": 60, "headless": headless})]
    if os.name != "nt":
        assert os.access(binary, os.X_OK)


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

    def execute(self, cmd, env, cwd, timeout):
        self.calls.append(cmd)
        self.boots.append((cmd, env))
        return self.boot_rc, ""

    def commands(self, program: str) -> list[list[str]]:
        return [cmd for cmd in self.calls if cmd[0] == program]


def _run_dmg_main(monkeypatch, tmp_path, tools: FakeDmgTools, *, create_dmg=True):
    dmg = tmp_path / "QGroundControl.dmg"
    if create_dmg:
        dmg.write_bytes(b"dmg")
    runner_temp = tmp_path / "runner-temp"
    runner_temp.mkdir()
    monkeypatch.setenv("RUNNER_TEMP", str(runner_temp))
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
        assert not any((tmp_path / "runner-temp").iterdir())
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
