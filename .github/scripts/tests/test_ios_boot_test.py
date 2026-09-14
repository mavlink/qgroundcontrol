"""Exercise simulator lifecycle and QGC success-marker validation without Xcode."""

import plistlib
import subprocess
from unittest.mock import patch

import pytest
from ios_boot_test import boot_test, select_device

DEVICES = {
    "devices": {
        "com.apple.CoreSimulator.SimRuntime.iOS-18-5": [
            {"name": "iPhone 16", "isAvailable": True, "deviceTypeIdentifier": "iphone16"}
        ]
    }
}


@pytest.mark.parametrize(
    "output,error",
    [
        ("Simple boot test completed", False),
        ("launch successful", True),
        ("Simple boot test failed", True),
    ],
)
def test_app_must_report_boot_success_and_device_is_deleted(tmp_path, output, error):
    import json

    app = tmp_path / "QGC.app"
    app.mkdir()
    (app / "Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "org.qgc"}))
    with patch(
        "ios_boot_test.simctl",
        side_effect=[json.dumps(DEVICES), "uuid", "", "", "", output, "", ""],
    ) as simctl:
        if error:
            with pytest.raises(RuntimeError, match="successful"):
                boot_test(app, tmp_path / "log")
        else:
            boot_test(app, tmp_path / "log")
    assert simctl.call_args_list[4].kwargs["timeout"] == 300
    assert simctl.call_args_list[5].kwargs["timeout"] == 300
    assert simctl.call_args_list[5].args[-4:] == (
        "--simple-boot-test",
        "--logging",
        "Main",
        "--log-output",
    )
    assert simctl.call_args_list[-1].args == ("delete", "uuid")
    assert (tmp_path / "log").read_text() == output


def test_missing_runtime_fails():
    with pytest.raises(RuntimeError, match="No available"):
        select_device({"devices": {}})


@pytest.mark.parametrize("timeout", [300, 420])
@pytest.mark.parametrize("phase", ["install", "launch"])
def test_install_or_launch_timeout_still_cleans_up(tmp_path, timeout, phase):
    import json

    app = tmp_path / "QGC.app"
    app.mkdir()
    (app / "Info.plist").write_bytes(plistlib.dumps({"CFBundleIdentifier": "org.qgc"}))
    responses: list[str | subprocess.TimeoutExpired] = [json.dumps(DEVICES), "uuid", "", ""]
    if phase == "launch":
        responses.append("")
    responses.extend([subprocess.TimeoutExpired(phase, timeout, output=b"hung"), "", ""])
    with (
        patch(
            "ios_boot_test.simctl",
            side_effect=responses,
        ) as simctl,
        pytest.raises(subprocess.TimeoutExpired),
    ):
        boot_test(app, tmp_path / "log", timeout=timeout)
    phase_call = next(call for call in simctl.call_args_list if call.args[0] == phase)
    assert phase_call.kwargs["timeout"] == timeout
    assert simctl.call_args_list[-1].args == ("delete", "uuid")
    assert (tmp_path / "log").read_text() == "hung"


def test_invalid_boot_deadline_does_not_create_simulator(tmp_path):
    with patch("ios_boot_test.simctl") as simctl, pytest.raises(ValueError, match="positive"):
        boot_test(tmp_path / "app", tmp_path / "log", timeout=0)
    simctl.assert_not_called()
