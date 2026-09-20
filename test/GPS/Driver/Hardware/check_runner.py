"""Exercise the real runner entry point without opening physical transports."""

from __future__ import annotations

import json
import signal
import subprocess
import sys
import time
from pathlib import Path
from tempfile import TemporaryDirectory


def run(binary: str, arguments: list[str], expected: int) -> dict:
    result = subprocess.run(
        [binary, *arguments],
        capture_output=True,
        text=True,
        timeout=30,
        check=False,
    )
    assert result.returncode == expected, (
        arguments,
        result.returncode,
        result.stdout,
        result.stderr,
    )
    report = json.loads(result.stdout)
    assert report["schema_version"] == 1
    assert report.get("physical_hardware_verified") is not True
    assert "legacy_baseline" not in report and "px4_revision" not in report
    if "backend" in report:
        assert report["backend"] == "native"
        assert report["receive_outcome_semantics"] == "typed_native"
    return report


def main() -> None:
    binary = sys.argv[1]
    serial_disabled = len(sys.argv) > 2 and sys.argv[2] == "1"
    with TemporaryDirectory(prefix="gps-runner-", dir=Path.cwd()) as directory:
        path = Path(directory) / "plan.json"
        report = run(binary, ["--output", str(path)], 0)
        assert json.loads(path.read_text()) == report
        original = path.read_bytes()
        error = run(binary, ["--action", "suite", "--output", str(path)], 4)
        assert error["outcome"] == "evidence_error" and "stages" not in error
        assert path.read_bytes() == original
        missing = Path(directory) / "missing" / "report.json"
        error = run(binary, ["--action", "suite", "--output", str(missing)], 4)
        assert error["outcome"] == "evidence_error" and "stages" not in error

        path = Path(directory) / "progress.json"
        with subprocess.Popen(
            [
                binary,
                "--action",
                "configure",
                "--output",
                str(path),
                "--observe-ms",
                "2500",
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        ) as process:
            deadline = time.monotonic() + 5
            progress = {}
            surveys = []
            while time.monotonic() < deadline and process.poll() is None:
                text = path.read_text() if path.exists() else ""
                if text:
                    progress = json.loads(text)
                    surveys = progress.get("active_stage", {}).get("survey_observations", [])
                    if any(
                        item["origin_assessment"] == "consistent_with_fresh" for item in surveys
                    ):
                        break
                time.sleep(0.01)
            assert progress.get("outcome") == "running", progress
            assert progress.get("active_stage", {}).get("phase") == "observing", progress
            assert surveys, progress
            expected = 3
            if sys.platform != "win32":
                process.send_signal(signal.SIGINT)
                expected = 1
            stdout, stderr = process.communicate(timeout=10)
            assert process.returncode == expected, (stdout, stderr)
            report = json.loads(stdout)
            assert json.loads(path.read_text()) == report
            assert "active_stage" not in report
            if sys.platform != "win32":
                assert report["interrupted"]

    for transport, endpoint in (
        ("serial", ["--device", "not-a-real-device"]),
        ("tcp", ["--host", "127.0.0.1", "--port", "1"]),
        ("udp", ["--host", "127.0.0.1", "--port", "1"]),
    ):
        if serial_disabled and transport == "serial":
            report = run(binary, ["--transport", transport, *endpoint], 2)
            assert "disabled" in report["detail"]
            continue
        report = run(binary, ["--transport", transport, *endpoint], 0)
        assert report["outcome"] == "not_run" and "stages" not in report
        report = run(
            binary,
            ["--action", "suite", "--transport", transport, *endpoint],
            2,
        )
        assert report["outcome"] == "rejected" and "stages" not in report
    for family in ("trimble", "septentrio", "femto"):
        report = run(binary, ["--action", "role-cycle", "--family", family], 2)
        assert "does not support Position" in report["detail"]
    endpoint = ["--transport", "tcp", "--host", "127.0.0.1", "--port", "1"]
    for family, mode in (("unicore", "receiver-averaging"), ("quectel", "survey")):
        report = run(binary, [*endpoint, "--family", family, "--base-mode", mode], 0)
        assert report["outcome"] == "not_run" and "stages" not in report
        assert report["requested"]["base_mode"] == mode
        assert report["requested"]["allow_persistent_changes"] is False
        report = run(
            binary,
            [
                *endpoint,
                "--family",
                family,
                "--base-mode",
                "fixed",
                "--latitude",
                "0",
                "--longitude",
                "0",
                "--altitude",
                "0",
            ],
            0,
        )
        assert report["requested"]["ellipsoid_altitude_m"] == 0
    report = run(
        binary, [*endpoint, "--family", "passive", "--role", "passive", "--baud", "115200"], 0
    )
    assert report["requested"]["role"] == "passive"
    assert "base_mode" not in report["requested"]
    run(binary, [*endpoint, "--family", "unicore"], 2)
    run(binary, [*endpoint, "--family", "quectel", "--base-mode", "receiver-averaging"], 2)
    run(binary, [*endpoint, "--family", "passive", "--role", "passive"], 2)
    run(binary, [*endpoint, "--family", "unicore", "--base-mode", "fixed"], 2)
    report = run(binary, [*endpoint, "--family", "quectel", "--allow-save"], 0)
    assert report["requested"]["allow_persistent_changes"] is True
    assert report["deadlines"]["open_and_configure_ms"] == 60000
    report = run(binary, [*endpoint, "--family", "quectel", "--timeout-ms", "1000"], 0)
    assert report["deadlines"]["open_and_configure_ms"] == 1000
    run(
        binary,
        [*endpoint, "--family", "passive", "--role", "passive", "--baud", "115200", "--allow-save"],
        2,
    )
    run(
        binary,
        [
            *endpoint,
            "--family",
            "unicore",
            "--base-mode",
            "receiver-averaging",
            "--survey-accuracy",
            "2",
        ],
        2,
    )
    for action in ("configure", "role-cycle", "suite"):
        rejected = run(binary, ["--action", action, "--dynamic-model", "2"], 2)
        assert rejected["outcome"] == "rejected" and "stages" not in rejected
    run(binary, ["--role", "position", "--dynamic-model", "1"], 2)
    run(binary, ["--survey-accuracy", "nan"], 2)
    run(binary, ["--observe-ms", "-1"], 2)
    run(binary, ["--backend", "legacy", "--action", "suite"], 2)
    run(binary, ["--backend", "native", "--action", "suite"], 2)

    for model in ("f9p", "m8p"):
        report = run(
            binary,
            [
                "--action",
                "suite",
                "--model",
                model,
                "--observe-ms",
                "20",
                "--constellations",
                "1",
            ],
            3,
        )
        assert report["evidence_origin"] == "scripted"
        assert report["outcome"] == "inconclusive"
        assert report["scripted_wire_valid"]
        stages = report["stages"]
        assert [stage["name"] for stage in stages] == [
            "base_initial",
            "position",
            "base_return",
            "reconnected_base",
        ]
        for stage in stages:
            checks = {item["name"]: item for item in stage["checks"]}
            assert checks["configure_return"]["status"] == "passed", stage
            assert stage["wire_evidence"]["transport_written_bytes"] > 0
            assert stage["wire_evidence"]["ubx_ack_frames"] > 0
            assert checks["requested_settings_readback"]["status"] == "inconclusive"
            settings = {item["setting"]: item for item in stage["requested_setting_observations"]}
            assert settings["time_mode"]["matching_write_observed"], stage
            assert not settings["time_mode"]["transaction_correlation_verified"]
            if stage["name"] == "position":
                assert settings["time_mode"]["readback"] == "matching_value_observed", stage
            else:
                assert settings["survey_duration_s"]["matching_write_observed"], stage
                assert settings["survey_accuracy_0.1mm"]["matching_write_observed"], stage
                assert stage["survey_observations"], stage
                assert any(
                    item["origin_assessment"] == "consistent_with_fresh"
                    for item in stage["survey_observations"]
                ), stage
                assert not any(item["fresh_survey_proven"] for item in stage["survey_observations"])
        checks = {item["name"]: item for item in stages[-1]["checks"]}
        assert checks["reconnect"]["status"] == "passed"
        assert checks["receive_cancellation"]["status"] == "passed"
        commands = stages[1]["configuration_evidence"]["commands"]
        assert any(item["outcome"] == "readback_verified" for item in commands)
        position = run(
            binary,
            [
                "--action",
                "configure",
                "--role",
                "position",
                "--model",
                model,
                "--dynamic-model",
                "2",
                "--observe-ms",
                "20",
            ],
            3,
        )
        settings = {
            item["setting"]: item
            for item in position["stages"][0]["requested_setting_observations"]
        }
        assert settings["dynamic_model"]["matching_write_observed"], position

    retained = run(
        binary,
        ["--action", "configure", "--survey-state", "retained", "--observe-ms", "20"],
        3,
    )
    assert any(
        item["origin_assessment"] == "retained_or_preexisting"
        for item in retained["stages"][0]["survey_observations"]
    ), retained
    absent = run(
        binary,
        ["--action", "configure", "--survey-state", "none", "--observe-ms", "20"],
        3,
    )
    checks = {item["name"]: item for item in absent["stages"][0]["checks"]}
    assert checks["fresh_survey"]["status"] == "inconclusive"

    for fault in ("nak", "wrong-readback", "cancel"):
        failed = run(
            binary,
            ["--action", "configure", "--role", "position", "--fault", fault, "--observe-ms", "20"],
            1,
        )
        assert failed["outcome"] == "failed"

    for fault in ("rtcm-nak", "rtcm-nak-cancel"):
        failed = run(
            binary,
            [
                "--action",
                "cancel",
                "--fault",
                fault,
                "--observe-ms",
                "20",
                "--cancel-after-ms",
                "1000",
            ],
            1,
        )
        stage = failed["stages"][0]
        checks = {item["name"]: item for item in stage["checks"]}
        assert checks["configure_return"]["status"] == "passed", stage
        assert failed["outcome"] == "failed", failed
        assert checks["receive_outcome"]["detail"] == "terminal_protocol_error", stage
        assert stage["transport_healthy_at_receive_failure"], stage
        assert checks["receive_cancellation"]["status"] == (
            "not_run" if fault == "rtcm-nak" else "failed"
        ), stage
    print("Native GPS runner safety, survey provenance and failure contracts passed")


if __name__ == "__main__":
    main()
