"""Tests for android_boot_test.py helpers."""

from __future__ import annotations

import re
import subprocess
from pathlib import Path
from typing import TYPE_CHECKING, Any
from unittest.mock import patch

from ci.android import android_boot_test as mod

from .._helpers import get_step, get_steps, load_action

if TYPE_CHECKING:
    import pytest


def _cp(stdout: bytes, returncode: int = 0) -> subprocess.CompletedProcess[bytes]:
    return subprocess.CompletedProcess(
        args=["adb"], returncode=returncode, stdout=stdout, stderr=b""
    )


def test_extract_logcat_timestamp_parses_expected_prefix() -> None:
    line = "02-24 10:00:00.123 I/Tag( 123): hello"
    assert mod._extract_logcat_timestamp(line) == "02-24 10:00:00.123"
    assert mod._extract_logcat_timestamp("not a log line") is None


def test_app_running_from_process_table_matches_package_and_suffix() -> None:
    process_table = "\n".join(
        [
            "u0_a100 1234 567 0 0 S com.example.other",
            "u0_a101 4321 567 0 0 S org.mavlink.qgroundcontrol:qt",
        ]
    )
    assert mod.app_running_from_process_table("org.mavlink.qgroundcontrol", process_table) is True
    assert mod.app_running_from_process_table("org.mavlink.missing", process_table) is False


def test_logcat_poller_only_emits_new_lines_for_repeated_timestamp_window() -> None:
    responses = iter(
        [
            _cp(b"02-24 10:00:00.000 I/Tag( 1): one\n02-24 10:00:00.001 I/Tag( 1): two\n"),
            _cp(b"02-24 10:00:00.001 I/Tag( 1): two\n02-24 10:00:00.002 I/Tag( 1): three\n"),
        ]
    )
    observed_cmds: list[list[str]] = []

    def fake_run_command(cmd: list[str], **_kwargs: object) -> subprocess.CompletedProcess[bytes]:
        observed_cmds.append(cmd)
        return next(responses)

    with patch.object(mod, "run_command", side_effect=fake_run_command):
        poller = mod.LogcatPoller()
        content1, delta1 = poller.poll()
        content2, delta2 = poller.poll()

    assert "one" in content1
    assert "two" in delta1
    assert "three" in delta2
    assert "two" not in delta2
    assert content2.splitlines()[-1].endswith("three")
    assert observed_cmds[1][-2:] == ["-T", "02-24 10:00:00.001"]


_APP_PATTERN = re.compile(r"QGroundControl")
_LAUNCH_PATTERN = re.compile(r"ActivityManager.*START.*qgroundcontrol")
_CRASH_SIG = re.compile(r"FATAL EXCEPTION")
_CRASH_CTX = re.compile(r"qgroundcontrol")
_CRASH_LOG = re.compile(r"FATAL|qgroundcontrol")


def _make_run_command(
    canned: dict[tuple[str, ...], subprocess.CompletedProcess[bytes]],
    default: subprocess.CompletedProcess[bytes] | None = None,
):
    def fake(cmd: list[str], **_kwargs: object) -> subprocess.CompletedProcess[bytes]:
        key = tuple(cmd)
        for k, resp in canned.items():
            if key[: len(k)] == k:
                return resp
        return default or _cp(b"")

    return fake


class TestWaitForAdbReady:
    def test_returns_true_when_device_ready_and_boot_completed(self) -> None:
        canned = {
            ("adb", "get-state"): _cp(b"device\n"),
            ("adb", "shell", "getprop", "sys.boot_completed"): _cp(b"1\n"),
        }
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            assert mod.wait_for_adb_ready(timeout=5) is True

    def test_returns_false_when_timeout_expires(self) -> None:
        canned = {("adb", "get-state"): _cp(b"unauthorized\n", returncode=1)}
        clock = iter([0.0, 0.5, 1.0, 1.5, 6.0])  # final value exceeds the 5s deadline
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
            patch.object(mod.time, "monotonic", side_effect=lambda: next(clock)),
        ):
            assert mod.wait_for_adb_ready(timeout=5) is False

    def test_probes_time_out_and_count_as_not_ready(self) -> None:
        calls: list[tuple[list[str], object]] = []

        def hung(cmd: list[str], **kwargs: object) -> subprocess.CompletedProcess[bytes]:
            calls.append((cmd, kwargs.get("timeout")))
            raise subprocess.TimeoutExpired(cmd, mod.ADB_PROBE_TIMEOUT_SECONDS)

        with patch.object(mod, "run_command", side_effect=hung):
            assert mod.adb_ready() is False
        assert calls == [(["adb", "get-state"], mod.ADB_PROBE_TIMEOUT_SECONDS)]

    def test_starts_the_adb_server_before_polling(self, capsys: pytest.CaptureFixture[str]) -> None:
        calls: list[tuple[list[str], object]] = []
        ready = _make_run_command(
            {
                ("adb", "get-state"): _cp(b"device\n"),
                ("adb", "shell", "getprop", "sys.boot_completed"): _cp(b"1\n"),
            }
        )

        def fake(cmd: list[str], **kwargs: object) -> subprocess.CompletedProcess[bytes]:
            calls.append((cmd, kwargs.get("timeout")))
            if cmd == ["adb", "start-server"]:
                raise subprocess.TimeoutExpired(cmd, mod.ADB_PROBE_TIMEOUT_SECONDS)
            return ready(cmd)

        with patch.object(mod, "run_command", side_effect=fake):
            assert mod.wait_for_adb_ready(timeout=5) is True
        assert [cmd for cmd, _ in calls][:2] == [["adb", "start-server"], ["adb", "get-state"]]
        assert {timeout for _, timeout in calls} == {mod.ADB_PROBE_TIMEOUT_SECONDS}
        assert "::warning::adb start-server timed out" in capsys.readouterr().out


def test_defaults_are_the_emulator_actions_settings(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("RUNNER_TEMP", "/runner/temp")
    args = mod.parse_args(["--apk", "qgc.apk"])
    assert args.log_output == Path("/runner/temp/qgc_emulator_boot.log")
    assert (args.timeout, args.launch_retries) == (120, 2)
    assert (args.install_retries, args.install_retry_delay) == (4, 5)
    # The former `timeout 180s adb wait-for-device` plus --adb-ready-timeout 180.
    assert args.adb_ready_timeout == 360
    action = get_step(get_steps(load_action("android-emulator-test")), "Emulator Boot Test")
    assert action["with"]["script"] == (
        'python3 tools/ci/android/android_boot_test.py --apk "$APK_PATH" --package "$APP_PACKAGE"'
    )


def test_failure_report_tolerates_a_hung_logcat(capsys: pytest.CaptureFixture[str]) -> None:
    def hung(cmd: list[str], **kwargs: object) -> subprocess.CompletedProcess[bytes]:
        assert kwargs["timeout"] == mod.LOGCAT_TIMEOUT_SECONDS
        raise subprocess.TimeoutExpired(cmd, mod.LOGCAT_TIMEOUT_SECONDS)

    with patch.object(mod, "run_command", side_effect=hung):
        assert mod.read_logcat() == ""
    assert "::warning::adb logcat timed out" in capsys.readouterr().out


class TestRunBootAttempt:
    def _run(self, logcat: list[str], processes: list[bool]):
        """Run an attempt that reads each logcat delta in turn while the process table says *processes*."""
        content: list[str] = []

        def poll(_self):
            delta = logcat.pop(0) if logcat else ""
            content.append(delta)
            return "\n".join(content), delta

        with (
            patch.object(mod, "run_command", return_value=_cp(b"")),
            patch.object(mod.LogcatPoller, "poll", poll),
            patch.object(mod, "get_process_table", return_value=""),
            patch.object(mod, "app_running_from_process_table", side_effect=processes),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            return mod.run_boot_attempt(**self._kwargs())

    def test_a_running_app_must_log_the_completion_marker(self) -> None:
        ok, reason, notice, _, _ = self._run(["QGroundControl launch"], [True, True, True])
        assert ok is False
        assert reason == (
            "QGroundControl is running but did not log 'Simple boot test completed' within 3s"
        )
        assert notice is not None
        assert "-C .github/actions/android-emulator-test/boot-test.cmake" in notice

    def test_passes_once_the_completion_marker_follows_the_launch(self) -> None:
        ok, reason, _, _, _ = self._run(
            ["QGroundControl launch", "Simple boot test completed - Main"], [True, True, True]
        )
        assert (ok, reason) == (True, None)

    def test_fails_when_qgc_reports_a_failed_boot_test(self) -> None:
        ok, reason, _, content, _ = self._run(
            [
                "QGroundControl launch",
                "Warning: rtph264depay missing - Video.GStreamer.SelfTest - (run:12)\n"
                "Critical: Simple boot test failed - Main - (main:48)",
            ],
            [True, True, True],
        )
        assert (ok, reason) == (False, "QGroundControl's simple boot test failed")
        assert "Video.GStreamer.SelfTest" in content

    def test_fails_when_the_app_exits_without_the_completion_marker(self) -> None:
        kwargs = self._kwargs()
        kwargs["timeout"] = 10
        with (
            patch.object(mod, "run_command", return_value=_cp(b"")),
            patch.object(
                mod.LogcatPoller,
                "poll",
                return_value=("QGroundControl launch", "QGroundControl launch"),
            ),
            patch.object(mod, "get_process_table", return_value=""),
            patch.object(mod, "app_running_from_process_table", side_effect=[True] + [False] * 9),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, _, _, log_pattern = mod.run_boot_attempt(**kwargs)
        assert ok is False
        assert reason == "App process exited before logging 'Simple boot test completed'"
        assert log_pattern is _CRASH_LOG

    def _kwargs(self) -> dict[str, Any]:
        return {
            "package_name": "org.mavlink.qgroundcontrol",
            "timeout": 3,
            "app_launch_pattern": _LAUNCH_PATTERN,
            "app_log_pattern": _APP_PATTERN,
            "crash_signature_pattern": _CRASH_SIG,
            "crash_context_pattern": _CRASH_CTX,
            "crash_log_pattern": _CRASH_LOG,
        }

    def test_returns_failure_when_monkey_launch_exits_nonzero(self) -> None:
        canned = {
            ("adb", "shell", "am", "force-stop"): _cp(b""),
            ("adb", "logcat", "-c"): _cp(b""),
            ("adb", "shell", "monkey"): _cp(b"", returncode=1),
            ("adb", "logcat", "-d"): _cp(b"some logcat\n"),
        }
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, notice, _, _ = mod.run_boot_attempt(**self._kwargs())
        assert ok is False
        assert reason is not None and "monkey" in reason
        assert notice is None

    def test_returns_success_when_completion_marker_seen(self) -> None:
        completion_logcat = b"02-24 10:00:00.000 I/Tag( 1): Simple boot test completed\n"
        canned = {
            ("adb", "shell", "am", "force-stop"): _cp(b""),
            ("adb", "logcat", "-c"): _cp(b""),
            ("adb", "shell", "monkey"): _cp(b""),
            ("adb", "logcat", "-d"): _cp(completion_logcat),
            ("adb", "logcat"): _cp(completion_logcat),
            ("adb", "shell", "ps", "-A"): _cp(b""),
        }
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, notice, _, _ = mod.run_boot_attempt(**self._kwargs())
        assert ok is True
        assert reason is None and notice is None

    def test_returns_failure_when_crash_detected_in_logcat(self) -> None:
        crash_line = b"02-24 10:00:00.000 E/AndroidRuntime( 1): FATAL EXCEPTION qgroundcontrol\n"
        canned = {
            ("adb", "shell", "am", "force-stop"): _cp(b""),
            ("adb", "logcat", "-c"): _cp(b""),
            ("adb", "shell", "monkey"): _cp(b""),
            ("adb", "logcat", "-d"): _cp(crash_line),
            ("adb", "logcat"): _cp(crash_line),
            ("adb", "shell", "ps", "-A"): _cp(b""),
        }
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, _, _, log_pattern = mod.run_boot_attempt(**self._kwargs())
        assert ok is False
        assert reason == "Crash detected during boot test"
        assert log_pattern is _CRASH_LOG

    def test_returns_failure_on_timeout_with_no_app_activity(self) -> None:
        canned = {
            ("adb", "shell", "am", "force-stop"): _cp(b""),
            ("adb", "logcat", "-c"): _cp(b""),
            ("adb", "shell", "monkey"): _cp(b""),
            ("adb", "logcat", "-d"): _cp(b""),
            ("adb", "logcat"): _cp(b""),
            ("adb", "shell", "ps", "-A"): _cp(b""),
        }
        with (
            patch.object(mod, "run_command", side_effect=_make_run_command(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, notice, _, _ = mod.run_boot_attempt(**self._kwargs())
        assert ok is False
        assert reason is not None and "timed out" in reason
        assert notice is None


def test_boot_test_report_keeps_the_last_failure_and_self_test_lines() -> None:
    lines = [
        f"Warning: check {index} failed - Video.GStreamer.SelfTest - (run:1)" for index in range(30)
    ]
    logcat = "\n".join(
        ["QGroundControl launch", *lines, "Critical: Simple boot test failed - Main - (main:48)"]
    )
    report = mod.boot_test_report(logcat)
    assert len(report) == mod.MAX_BOOT_TEST_REPORT_LINES
    assert report[-1] == "Critical: Simple boot test failed - Main - (main:48)"
    assert report[0] == lines[30 - mod.MAX_BOOT_TEST_REPORT_LINES + 1]
    assert mod.boot_test_report("QGroundControl launch\nStarting application event loop") == []


def test_failure_annotation_quotes_the_boot_test_report(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    logcat = (
        "10-31 03:21:22.448 W/QGroundControl( 1234): Warning: rtph264depay missing - "
        "Video.GStreamer.SelfTest - (run:12)\n"
        "10-31 03:21:22.449 E/QGroundControl( 1234): Critical: Simple boot test failed - Main - "
        "(main:48)"
    )
    attempt = (False, "QGroundControl's simple boot test failed", None, logcat, _APP_PATTERN)
    with (
        patch.object(mod, "wait_for_adb_ready", return_value=True),
        patch.object(mod, "install_with_retries", return_value=True),
        patch.object(mod, "grant_all_files_access") as grant,
        patch.object(mod, "run_boot_attempt", return_value=attempt) as run_attempt,
        patch.object(mod.time, "sleep", lambda *_: None),
    ):
        exit_code = mod.main(["--apk", "qgc.apk", "--log-output", str(tmp_path / "boot.log")])
    assert exit_code == 1
    grant.assert_called_once_with("org.mavlink.qgroundcontrol")
    assert run_attempt.call_count == 2
    out = capsys.readouterr().out
    assert (
        "::error::QGroundControl's simple boot test failed%0A"
        "10-31 03:21:22.448 W/QGroundControl( 1234): Warning: rtph264depay missing - "
        "Video.GStreamer.SelfTest - (run:12)%0A"
        "10-31 03:21:22.449 E/QGroundControl( 1234): Critical: Simple boot test failed - Main - "
        "(main:48)\n"
    ) in out
    assert (tmp_path / "boot.log").read_text() == logcat


_GRANT_COMMAND = [
    "adb",
    "shell",
    "appops",
    "set",
    "--uid",
    "org.mavlink.qgroundcontrol",
    "MANAGE_EXTERNAL_STORAGE",
    "allow",
]


class TestGrantAllFilesAccess:
    def test_sets_the_app_op_for_the_package(self, capsys: pytest.CaptureFixture[str]) -> None:
        calls: list[tuple[list[str], object]] = []

        def fake(cmd: list[str], **kwargs: object) -> subprocess.CompletedProcess[bytes]:
            calls.append((cmd, kwargs.get("timeout")))
            return _cp(b"")

        with patch.object(mod, "run_command", side_effect=fake):
            mod.grant_all_files_access("org.mavlink.qgroundcontrol")
        assert calls == [(_GRANT_COMMAND, mod.ADB_PROBE_TIMEOUT_SECONDS)]
        assert "::warning" not in capsys.readouterr().out

    def test_a_failed_grant_only_warns(self, capsys: pytest.CaptureFixture[str]) -> None:
        failed = subprocess.CompletedProcess(
            args=_GRANT_COMMAND, returncode=255, stdout=b"", stderr=b"Unknown operation string"
        )
        with patch.object(mod, "run_command", return_value=failed):
            mod.grant_all_files_access("org.mavlink.qgroundcontrol")
        assert (
            "::warning::Cannot grant org.mavlink.qgroundcontrol all-files access: "
            "Unknown operation string"
        ) in capsys.readouterr().out

    def test_a_hung_grant_only_warns(self, capsys: pytest.CaptureFixture[str]) -> None:
        hung = subprocess.TimeoutExpired(_GRANT_COMMAND, mod.ADB_PROBE_TIMEOUT_SECONDS)
        with patch.object(mod, "run_command", side_effect=hung):
            mod.grant_all_files_access("org.mavlink.qgroundcontrol")
        assert "::warning::Granting org.mavlink.qgroundcontrol all-files access timed out" in (
            capsys.readouterr().out
        )
