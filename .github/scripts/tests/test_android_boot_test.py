"""Behavioral contracts for the Android emulator boot probe."""

from __future__ import annotations

import re
import subprocess
from unittest.mock import patch

import android_boot_test as mod


def _cp(stdout: bytes = b"", returncode: int = 0) -> subprocess.CompletedProcess[bytes]:
    return subprocess.CompletedProcess(["adb"], returncode, stdout, b"")


def _command_responder(
    canned: dict[tuple[str, ...], subprocess.CompletedProcess[bytes]],
):
    def run(cmd: list[str], check: bool = False) -> subprocess.CompletedProcess[bytes]:
        del check
        return next(
            (value for prefix, value in canned.items() if tuple(cmd[: len(prefix)]) == prefix),
            _cp(),
        )

    return run


def test_log_parsing_and_process_detection() -> None:
    assert mod._extract_logcat_timestamp("02-24 10:00:00.123 I/Tag( 123): hello") == (
        "02-24 10:00:00.123"
    )
    assert mod._extract_logcat_timestamp("not a log line") is None

    table = "u0_a100 1 S com.example.other\nu0_a101 2 S org.mavlink.qgroundcontrol:qt"
    assert mod.app_running_from_process_table("org.mavlink.qgroundcontrol", table)
    assert not mod.app_running_from_process_table("org.mavlink.missing", table)


def test_logcat_poller_deduplicates_the_timestamp_overlap() -> None:
    responses = iter(
        [
            _cp(b"02-24 10:00:00.000 I/T: one\n02-24 10:00:00.001 I/T: two\n"),
            _cp(b"02-24 10:00:00.001 I/T: two\n02-24 10:00:00.002 I/T: three\n"),
        ]
    )
    commands: list[list[str]] = []

    def run(cmd: list[str], check: bool = False) -> subprocess.CompletedProcess[bytes]:
        del check
        commands.append(cmd)
        return next(responses)

    with patch.object(mod, "run_command", side_effect=run):
        poller = mod.LogcatPoller()
        first_content, first_delta = poller.poll()
        content, delta = poller.poll()

    assert ("one" in first_content, "two" in first_delta) == (True, True)
    assert "three" in delta and "two" not in delta
    assert content.splitlines()[-1].endswith("three")
    assert commands[1][-2:] == ["-T", "02-24 10:00:00.001"]


def test_wait_for_adb_ready_covers_ready_and_timeout() -> None:
    ready = {
        ("adb", "get-state"): _cp(b"device\n"),
        ("adb", "shell", "getprop", "sys.boot_completed"): _cp(b"1\n"),
    }
    with (
        patch.object(mod, "run_command", side_effect=_command_responder(ready)),
        patch.object(mod.time, "sleep", lambda *_: None),
    ):
        assert mod.wait_for_adb_ready(5)

    clock = iter([0.0, 0.5, 1.0, 1.5, 6.0])
    unavailable = {("adb", "get-state"): _cp(b"unauthorized\n", 1)}
    with (
        patch.object(mod, "run_command", side_effect=_command_responder(unavailable)),
        patch.object(mod.time, "sleep", lambda *_: None),
        patch.object(mod.time, "monotonic", side_effect=lambda: next(clock)),
    ):
        assert not mod.wait_for_adb_ready(5)


def test_boot_attempt_terminal_outcomes() -> None:
    patterns = {
        "app_launch_pattern": re.compile(r"ActivityManager.*START.*qgroundcontrol"),
        "app_log_pattern": re.compile(r"QGroundControl"),
        "crash_signature_pattern": re.compile(r"FATAL EXCEPTION"),
        "crash_context_pattern": re.compile(r"qgroundcontrol"),
        "crash_log_pattern": re.compile(r"FATAL|qgroundcontrol"),
    }
    cases = (
        ("launch failure", b"", 1, False, "monkey", None),
        ("completion marker", b"Simple boot test completed\n", 0, True, None, None),
        (
            "relevant crash",
            b"02-24 10:00:00.000 E/AndroidRuntime: FATAL EXCEPTION qgroundcontrol\n",
            0,
            False,
            "Crash detected",
            patterns["crash_log_pattern"],
        ),
        (
            "application logs without a running process",
            b"02-24 10:00:00.000 I/QGroundControl: starting\n",
            0,
            False,
            "timed out",
            None,
        ),
        ("timeout", b"", 0, False, "timed out", None),
    )

    for name, logcat, monkey_code, expected_ok, reason_text, expected_pattern in cases:
        canned = {
            ("adb", "shell", "monkey"): _cp(returncode=monkey_code),
            ("adb", "logcat"): _cp(logcat),
            ("adb", "shell", "ps", "-A"): _cp(),
        }
        with (
            patch.object(mod, "run_command", side_effect=_command_responder(canned)),
            patch.object(mod.time, "sleep", lambda *_: None),
        ):
            ok, reason, notice, _, log_pattern = mod.run_boot_attempt(
                package_name="org.mavlink.qgroundcontrol",
                timeout=3,
                stability_window=2,
                **patterns,
            )
        assert ok is expected_ok, name
        if reason_text is None:
            assert reason is None, name
        else:
            assert reason_text in (reason or ""), name
        if name == "application logs without a running process":
            assert notice == "App launched but did not satisfy the stability/marker checks."
        else:
            assert notice is None, name
        if expected_pattern is not None:
            assert log_pattern is expected_pattern, name
