"""Exercise simulator lifecycle and QGC success-marker validation without Xcode."""

import json
import plistlib
import subprocess
from unittest.mock import patch

import pytest
from ios_boot_test import (
    CLEANUP_TIMEOUT,
    DEFAULT_BOOT_TIMEOUT,
    DIAGNOSTIC_TIMEOUT,
    MAX_BOOT_ATTEMPTS,
    MAX_BOOT_TIMEOUT,
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


@pytest.fixture
def simulator():
    responses = {}
    devices = iter(("uuid-1", "uuid-2"))

    def run(command, **kwargs):
        args = tuple(command[2:])
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

    with patch("ios_boot_test.subprocess.run", side_effect=run) as process:
        yield process, responses


def commands(process):
    return [tuple(call.args[0][2:]) for call in process.call_args_list]


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
    assert calls[-3:] == [launch_command("uuid-1"), ("shutdown", "uuid-1"), ("delete", "uuid-1")]
    assert output in log.read_text()
    assert "create warning" in log.read_text()
    for call in process.call_args_list:
        if call.args[0][2] in ("install", "launch"):
            assert call.kwargs["timeout"] == 300
        assert call.kwargs["check"] is True


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
    assert calls[failure_index + 1 : failure_index + 4] == [
        ("list", "devices", "--json"),
        ("shutdown", "uuid-1"),
        ("delete", "uuid-1"),
    ]
    assert calls[failure_index + 4][0] == "create"
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
        "Retrying simulator cold boot",
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
@pytest.mark.parametrize("phase", ["install", "launch"])
@pytest.mark.parametrize("error_type", [subprocess.TimeoutExpired, subprocess.CalledProcessError])
def test_app_command_failure_is_not_retried(app, tmp_path, simulator, timeout, phase, error_type):
    process, responses = simulator
    failure = (
        subprocess.TimeoutExpired(phase, timeout, output=b"hung", stderr="application error")
        if error_type is subprocess.TimeoutExpired
        else subprocess.CalledProcessError(1, phase, output=SUCCESS, stderr=b"application error")
    )
    command = ("install", "uuid-1", str(app)) if phase == "install" else launch_command("uuid-1")
    responses[command] = failure
    log = tmp_path / "log"
    with pytest.raises(error_type) as raised:
        boot_test(app, log, timeout=timeout)
    assert raised.value is failure
    calls = commands(process)
    assert sum(command[0] == "create" for command in calls) == 1
    assert calls[-2:] == [("shutdown", "uuid-1"), ("delete", "uuid-1")]
    phase_call = next(call for call in process.call_args_list if call.args[0][2] == phase)
    assert phase_call.kwargs["timeout"] == timeout
    assert "application error" in log.read_text()


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


@pytest.mark.parametrize("phase", ["bootstatus", "launch"])
def test_cleanup_failures_preserve_original_failure(app, tmp_path, simulator, phase):
    process, responses = simulator
    original = subprocess.TimeoutExpired(phase, 600, output=b"original failure")
    command = ("bootstatus", "uuid-1", "-b") if phase == "bootstatus" else launch_command("uuid-1")
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


def test_delete_failure_after_app_success_is_fatal(app, tmp_path, simulator):
    process, responses = simulator
    failure = subprocess.TimeoutExpired("delete", 30, output=b"delete hung")
    responses[("delete", "uuid-1")] = failure
    log = tmp_path / "log"
    with pytest.raises(subprocess.TimeoutExpired) as raised:
        boot_test(app, log)
    assert raised.value is failure
    assert sum(command[0] == "create" for command in commands(process)) == 1
    assert SUCCESS in log.read_text()
    assert "delete hung" in log.read_text()


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


def test_create_failure_is_not_retried_or_cleaned_up_as_a_device(app, tmp_path, simulator):
    process, responses = simulator
    runtime = next(iter(DEVICES["devices"]))
    failure = subprocess.CalledProcessError(1, "create", stderr="creation refused")
    responses[("create", "QGC CI boot test", "iphone16", runtime)] = failure
    log = tmp_path / "log"
    with pytest.raises(subprocess.CalledProcessError) as raised:
        boot_test(app, log)
    assert raised.value is failure
    assert [command[0] for command in commands(process)] == ["list", "create"]
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
    ],
)
def test_invalid_options_do_not_create_simulator(tmp_path, options):
    with patch("ios_boot_test.subprocess.run") as process, pytest.raises(ValueError):
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
    ],
)
def test_invalid_cli_options_fail_before_subprocesses(option, value, capsys):
    with (
        patch("sys.argv", ["ios_boot_test.py", "--app", "app", "--log", "log", option, value]),
        patch("ios_boot_test.subprocess.run") as process,
        pytest.raises(SystemExit) as raised,
    ):
        main()
    assert raised.value.code == 2
    assert "error:" in capsys.readouterr().err
    process.assert_not_called()


def test_cli_wires_boot_options():
    with (
        patch(
            "sys.argv",
            [
                "ios_boot_test.py",
                "--app",
                "app",
                "--log",
                "log",
                "--timeout",
                "420",
                "--boot-timeout",
                "900",
                "--boot-attempts",
                "1",
            ],
        ),
        patch("ios_boot_test.boot_test") as run,
    ):
        main()
    assert run.call_args.kwargs == {"timeout": 420, "boot_timeout": 900, "boot_attempts": 1}
