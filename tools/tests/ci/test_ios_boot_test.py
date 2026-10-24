"""Exercise simulator lifecycle and QGC success-marker validation without Xcode."""

import json
import os
import plistlib
import subprocess
import time
from types import SimpleNamespace
from unittest.mock import ANY, patch

import pytest
from ci.ios import ios_boot_test
from ci.ios.ios_boot_test import (
    CLEANUP_TIMEOUT,
    DEFAULT_APP_TIMEOUT,
    DEFAULT_BOOT_TIMEOUT,
    DEFAULT_TOTAL_TIMEOUT,
    DIAGNOSTIC_TIMEOUT,
    HOST_LOG_PREDICATE,
    LOG_SHOW_TIMEOUT,
    MAX_BOOT_ATTEMPTS,
    MAX_BOOT_TIMEOUT,
    SIMULATOR_LOG_PREDICATE,
    Progress,
    boot_test,
    main,
    select_device,
)

DEVICES = {
    "devices": {
        "com.apple.CoreSimulator.SimRuntime.iOS-18-5": [
            {"name": "iPhone 16", "isAvailable": True, "deviceTypeIdentifier": "iphone16"}
        ]
    }
}
SUCCESS = "Simple boot test completed"


@pytest.fixture
def app(tmp_path):
    path = tmp_path / "QGC.app"
    path.mkdir()
    (path / "Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "org.qgc"}))
    return path


@pytest.fixture(autouse=True)
def crash_reports(tmp_path, monkeypatch):
    """Point crash-report collection at an empty directory instead of the user's reports."""
    path = tmp_path / "DiagnosticReports"
    path.mkdir()
    monkeypatch.setattr(ios_boot_test, "CRASH_REPORT_DIR", path)
    return path


def _args(command):
    """simctl arguments, or the whole command for the host's log show."""
    return tuple(command[2:]) if command[:2] == ["xcrun", "simctl"] else tuple(command)


@pytest.fixture
def simulator():
    responses = {}
    devices = iter(("uuid-1", "uuid-2"))

    def run(command, **kwargs):
        args = _args(command)
        if args in responses:
            response = responses[args]
            if isinstance(response, Exception):
                raise response
            stdout, stderr = response
        elif args[0] == "list":
            stdout, stderr = json.dumps(DEVICES), ""
        elif args[0] == "create":
            stdout, stderr = next(devices), "create warning\n"
        elif args[0] == "launch":
            stdout, stderr = "", SUCCESS
        else:
            stdout, stderr = "", ""
        return subprocess.CompletedProcess(command, 0, stdout=stdout, stderr=stderr)

    with patch("ci.ios.ios_boot_test.subprocess.run", side_effect=run) as process:
        yield process, responses


def commands(process):
    return [_args(call.args[0]) for call in process.call_args_list]


def log_show(predicate):
    return ("log", "show", "--last", "10m", "--style", "compact", "--predicate", predicate)


def diagnostics(device):
    """Commands a failed attempt runs before cleaning up *device*."""
    return [
        ("list", "devices", "--json"),
        ("spawn", device, *log_show(SIMULATOR_LOG_PREDICATE)),
        log_show(HOST_LOG_PREDICATE),
    ]


def launch_command(device):
    return (
        "launch",
        "--console",
        "--terminate-running-process",
        device,
        "org.qgc",
        "--simple-boot-test",
        "--logging",
        "Main",
        "--log-output",
    )


@pytest.mark.parametrize(
    "output,error",
    [
        (SUCCESS, False),
        ("launch successful", True),
        ("Simple boot test failed", True),
        (f"{SUCCESS}\nSimple boot test failed", True),
        ("", True),
    ],
)
def test_app_must_report_boot_success_without_retry(app, tmp_path, simulator, output, error):
    process, responses = simulator
    responses[launch_command("uuid-1")] = ("", output)
    log = tmp_path / "log"
    if error:
        with pytest.raises(RuntimeError, match="successful"):
            boot_test(app, log)
    else:
        boot_test(app, log)
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == 1
    assert calls[calls.index(launch_command("uuid-1")) :] == [
        launch_command("uuid-1"),
        *(diagnostics("uuid-1") if error else []),
        ("shutdown", "uuid-1"),
        ("delete", "uuid-1"),
    ]
    assert output in log.read_text()
    assert "create warning" in log.read_text()
    for call in process.call_args_list:
        if call.args[0][2] in ("install", "launch"):
            assert call.kwargs["timeout"] == DEFAULT_APP_TIMEOUT
        # Diagnostic log captures keep their output whatever their exit code.
        assert call.kwargs["check"] is (_args(call.args[0])[0] not in ("spawn", "log"))


def test_missing_runtime_fails():
    with pytest.raises(RuntimeError, match="No available"):
        select_device({"devices": {}})


@pytest.mark.parametrize("phase", ["boot", "bootstatus"])
@pytest.mark.parametrize("error_type", [subprocess.TimeoutExpired, subprocess.CalledProcessError])
def test_cold_boot_failure_recovers_on_fresh_device(app, tmp_path, simulator, phase, error_type):
    process, responses = simulator
    failed_command = (phase, "uuid-1", "-b") if phase == "bootstatus" else (phase, "uuid-1")
    failure = (
        subprocess.TimeoutExpired(phase, 420, output=b"cold boot stalled\n", stderr="boot error\n")
        if error_type is subprocess.TimeoutExpired
        else subprocess.CalledProcessError(
            1, phase, output="boot refused\n", stderr=b"boot error\n"
        )
    )
    responses[failed_command] = failure
    log = tmp_path / "log"
    log.write_text("stale previous run")
    boot_test(app, log, boot_timeout=420)
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == 2
    failure_index = calls.index(failed_command)
    assert calls[failure_index + 1 : failure_index + 6] == [
        *diagnostics("uuid-1"),
        ("shutdown", "uuid-1"),
        ("delete", "uuid-1"),
    ]
    assert calls[failure_index + 6][0] == "create"
    assert [command for command in calls if command[0] == "install"] == [
        ("install", "uuid-2", str(app))
    ]
    assert [command for command in calls if command[0] == "launch"] == [launch_command("uuid-2")]
    assert calls[-2:] == [("shutdown", "uuid-2"), ("delete", "uuid-2")]
    text = log.read_text()
    for expected in (
        "attempt 1/2",
        "attempt 2/2",
        "boot error",
        str(failure),
        "Simulator state before cleanup of uuid-1",
        json.dumps(DEVICES),
        "Retrying with a fresh simulator after the boot failure of attempt 1/2",
        SUCCESS,
    ):
        assert expected in text
    assert "stale previous run" not in text
    for call in process.call_args_list:
        if call.args[0][2] == "bootstatus":
            assert call.kwargs["timeout"] == 420
        elif call.args[0][2:] == ["list", "devices", "--json"]:
            assert call.kwargs["timeout"] == DIAGNOSTIC_TIMEOUT
        elif call.args[0][2] in ("shutdown", "delete"):
            assert call.kwargs["timeout"] == CLEANUP_TIMEOUT


@pytest.mark.parametrize("attempts", [1, 2])
def test_exhausted_boot_attempts_retain_all_diagnostics(app, tmp_path, simulator, attempts):
    process, responses = simulator
    failures = [
        subprocess.TimeoutExpired("bootstatus", DEFAULT_BOOT_TIMEOUT, output=f"stalled {attempt}")
        for attempt in range(1, attempts + 1)
    ]
    for attempt, failure in enumerate(failures, 1):
        responses[("bootstatus", f"uuid-{attempt}", "-b")] = failure
    log = tmp_path / "log"
    with pytest.raises(subprocess.TimeoutExpired) as raised:
        boot_test(app, log, boot_attempts=attempts)
    assert raised.value is failures[-1]
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == attempts
    assert not any(command[0] in ("install", "launch") for command in calls)
    for attempt in range(1, attempts + 1):
        assert ("delete", f"uuid-{attempt}") in calls
        assert f"stalled {attempt}" in log.read_text()
        assert f"Simulator state before cleanup of uuid-{attempt}" in log.read_text()


@pytest.mark.parametrize("timeout", [300, 420])
@pytest.mark.parametrize("error_type", [subprocess.TimeoutExpired, subprocess.CalledProcessError])
def test_install_failure_recovers_on_fresh_device(app, tmp_path, simulator, timeout, error_type):
    process, responses = simulator
    failure = (
        subprocess.TimeoutExpired("install", timeout, output=b"copying", stderr="install stalled")
        if error_type is subprocess.TimeoutExpired
        else subprocess.CalledProcessError(1, "install", stderr=b"install stalled")
    )
    failed_command = ("install", "uuid-1", str(app))
    responses[failed_command] = failure
    log = tmp_path / "log"
    boot_test(app, log, timeout=timeout)
    calls = commands(process)
    failure_index = calls.index(failed_command)
    assert calls[failure_index + 1 : failure_index + 6] == [
        *diagnostics("uuid-1"),
        ("shutdown", "uuid-1"),
        ("delete", "uuid-1"),
    ]
    assert calls[failure_index + 6][0] == "create"
    assert [command for command in calls if command[0] == "launch"] == [launch_command("uuid-2")]
    assert calls[-2:] == [("shutdown", "uuid-2"), ("delete", "uuid-2")]
    for call in process.call_args_list:
        if call.args[0][2] in ("install", "launch"):
            assert call.kwargs["timeout"] == timeout
    text = log.read_text()
    for expected in (
        "install stalled",
        "attempt 1/2 failed during install",
        "Retrying with a fresh simulator after the install failure of attempt 1/2",
        "Simulator state before cleanup of uuid-1",
        SUCCESS,
    ):
        assert expected in text


def test_exhausted_install_attempts_raise_last_failure(app, tmp_path, simulator):
    process, responses = simulator
    failures = [
        subprocess.TimeoutExpired("install", DEFAULT_APP_TIMEOUT, output=f"install {attempt}")
        for attempt in range(1, MAX_BOOT_ATTEMPTS + 1)
    ]
    for attempt, failure in enumerate(failures, 1):
        responses[("install", f"uuid-{attempt}", str(app))] = failure
    log = tmp_path / "log"
    with pytest.raises(subprocess.TimeoutExpired) as raised:
        boot_test(app, log)
    assert raised.value is failures[-1]
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == MAX_BOOT_ATTEMPTS
    assert not any(command[0] == "launch" for command in calls)
    assert calls[-1] == ("delete", f"uuid-{MAX_BOOT_ATTEMPTS}")
    for failure in failures:
        assert failure.output in log.read_text()


@pytest.mark.parametrize(
    "failure",
    [
        subprocess.TimeoutExpired("launch", 600, output=b"hung", stderr="application error"),
        subprocess.TimeoutExpired("launch", 600, stderr=b"application error"),
        subprocess.CalledProcessError(1, "launch", output=SUCCESS, stderr=b"application error"),
        subprocess.CalledProcessError(1, "launch", stderr="application error"),
    ],
)
def test_launch_failure_with_output_is_not_retried(app, tmp_path, simulator, failure):
    process, responses = simulator
    responses[launch_command("uuid-1")] = failure
    log = tmp_path / "log"
    with pytest.raises(type(failure)) as raised:
        boot_test(app, log)
    assert raised.value is failure
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == 1
    assert calls[-2:] == [("shutdown", "uuid-1"), ("delete", "uuid-1")]
    assert "application error" in log.read_text()
    assert "failed during launch" in log.read_text()
    assert "Retrying" not in log.read_text()


@pytest.mark.parametrize("output", [None, b"", " \n"])
def test_silent_launch_timeout_recovers_on_fresh_device(app, tmp_path, simulator, output):
    process, responses = simulator
    responses[launch_command("uuid-1")] = subprocess.TimeoutExpired(
        "launch", DEFAULT_APP_TIMEOUT, output=output, stderr=output
    )
    log = tmp_path / "log"
    boot_test(app, log)
    calls = commands(process)
    assert [command for command in calls if command[0] == "launch"] == [
        launch_command("uuid-1"),
        launch_command("uuid-2"),
    ]
    assert calls[-2:] == [("shutdown", "uuid-2"), ("delete", "uuid-2")]
    text = log.read_text()
    assert "Retrying with a fresh simulator after the launch failure of attempt 1/2" in text
    assert SUCCESS in text


def test_silent_launch_timeout_on_last_attempt_is_raised(app, tmp_path, simulator):
    process, responses = simulator
    failure = subprocess.TimeoutExpired("launch", DEFAULT_APP_TIMEOUT)
    responses[launch_command("uuid-1")] = failure
    with pytest.raises(subprocess.TimeoutExpired) as raised:
        boot_test(app, tmp_path / "log", boot_attempts=1)
    assert raised.value is failure
    assert commands(process)[-1] == ("delete", "uuid-1")


@pytest.mark.parametrize("output", [b"partial \xff stdout", "partial stdout", None])
@pytest.mark.parametrize("stderr", [b"partial \xff stderr", "partial stderr", None])
def test_partial_timeout_output_is_retained(app, tmp_path, simulator, output, stderr):
    _, responses = simulator
    responses[("bootstatus", "uuid-1", "-b")] = subprocess.TimeoutExpired(
        "bootstatus", 600, output=output, stderr=stderr
    )
    log = tmp_path / "log"
    with pytest.raises(subprocess.TimeoutExpired):
        boot_test(app, log, boot_attempts=1)
    text = log.read_text()
    if output is not None:
        assert (
            output.decode("utf-8", errors="replace") if isinstance(output, bytes) else output
        ) in text
    if stderr is not None:
        assert (
            stderr.decode("utf-8", errors="replace") if isinstance(stderr, bytes) else stderr
        ) in text
    assert "TimeoutExpired" in text


@pytest.mark.parametrize(
    "diagnostic_failure",
    [
        subprocess.TimeoutExpired("list", 30, output=b"partial device state"),
        subprocess.CalledProcessError(1, "list", stderr="device state error"),
        OSError("simctl unavailable"),
    ],
)
@pytest.mark.parametrize("attempts", [1, 2])
def test_diagnostic_failure_does_not_mask_or_block_recovery(
    app, tmp_path, simulator, diagnostic_failure, attempts
):
    process, responses = simulator
    failure = subprocess.TimeoutExpired("bootstatus", 600, output=b"original boot failure")
    responses[("bootstatus", "uuid-1", "-b")] = failure
    responses[("list", "devices", "--json")] = diagnostic_failure
    log = tmp_path / "log"
    if attempts == 1:
        with pytest.raises(subprocess.TimeoutExpired) as raised:
            boot_test(app, log, boot_attempts=attempts)
        assert raised.value is failure
    else:
        boot_test(app, log, boot_attempts=attempts)
    assert ("delete", "uuid-1") in commands(process)
    text = log.read_text()
    assert "original boot failure" in text
    assert str(diagnostic_failure) in text
    assert "Simulator state collection failed" in text
    if attempts > 1:
        assert SUCCESS in text


@pytest.mark.parametrize("phase", ["bootstatus", "install", "launch"])
def test_cleanup_failures_preserve_original_failure(app, tmp_path, simulator, phase):
    process, responses = simulator
    original = subprocess.TimeoutExpired(phase, 600, output=b"original failure")
    command = {
        "bootstatus": ("bootstatus", "uuid-1", "-b"),
        "install": ("install", "uuid-1", str(app)),
        "launch": launch_command("uuid-1"),
    }[phase]
    responses[command] = original
    responses[("shutdown", "uuid-1")] = subprocess.TimeoutExpired(
        "shutdown", 30, output=b"shutdown hung"
    )
    responses[("delete", "uuid-1")] = subprocess.CalledProcessError(
        1, "delete", stderr="delete refused"
    )
    log = tmp_path / "log"
    with pytest.raises(subprocess.TimeoutExpired) as raised:
        boot_test(app, log)
    assert raised.value is original
    calls = commands(process)
    assert calls[-2:] == [("shutdown", "uuid-1"), ("delete", "uuid-1")]
    assert sum(command[0] == "create" for command in calls) == 1
    text = log.read_text()
    for output in ("original failure", "shutdown hung", "delete refused"):
        assert output in text
    assert ("could not be deleted" in text) == (phase != "launch")


@pytest.mark.parametrize(
    "failure,annotation",
    [
        (
            subprocess.TimeoutExpired("delete", 60, output=b"delete hung"),
            "timed out after 60 seconds",
        ),
        (OSError("delete hung\n50% done"), "delete hung%0A50%25 done"),
    ],
)
@pytest.mark.parametrize("shutdown_fails", [False, True])
def test_cleanup_failure_after_app_success_warns(
    app, tmp_path, simulator, capsys, failure, annotation, shutdown_fails
):
    process, responses = simulator
    if shutdown_fails:
        responses[("shutdown", "uuid-1")] = subprocess.TimeoutExpired("shutdown", 60)
    responses[("delete", "uuid-1")] = failure
    log = tmp_path / "log"
    boot_test(app, log)
    assert sum(command[0] == "create" for command in commands(process)) == 1
    text = log.read_text()
    for expected in (SUCCESS, "delete hung", "cleanup failed after a successful boot test"):
        assert expected in text
    warnings = [line for line in capsys.readouterr().out.splitlines() if line.startswith("::")]
    assert len(warnings) == 1
    assert warnings[0].startswith(
        "::warning::iOS simulator cleanup failed after a successful boot test: "
    )
    assert annotation in warnings[0]


def test_shutdown_failure_still_deletes_device_and_allows_boot_recovery(app, tmp_path, simulator):
    process, responses = simulator
    responses[("boot", "uuid-1")] = subprocess.CalledProcessError(1, "boot", stderr="boot refused")
    responses[("shutdown", "uuid-1")] = subprocess.CalledProcessError(
        1, "shutdown", stderr="already shutdown"
    )
    log = tmp_path / "log"
    boot_test(app, log)
    assert ("delete", "uuid-1") in commands(process)
    assert "already shutdown" in log.read_text()
    assert SUCCESS in log.read_text()


def test_launch_marker_failure_survives_cleanup_failure(app, tmp_path, simulator):
    _, responses = simulator
    responses[launch_command("uuid-1")] = ("Simple boot test failed", "")
    responses[("delete", "uuid-1")] = OSError("delete unavailable")
    with pytest.raises(RuntimeError, match="successful"):
        boot_test(app, tmp_path / "log")
    assert "delete unavailable" in (tmp_path / "log").read_text()


def test_missing_simctl_during_boot_is_not_retried(app, tmp_path, simulator):
    process, responses = simulator
    responses[("boot", "uuid-1")] = FileNotFoundError("xcrun missing")
    with pytest.raises(FileNotFoundError, match="xcrun missing"):
        boot_test(app, tmp_path / "log")
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == 1
    assert calls[-2:] == [("shutdown", "uuid-1"), ("delete", "uuid-1")]


@pytest.mark.parametrize("attempts", [1, 2])
def test_create_failure_is_retried_without_device_cleanup(app, tmp_path, simulator, attempts):
    process, responses = simulator
    runtime = next(iter(DEVICES["devices"]))
    failure = subprocess.CalledProcessError(1, "create", stderr="creation refused")
    responses[("create", "QGC CI boot test", "iphone16", runtime)] = failure
    log = tmp_path / "log"
    with pytest.raises(subprocess.CalledProcessError) as raised:
        boot_test(app, log, boot_attempts=attempts)
    assert raised.value is failure
    # Without a device only the host's CoreSimulator log is saved.
    assert commands(process) == [
        ("list", "devices", "available", "--json"),
        *[("create", "QGC CI boot test", "iphone16", runtime), log_show(HOST_LOG_PREDICATE)]
        * attempts,
    ]
    assert "creation refused" in log.read_text()


def test_runtime_selection_uses_stdout_without_discarding_stderr(app, tmp_path, simulator):
    _, responses = simulator
    responses[("list", "devices", "available", "--json")] = (
        json.dumps(DEVICES),
        "runtime warning",
    )
    log = tmp_path / "log"
    boot_test(app, log)
    assert "runtime warning" in log.read_text()
    assert SUCCESS in log.read_text()


def test_prior_attempt_success_marker_does_not_hide_app_failure(app, tmp_path, simulator):
    process, responses = simulator
    responses[("bootstatus", "uuid-1", "-b")] = subprocess.TimeoutExpired(
        "bootstatus", 600, output=SUCCESS
    )
    responses[launch_command("uuid-2")] = ("launch successful", "")
    log = tmp_path / "log"
    with pytest.raises(RuntimeError, match="successful"):
        boot_test(app, log)
    assert SUCCESS in log.read_text()
    assert "launch successful" in log.read_text()
    assert [command for command in commands(process) if command[0] == "launch"] == [
        launch_command("uuid-2")
    ]
    assert commands(process)[-1] == ("delete", "uuid-2")


@pytest.mark.parametrize(
    "options",
    [
        {"timeout": 0},
        {"timeout": -1},
        {"boot_timeout": 0},
        {"boot_timeout": -1},
        {"boot_timeout": MAX_BOOT_TIMEOUT + 1},
        {"boot_attempts": 0},
        {"boot_attempts": -1},
        {"boot_attempts": MAX_BOOT_ATTEMPTS + 1},
        {"total_timeout": 0},
    ],
)
def test_invalid_options_do_not_create_simulator(tmp_path, options):
    with patch("ci.ios.ios_boot_test.subprocess.run") as process, pytest.raises(ValueError):
        boot_test(tmp_path / "app", tmp_path / "log", **options)
    process.assert_not_called()
    assert not (tmp_path / "log").exists()


@pytest.mark.parametrize("boot_timeout", [1, DEFAULT_BOOT_TIMEOUT, MAX_BOOT_TIMEOUT])
def test_boot_timeout_is_independent_of_app_timeout(app, tmp_path, simulator, boot_timeout):
    process, _ = simulator
    boot_test(app, tmp_path / "log", timeout=420, boot_timeout=boot_timeout, boot_attempts=1)
    for call in process.call_args_list:
        if call.args[0][2] == "bootstatus":
            assert call.kwargs["timeout"] == boot_timeout
        elif call.args[0][2] in ("install", "launch"):
            assert call.kwargs["timeout"] == 420


@pytest.mark.parametrize(
    "option,value",
    [
        ("--timeout", "0"),
        ("--boot-timeout", "-1"),
        ("--boot-timeout", str(MAX_BOOT_TIMEOUT + 1)),
        ("--boot-attempts", "0"),
        ("--boot-attempts", str(MAX_BOOT_ATTEMPTS + 1)),
        ("--boot-attempts", "1.5"),
        ("--total-timeout", "0"),
    ],
)
def test_invalid_cli_options_fail_before_subprocesses(option, value, capsys):
    with (
        patch("ci.ios.ios_boot_test.subprocess.run") as process,
        pytest.raises(SystemExit) as raised,
    ):
        main(["--app", "app", "--log", "log", option, value])
    assert raised.value.code == 2
    assert "error:" in capsys.readouterr().err
    process.assert_not_called()


def test_cli_wires_boot_options():
    argv = ["--app", "app", "--log", "log", "--timeout", "420"]
    with patch("ci.ios.ios_boot_test.boot_test") as run:
        options = ["--boot-timeout", "900", "--boot-attempts", "1", "--total-timeout", "1200"]
        assert main([*argv, *options]) == 0
    assert run.call_args.kwargs == {
        "timeout": 420,
        "boot_timeout": 900,
        "boot_attempts": 1,
        "total_timeout": 1200,
        "progress": ANY,
    }


def test_cli_defaults():
    with patch("ci.ios.ios_boot_test.boot_test") as run:
        assert main(["--app", "app", "--log", "log"]) == 0
    assert run.call_args.kwargs == {
        "timeout": DEFAULT_APP_TIMEOUT,
        "boot_timeout": DEFAULT_BOOT_TIMEOUT,
        "boot_attempts": MAX_BOOT_ATTEMPTS,
        "total_timeout": DEFAULT_TOTAL_TIMEOUT,
        "progress": ANY,
    }


FAILED_COMMANDS = {
    "bootstatus": ("bootstatus", "uuid-1", "-b"),
    "install": lambda app: ("install", "uuid-1", str(app)),
    "launch": launch_command("uuid-1"),
}


def _failed_command(phase, app):
    command = FAILED_COMMANDS[phase]
    return command(app) if callable(command) else command


@pytest.mark.parametrize("phase", ["bootstatus", "install", "launch"])
def test_every_failed_phase_saves_simulator_and_host_logs(app, tmp_path, simulator, phase):
    process, responses = simulator
    failure = subprocess.CalledProcessError(1, phase, stderr="refused")
    responses[_failed_command(phase, app)] = failure
    responses[("spawn", "uuid-1", *log_show(SIMULATOR_LOG_PREDICATE))] = ("QGC last words\n", "")
    responses[log_show(HOST_LOG_PREDICATE)] = ("CoreSimulatorService stalled\n", "warning\n")
    log = tmp_path / "ios-boot-test" / "ios-boot-test.log"

    with pytest.raises(subprocess.CalledProcessError) as raised:
        boot_test(app, log, boot_attempts=1)

    assert raised.value is failure
    saved = log.parent / "ios-boot-test-attempt1-simulator.log"
    assert saved.read_text() == "QGC last words\n"
    host = log.parent / "ios-boot-test-attempt1-coresimulator.log"
    assert host.read_text() == "CoreSimulatorService stalled\nwarning\n"
    text = log.read_text()
    assert "Saved ios-boot-test-attempt1-simulator.log (exit code 0, 1 lines)" in text
    assert "QGC last words" not in text
    for call in process.call_args_list:
        if _args(call.args[0])[0] in ("spawn", "log"):
            assert call.kwargs["timeout"] == LOG_SHOW_TIMEOUT


def test_failed_attempt_keeps_only_this_runs_app_crash_reports(
    app, tmp_path, simulator, crash_reports
):
    _, responses = simulator
    responses[launch_command("uuid-1")] = ("Simple boot test failed", "")
    (crash_reports / "QGC-2026-10-23-120000.ips").write_text("crashed")
    (crash_reports / "Other-2026-10-23-120000.ips").write_text("another app")
    old = crash_reports / "QGC-2026-01-01-120000.crash"
    old.write_text("an earlier crash")
    os.utime(old, (time.time() - 3600, time.time() - 3600))
    log = tmp_path / "log"

    with pytest.raises(RuntimeError):
        boot_test(app, log)

    assert sorted(path.name for path in tmp_path.glob("log-*.ips")) == [
        "log-QGC-2026-10-23-120000.ips"
    ]
    assert not list(tmp_path.glob("log-*.crash"))
    assert "Saved crash report QGC-2026-10-23-120000.ips" in log.read_text()


@pytest.mark.parametrize(
    ("failure", "expected"),
    [
        (OSError("log unavailable"), "log failed: log unavailable"),
        (
            subprocess.TimeoutExpired("log", LOG_SHOW_TIMEOUT, output=b"partial log\n"),
            "timed out after 60s",
        ),
    ],
)
def test_diagnostic_log_failures_keep_the_original_failure(
    app, tmp_path, simulator, failure, expected
):
    _, responses = simulator
    original = subprocess.CalledProcessError(1, "install", stderr="install refused")
    responses[("install", "uuid-1", str(app))] = original
    responses[log_show(HOST_LOG_PREDICATE)] = failure
    log = tmp_path / "log"

    with pytest.raises(subprocess.CalledProcessError) as raised:
        boot_test(app, log, boot_attempts=1)

    assert raised.value is original
    assert expected in log.read_text()
    host = tmp_path / "log-attempt1-coresimulator.log"
    assert host.exists() == isinstance(failure, subprocess.TimeoutExpired)
    if host.exists():
        assert host.read_text() == "partial log\n"


def test_spent_budget_caps_timeouts_and_stops_retries(app, tmp_path, simulator, monkeypatch):
    process, responses = simulator
    responses[("bootstatus", "uuid-1", "-b")] = subprocess.TimeoutExpired("bootstatus", 1)
    readings = iter([0.0])
    clock = SimpleNamespace(monotonic=lambda: next(readings, 1600.0), time=time.time)
    monkeypatch.setattr(ios_boot_test, "time", clock)
    log = tmp_path / "log"

    with pytest.raises(subprocess.TimeoutExpired):
        boot_test(app, log)

    calls = process.call_args_list
    assert sum(_args(call.args[0])[0] == "create" for call in calls) == 1
    for call in calls:
        if _args(call.args[0])[0] in ("create", "boot", "bootstatus"):
            assert call.kwargs["timeout"] == 1
    assert f"Not retrying because the {DEFAULT_TOTAL_TIMEOUT}s budget is spent" in log.read_text()


def test_budget_caps_app_timeouts(app, tmp_path, simulator):
    process, _ = simulator
    boot_test(app, tmp_path / "log", total_timeout=100)
    timeouts = [
        call.kwargs["timeout"]
        for call in process.call_args_list
        if call.args[0][2] in ("install", "launch")
    ]
    assert len(timeouts) == 2
    assert all(90 <= timeout <= 100 for timeout in timeouts)


def test_main_annotates_the_failing_phase_and_attempt(app, tmp_path, simulator, gh_summary, capsys):
    _, responses = simulator
    for device in ("uuid-1", "uuid-2"):
        responses[("install", device, str(app))] = subprocess.TimeoutExpired(
            ["xcrun", "simctl", "install"], 600, output="".join(f"copying {n}\n" for n in range(30))
        )
    log = tmp_path / "ios-boot-test" / "ios-boot-test.log"

    assert main(["--app", str(app), "--log", str(log)]) == 1

    captured = capsys.readouterr()
    errors = [line for line in captured.out.splitlines() if line.startswith("::error")]
    assert errors == [
        "::error::iOS simulator boot test failed during install (attempt 2/2): TimeoutExpired: "
        "Command '['xcrun', 'simctl', 'install']' timed out after 600 seconds"
    ]
    assert "Traceback" in captured.err
    summary = gh_summary.read_text()
    assert summary.startswith(
        "### Boot test\n\n| Item | Value |\n| --- | --- |\n"
        "| Result | failed during install (attempt 2/2) |\n| App | `QGC.app` |\n"
        "| Simulator | iphone16 (iOS-18-5) |\n| Attempts | 2/2 |\n| Duration | "
    )
    details = summary[summary.index("<details><summary>Last 40 lines of the boot log</summary>") :]
    lines = log.read_text().splitlines()
    assert lines[-1] in details
    assert f"\n{lines[-41]}\n" not in details
    assert summary.endswith("</details>\n\n")


def test_main_summarizes_a_passing_boot(app, tmp_path, simulator, gh_summary, capsys):
    assert main(["--app", str(app), "--log", str(tmp_path / "log")]) == 0
    summary = gh_summary.read_text()
    assert "| Result | passed |\n" in summary
    assert "| Attempts | 1/2 |\n" in summary
    assert "<details>" not in summary
    assert "::error" not in capsys.readouterr().out


def test_main_reports_setup_failures(tmp_path, gh_summary, capsys):
    with patch("ci.ios.ios_boot_test.subprocess.run") as process:
        assert main(["--app", str(tmp_path / "missing.app"), "--log", str(tmp_path / "log")]) == 1
    process.assert_not_called()
    assert "::error::iOS simulator boot test failed during setup: FileNotFoundError: " in (
        capsys.readouterr().out
    )
    summary = gh_summary.read_text()
    assert "| Result | failed during setup |\n" in summary
    assert "Attempts" not in summary
    assert "<details>" not in summary


def test_main_annotation_escapes_line_breaks(capsys):
    with patch("ci.ios.ios_boot_test.boot_test", side_effect=RuntimeError("stalled\n50% done")):
        assert main(["--app", "app", "--log", "log"]) == 1
    errors = [line for line in capsys.readouterr().out.splitlines() if line.startswith("::")]
    assert errors == [
        "::error::iOS simulator boot test failed during setup: RuntimeError: stalled%0A50%25 done"
    ]


def test_progress_tracks_the_phase_and_attempt(app, tmp_path, simulator):
    _, responses = simulator
    responses[launch_command("uuid-1")] = ("Simple boot test failed", "")
    progress = Progress()
    with pytest.raises(RuntimeError):
        boot_test(app, tmp_path / "log", progress=progress)
    assert progress == Progress("launch", 1, 2, "iphone16 (iOS-18-5)")
