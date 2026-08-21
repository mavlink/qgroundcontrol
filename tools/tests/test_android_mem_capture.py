#!/usr/bin/env python3
"""Tests for tools/android_mem_capture.py."""

from __future__ import annotations

import argparse
import subprocess

import android_mem_capture as amc
import pytest


def _completed(
    stdout: str = "", stderr: str = "", returncode: int = 0
) -> subprocess.CompletedProcess[str]:
    return subprocess.CompletedProcess(
        args=["adb"], returncode=returncode, stdout=stdout, stderr=stderr
    )


def test_parse_duration_accepts_seconds_minutes_hours() -> None:
    assert amc._parse_duration("600") == 600
    assert amc._parse_duration("90s") == 90
    assert amc._parse_duration("30m") == 1800
    assert amc._parse_duration("1.5h") == 5400


def test_parse_duration_rejects_garbage() -> None:
    for text in ("", "abc", "10x", "-5", "1h30m"):
        with pytest.raises(argparse.ArgumentTypeError):
            amc._parse_duration(text)


DUMPSYS_OUTPUT = """\
** MEMINFO in pid 12345 [org.mavlink.qgroundcontrol] **
                   Pss  Private
        EGL mtrack    81000        0
         GL mtrack    23000        0
             TOTAL   268000   210000

 App Summary
           Java Heap:    14000
         Native Heap:    98000
            Graphics:   104000
               TOTAL PSS:   268000
"""


def test_sample_meminfo_parses_pss_breakdown(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(amc, "_adb", lambda serial, *args: DUMPSYS_OUTPUT)

    sample = amc._sample_meminfo(None, "org.mavlink.qgroundcontrol")

    assert sample == {
        "total_pss_kb": 268000,
        "java_heap_kb": 14000,
        "native_heap_kb": 98000,
        "graphics_kb": 104000,
        "egl_mtrack_kb": 81000,
        "gl_mtrack_kb": 23000,
    }


def test_sample_meminfo_missing_fields_are_omitted(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(amc, "_adb", lambda serial, *args: "No process found\n")

    assert amc._sample_meminfo(None, "org.mavlink.qgroundcontrol") == {}


def test_pidof_returns_first_pid(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(amc, "_run_adb", lambda serial, *args: _completed(stdout="4242 4243\n"))

    assert amc._pidof(None, "pkg") == 4242


def test_pidof_none_when_process_not_running(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(amc, "_run_adb", lambda serial, *args: _completed(returncode=1))

    assert amc._pidof(None, "pkg") is None


def test_pidof_raises_on_adb_transport_error(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        amc,
        "_run_adb",
        lambda serial, *args: _completed(returncode=1, stderr="error: device offline"),
    )

    with pytest.raises(RuntimeError, match="device offline"):
        amc._pidof(None, "pkg")


def test_adb_raises_on_nonzero_exit(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(
        amc, "_run_adb", lambda serial, *args: _completed(returncode=1, stderr="boom")
    )

    with pytest.raises(RuntimeError, match="boom"):
        amc._adb(None, "shell", "true")


def test_slope_uses_second_half_of_run() -> None:
    # First half flat, second half rising 100 kB every 10s -> 600 kB/min
    points = [(float(t), 1000.0) for t in range(0, 60, 10)]
    points += [(float(t), 1000.0 + (t - 60) * 10) for t in range(60, 130, 10)]

    slope = amc._slope_kb_per_min(points)

    assert slope == pytest.approx(600, rel=0.01)


def test_slope_flat_run_is_zero() -> None:
    points = [(float(t), 5000.0) for t in range(0, 100, 10)]

    assert amc._slope_kb_per_min(points) == pytest.approx(0)


def test_slope_needs_at_least_three_points() -> None:
    assert amc._slope_kb_per_min([(0.0, 1.0), (10.0, 2.0), (20.0, 3.0), (30.0, 4.0)]) is None


def test_column_skips_rows_with_missing_values() -> None:
    rows = [
        {"time_s": "0.0", "total_pss_kb": "100"},
        {"time_s": "10.0", "total_pss_kb": ""},
        {"time_s": "", "total_pss_kb": "300"},
        {"time_s": "30.0", "total_pss_kb": "400"},
    ]

    assert amc._column(rows, "total_pss_kb") == [(0.0, 100.0), (30.0, 400.0)]


@pytest.mark.parametrize(
    "argv",
    [["--interval", "0"], ["--interval", "-5"], ["--interval", "nan"], ["--interval", "inf"]],
)
def test_main_rejects_invalid_interval(monkeypatch: pytest.MonkeyPatch, argv: list[str]) -> None:
    monkeypatch.setattr("sys.argv", ["android_mem_capture.py", *argv])

    with pytest.raises(SystemExit) as excinfo:
        amc.main()
    assert excinfo.value.code == 2


def test_main_rejects_zero_duration(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr("sys.argv", ["android_mem_capture.py", "--duration", "0"])

    with pytest.raises(SystemExit) as excinfo:
        amc.main()
    assert excinfo.value.code == 2
