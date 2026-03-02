"""Tests for android_boot_test.py helpers."""

from __future__ import annotations

import subprocess
from unittest.mock import patch

import android_boot_test as mod


def _cp(stdout: bytes, returncode: int = 0) -> subprocess.CompletedProcess[bytes]:
    return subprocess.CompletedProcess(args=["adb"], returncode=returncode, stdout=stdout, stderr=b"")


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
            _cp(
                b"02-24 10:00:00.000 I/Tag( 1): one\n"
                b"02-24 10:00:00.001 I/Tag( 1): two\n"
            ),
            _cp(
                b"02-24 10:00:00.001 I/Tag( 1): two\n"
                b"02-24 10:00:00.002 I/Tag( 1): three\n"
            ),
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
