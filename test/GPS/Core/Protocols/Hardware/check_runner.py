"""Exercise the real runner entry point without opening physical transports."""

from __future__ import annotations

import json
import os
import re
import signal
import subprocess
import sys
import time
from pathlib import Path
from tempfile import TemporaryDirectory
from typing import TYPE_CHECKING, cast

if TYPE_CHECKING:
    from collections.abc import Iterator

# Waits scale on CI as the C++ suites' TestTimeout does.
CI_SCALE = 2 if "CI" in os.environ or "GITHUB_ACTIONS" in os.environ else 1
# Scripted cancellation only has to return with a typed outcome, not within a hardware latency budget.
CANCEL_SLACK = ["--cancel-slack-ms", str(2000 * CI_SCALE)]


def run(binary: str, arguments: list[str], expected: int) -> dict:
    result = subprocess.run(
        [binary, *arguments],
        capture_output=True,
        text=True,
        timeout=30 * CI_SCALE,
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
    return report


def numbers(value: object) -> Iterator[float]:
    if isinstance(value, dict):
        for item in cast("dict[str, object]", value).values():
            yield from numbers(item)
    elif isinstance(value, list):
        for item in cast("list[object]", value):
            yield from numbers(item)
    elif isinstance(value, (int, float)) and not isinstance(value, bool):
        yield float(value)


def check_record_and_replay(binary: str) -> None:
    corpus = Path(__file__).resolve().parents[2] / "Protocols" / "corpus"
    fixtures = corpus.parent / "fixtures"
    replay = ["--action", "configure"]
    passive = [*replay, "--family", "passive", "--role", "passive", "--baud", "115200"]
    with TemporaryDirectory(prefix="gps-record-", dir=Path.cwd()) as directory:
        report = run(
            binary,
            [
                "--action",
                "configure",
                "--observe-ms",
                "20",
                "--record",
                str(Path(directory) / "GPS"),
            ],
            3,
        )
        assert len(report["recordings"]) == 1, report
        received = Path(report["recordings"][0]["received"])
        sent = Path(report["recordings"][0]["sent"])
        assert re.fullmatch(r"gps-ublox-\d{8}-\d{6}-rx\.ubx", received.name), received
        assert sent.name == received.name.replace("-rx.ubx", "-tx.bin"), sent
        assert b"\xb5\x62" in received.read_bytes() and sent.read_bytes().startswith(b"\xb5\x62")
        # The passive decoder reads the recorded UBX fix and survey status too.
        report = run(binary, ["--replay", str(received), *passive], 0)
        assert report["outcome"] == "replayed" and report["replay"]["complete"], report
        assert report["replay"]["bytes"] == received.stat().st_size
        assert report["positions"]["fix_types"]["3d"] > 0, report
        assert report["survey"]["messages"] > 0, report
        report = run(binary, ["--replay", str(received), *replay, "--family", "ublox"], 0)
        assert report["outcome"] == "replayed" and report["replay"]["decode_only"], report
        assert report["positions"]["fix_types"]["3d"] > 0, report
        assert report["survey"]["messages"] > 0 and "active" in report["survey"], report
        assert "requested" not in report, report
        # The scripted fix is at 47.3, 8.5.
        assert not {47.3, 8.5} & set(numbers(report)), report

    with TemporaryDirectory(prefix="gps-slice-", dir=Path.cwd()) as directory:
        # The NAV-PVT frame at the start of the recording, as in the C++ NAV_PVT slice.
        ublox = Path(directory) / "nav-pvt.ubx"
        ublox.write_bytes((fixtures / "navigation.ubx").read_bytes()[:100])
        report = run(binary, ["--replay", str(ublox), *replay, "--family", "ublox"], 0)
    assert report["outcome"] == "replayed" and report["replay"]["bytes"] == 100
    positions = report["positions"]
    assert positions["messages"] == 1 and len(positions["fix_types"]) == 1, report
    assert "latitude" not in json.dumps(report) and "longitude" not in json.dumps(report), report

    # Base status decodes as configuring the requested base leaves the decoder, as the decode goldens show.
    quectel = corpus / "synthetic-quectel-base.nmea"
    report = run(binary, ["--replay", str(quectel), *replay, "--family", "quectel"], 0)
    assert report["survey"]["messages"] == 4 and not report["survey"]["valid"], report
    unicore = corpus / "synthetic-unicore-base.ascii"
    fixed = ["--base-mode", "fixed", "--latitude", "0", "--longitude", "90", "--altitude", "100"]
    report = run(binary, ["--replay", str(unicore), *replay, "--family", "unicore", *fixed], 0)
    assert report["survey"]["messages"] == 3 and not report["survey"]["valid"], report
    assert "latitude" not in json.dumps(report) and "longitude" not in json.dumps(report), report
    # Unicore cannot run the requested survey-in; Femto times its survey from the configuration.
    report = run(binary, ["--replay", str(unicore), *replay, "--family", "unicore"], 0)
    assert report["survey"] == "unsupported" and report["outcome"] == "replayed", report
    femto = corpus / "femto-600.bin"
    report = run(binary, ["--replay", str(femto), *replay, "--family", "femto"], 0)
    assert report["survey"] == "unsupported", report

    capture = fixtures / "synthetic-gga.nmea"
    report = run(binary, ["--replay", str(capture), *passive], 0)
    assert report["evidence_origin"] == "recording" and report["outcome"] == "replayed", report
    assert (
        report["positions"]["messages"] > 0 and report["positions"]["fix_types"]["rtk-fixed"] == 1
    )
    # Summaries only: no coordinate field, and none of the synthetic fixes' coordinates.
    assert "latitude" not in json.dumps(report) and "longitude" not in json.dumps(report), report
    assert not {12.5, -12.5, 45.25, -45.25} & set(numbers(report)), report
    paced = run(binary, ["--replay", str(capture), "--replay-baud", "38400", *passive], 0)
    assert paced["replay"]["elapsed_ms"] >= capture.stat().st_size * 10 * 1000 // 38400 - 1, paced
    report = run(binary, ["--replay", str(corpus / "missing.nmea"), *passive], 1)
    assert report["outcome"] == "failed" and "positions" not in report

    run(binary, ["--replay", str(capture), "--transport", "tcp", *passive], 2)
    run(binary, ["--replay", str(capture), "--record", "recordings", *passive], 2)
    run(binary, ["--replay", str(capture), "--fault", "nak", *replay], 2)
    run(binary, ["--replay", str(capture), "--action", "suite", *passive[2:]], 2)
    run(binary, ["--replay-baud", "9600"], 2)


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
        # SIGINT ends the observation, so it can outlast any wait for progress; Windows observes to the end.
        observe = "2500" if sys.platform == "win32" else "600000"
        with subprocess.Popen(
            [
                binary,
                "--action",
                "configure",
                "--output",
                str(path),
                "--observe-ms",
                observe,
            ],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        ) as process:
            deadline = time.monotonic() + 5 * CI_SCALE
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
            stdout, stderr = process.communicate(timeout=10 * CI_SCALE)
            assert process.returncode == expected, (stdout, stderr)
            report = json.loads(stdout)
            assert json.loads(path.read_text()) == report
            assert "active_stage" not in report
            if sys.platform != "win32":
                assert report["interrupted"]

    tcp_endpoint = ["--transport", "tcp", "--device", "127.0.0.1:1"]
    report = run(binary, tcp_endpoint, 0)
    assert report["outcome"] == "not_run" and "stages" not in report
    for device in ("127.0.0.1", "127.0.0.1:0", "127.0.0.1:65536"):
        report = run(binary, ["--transport", "tcp", "--device", device], 2)
        assert "host:port" in report["detail"]
    report = run(binary, ["--action", "configure", *tcp_endpoint], 2)
    assert "allow-reconfigure" in report["detail"]
    serial_endpoint = ["--transport", "serial", "--device", "not-a-real-device"]
    if serial_disabled:
        report = run(binary, serial_endpoint, 2)
        assert "disabled" in report["detail"]
    else:
        report = run(binary, serial_endpoint, 0)
        assert report["outcome"] == "not_run" and "stages" not in report
        report = run(
            binary,
            ["--action", "suite", *serial_endpoint],
            2,
        )
        assert report["outcome"] == "rejected" and "stages" not in report
    if not serial_disabled:
        endpoint = serial_endpoint
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
            [
                *endpoint,
                "--family",
                "passive",
                "--role",
                "passive",
                "--baud",
                "115200",
                "--allow-save",
            ],
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
    run(binary, ["--survey-accuracy", "nan"], 2)
    run(binary, ["--observe-ms", "-1"], 2)

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
                *CANCEL_SLACK,
            ],
            3,
        )
        assert report["evidence_origin"] == "scripted"
        assert report["outcome"] == "inconclusive"
        assert report["scripted_wire_valid"]
        stages = report["stages"]
        assert [stage["name"] for stage in stages] == ["configured", "reconnected_base"]
        for stage in stages:
            checks = {item["name"]: item for item in stage["checks"]}
            assert checks["configure_return"]["status"] == "passed", stage
            assert stage["wire_evidence"]["transport_written_bytes"] > 0
            assert stage["wire_evidence"]["ubx_ack_frames"] > 0
            assert checks["requested_settings_readback"]["status"] == "inconclusive"
            settings = {item["setting"]: item for item in stage["requested_setting_observations"]}
            assert settings["time_mode"]["matching_write_observed"], stage
            assert not settings["time_mode"]["transaction_correlation_verified"]
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
            ["--action", "configure", "--fault", fault, "--observe-ms", "20"],
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
                *CANCEL_SLACK,
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
    check_record_and_replay(binary)
    print("Native GPS runner safety, survey provenance, failure and record/replay contracts passed")


if __name__ == "__main__":
    main()
