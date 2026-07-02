"""Tests for android_boot_test.py helpers."""

from __future__ import annotations

import re
import subprocess
from unittest.mock import patch

import android_boot_test as mod


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

    def fake_run_command(cmd: list[str], check: bool = False) -> subprocess.CompletedProcess[bytes]:
        del check
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
    def fake(cmd: list[str], check: bool = False) -> subprocess.CompletedProcess[bytes]:
        del check
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


class TestRunBootAttempt:
    def _kwargs(self):
        return {
            "package_name": "org.mavlink.qgroundcontrol",
            "timeout": 3,
            "stability_window": 2,
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
