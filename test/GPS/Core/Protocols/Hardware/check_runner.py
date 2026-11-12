"""Exercise the real runner entry point without opening physical transports."""

from __future__ import annotations

import json
import os
import re
import signal
import socket
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


class Simulator:
    """The runner serving a simulated receiver on a free port of the loopback interface."""

    def __init__(self, binary: str, arguments: list[str]) -> None:
        self.process = subprocess.Popen(
            [binary, "--serve", "0", *arguments],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        assert self.process.stdout is not None
        # The first line says where it listens.
        self.report = json.loads(self.process.stdout.readline())
        assert self.report["outcome"] == "serving" and self.report["port"] > 0, self.report

    def __enter__(self) -> Simulator:
        return self

    def __exit__(self, *_: object) -> None:
        if self.process.poll() is None:
            self.process.kill()
        self.process.communicate()

    def connect(self) -> socket.socket:
        return socket.create_connection(("127.0.0.1", self.report["port"]), timeout=5 * CI_SCALE)

    def stop(self) -> dict:
        """Interrupts the simulator and returns what it reports to have served; nothing on Windows."""
        if sys.platform == "win32":
            return {}
        self.process.send_signal(signal.SIGINT)
        stdout, stderr = self.process.communicate(timeout=10 * CI_SCALE)
        assert self.process.returncode == 0, (stdout, stderr)
        report = json.loads(stdout.splitlines()[-1])
        assert report["outcome"] == "served" and report["schema_version"] == 1, report
        return report


def received(link: socket.socket) -> bytes:
    """What the peer sends until it closes the connection."""
    data = b""
    while chunk := link.recv(65536):
        data += chunk
    return data


def ubx_frame(link: socket.socket) -> bytes:
    """One UBX frame from the peer: six header bytes that end with the payload length, the payload, a checksum."""
    data = b""
    while len(data) < 6 or len(data) < 8 + int.from_bytes(data[4:6], "little"):
        chunk = link.recv(65536)
        assert chunk, data
        data += chunk
    return data


def check_serve(binary: str) -> None:
    # The u-blox receiver model answers a driver's identity poll and ignores another family's.
    poll = bytes.fromhex("b5620a0400000e34")
    foreign = b"$PQTMVERNO*58\r\n"
    with Simulator(binary, []) as simulator:
        assert simulator.report["evidence_origin"] == "scripted", simulator.report
        assert simulator.report["address"] == "127.0.0.1", simulator.report
        with simulator.connect() as link:
            link.sendall(foreign + poll)
            reply = ubx_frame(link)
        assert reply.startswith(b"\xb5\x62\x0a\x04") and b"MOD=ZED-F9P" in reply, reply
        report = simulator.stop()
    if report:
        assert report["connections"] == 1 and report["scripted_wire_valid"], report
        assert report["received_bytes"] == len(foreign + poll), report
        assert report["sent_bytes"] == len(reply), report

    # The simulator serves a receiver and nothing else, on an address and a port.
    run(binary, ["--serve", "0", "--action", "configure"], 2)
    run(binary, ["--serve", "0", "--record", "recordings"], 2)
    run(binary, ["--serve", "65536"], 2)
    run(binary, ["--serve", "0", "--serve-address", "localhost"], 2)
    run(binary, ["--serve-address", "0.0.0.0"], 2)
    report = run(binary, ["--serve", "0", "--replay", "missing.log"], 1)
    assert report["outcome"] == "failed" and "missing.log" in report["detail"], report


def check_detected_session(binary: str) -> None:
    # A session of --family automatic is one the driver detects the receiver in, as the application's "Detect
    # automatically": the scripted receiver is found to be a u-blox one and configured as a base.
    with TemporaryDirectory(prefix="gps-detect-", dir=Path.cwd()) as directory:
        report = run(
            binary,
            [
                "--family",
                "automatic",
                "--action",
                "configure",
                "--observe-ms",
                "20",
                "--record",
                str(Path(directory) / "GPS"),
            ],
            3,
        )
        assert report["receiver_family"] == "automatic", report
        stage = report["stages"][0]
        checks = {item["name"]: item for item in stage["checks"]}
        assert checks["configure_return"]["status"] == "passed", stage
        assert stage["detected_family"] == "ublox", stage
        assert stage["receiver_identity"] == "ZED-F9P HPG 1.32", stage
        # The evidence of a u-blox receiver's settings, which the family the driver found decides.
        settings = {item["setting"]: item for item in stage["requested_setting_observations"]}
        assert settings["time_mode"]["matching_write_observed"], stage
        # Its recording is named as the application names a detected session's, and replays as one.
        recording = Path(report["recordings"][0])
        assert re.fullmatch(r"\d{8}T\d{6}\.\d{3}Z-automatic\.log", recording.name), recording
        replayed = run(
            binary, ["--replay", str(recording), "--dialogue", "--action", "configure"], 0
        )
        assert replayed["outcome"] == "reproduced" and replayed["configured"], replayed
        assert replayed["receiver_family"] == "automatic", replayed
        assert replayed["detected_family"] == "ublox", replayed
        # Decoding takes the decoder of one family, which a detected session does not name: the family its dialogue
        # reports does.
        refused = run(
            binary,
            ["--replay", str(recording), "--action", "configure", "--family", "automatic"],
            2,
        )
        assert "--dialogue" in refused["detail"] and "detected" in refused["detail"], refused
        decoded = run(
            binary,
            [
                "--replay",
                str(recording),
                "--action",
                "configure",
                "--family",
                replayed["detected_family"],
            ],
            0,
        )
        assert decoded["positions"]["fix_types"]["3d"] > 0, decoded
    # A session that configures a named family reports no detection.
    report = run(binary, ["--action", "configure", "--observe-ms", "20"], 3)
    assert "detected_family" not in report["stages"][0], report
    # Detection can take 45 s before a configuration that takes as long, so the deadline of a stage allows both.
    endpoint = ["--transport", "tcp", "--device", "127.0.0.1:1", "--family", "automatic"]
    report = run(binary, endpoint, 0)
    assert report["outcome"] == "not_run" and report["requested"]["role"] == "base", report
    assert report["deadlines"]["open_and_configure_ms"] == 105000, report
    report = run(binary, [*endpoint, "--timeout-ms", "1000"], 0)
    assert report["deadlines"]["open_and_configure_ms"] == 1000, report
    # Passive input is decoded as it comes: there is no receiver to detect.
    refused = run(binary, [*endpoint, "--role", "passive", "--baud", "115200"], 2)
    assert "--family passive" in refused["detail"], refused


def numbers(value: object) -> Iterator[float]:
    if isinstance(value, dict):
        for item in cast("dict[str, object]", value).values():
            yield from numbers(item)
    elif isinstance(value, list):
        for item in cast("list[object]", value):
            yield from numbers(item)
    elif isinstance(value, (int, float)) and not isinstance(value, bool):
        yield float(value)


def at_second(sentence: bytes, second: int) -> bytes:
    """An NMEA sentence with its time moved to the given second of its minute."""
    body = sentence[1 : sentence.index(b"*")]
    body = re.sub(rb",(\d{4})\d\d\b", b",\\g<1>%02d" % second, body, count=1)
    checksum = 0
    for byte in body:
        checksum ^= byte
    return b"$%s*%02X\r\n" % (body, checksum)


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
        recording = Path(report["recordings"][0])
        assert re.fullmatch(r"\d{8}T\d{6}\.\d{3}Z-ublox\.log", recording.name), recording
        # The passive decoder reads the recorded UBX fix and survey status too.
        report = run(binary, ["--replay", str(recording), *passive], 0)
        replayed = report["replay"]
        assert report["outcome"] == "replayed" and replayed["complete"], report
        # The runner reads the file with the C++ reader of the format, which rejects any line that is not a record.
        # The session's commands and the baud rate it set are in the file with what the receiver sent.
        assert replayed["session_recording"], report
        assert (
            replayed["bytes"] > 0 and replayed["sent_bytes"] > 0 and replayed["baud_changes"] > 0
        ), report
        assert report["positions"]["fix_types"]["3d"] > 0, report
        assert report["survey"]["messages"] > 0, report
        # The simulator plays what the receiver sent to a client, and closes the connection after the last byte.
        with Simulator(binary, ["--replay", str(recording)]) as simulator:
            assert simulator.report["evidence_origin"] == "recording", simulator.report
            with simulator.connect() as link:
                played = received(link)
            served = simulator.stop()
        assert len(played) == replayed["bytes"] and played.startswith(b"\xb5\x62"), played
        if served:
            assert served["connections"] == 1 and served["sent_bytes"] == len(played), served
        # The receiver's side of the recording, extracted as raw receiver output, decodes to the same summaries.
        extracted = Path(directory) / "received.ubx"
        again = run(binary, ["--replay", str(recording), "--extract", str(extracted), *passive], 0)
        assert again["replay"]["extracted"] == str(extracted), again
        assert extracted.read_bytes() == played, again
        raw = run(binary, ["--replay", str(extracted), *passive], 0)
        assert not raw["replay"]["session_recording"], raw
        for summary in ("positions", "satellites", "corrections", "survey"):
            assert raw[summary] == report[summary], (raw, report)
        # An extract never replaces a file.
        refused = run(
            binary, ["--replay", str(recording), "--extract", str(extracted), *passive], 1
        )
        assert refused["outcome"] == "failed" and "positions" not in refused, refused
        report = run(binary, ["--replay", str(recording), *replay, "--family", "ublox"], 0)
        assert report["outcome"] == "replayed" and report["replay"]["decode_only"], report
        assert report["positions"]["fix_types"]["3d"] > 0, report
        assert report["survey"]["messages"] > 0 and "active" in report["survey"], report
        assert "requested" not in report, report
        # The scripted fix is at 47.3, 8.5.
        assert not {47.3, 8.5} & set(numbers(report)), report
        # The recording as the dialogue it was: the driver, run again as the recording's settings describe, sends in
        # answer to the receiver's bytes what the recording holds. The session ran on the steady clock.
        dialogue = ["--replay", str(recording), "--dialogue", "--action", "configure"]
        report = run(binary, dialogue, 0)
        assert report["outcome"] == "reproduced" and report["configured"], report
        assert report["receiver_family"] == "ublox", report
        assert report["requested"]["base_mode"] == "survey", report
        # The scripted receiver is on a port that runs at one rate, as a network bridge does.
        assert report["requested"]["fixed_baud_rate"] == 115200, report
        assert report["receiver_identity"] == "ZED-F9P HPG 1.32", report
        for count in ("bytes", "sent_bytes", "baud_changes"):
            assert report["replay"][count] == replayed[count], (report, replayed)
        # A recording that holds another command than the driver sends fails at that command's line.
        lines = recording.read_text().splitlines(keepends=True)
        command = next(index for index, line in enumerate(lines) if " TX b5" in line)
        lines[command] = lines[command].replace(" TX b5", " TX b4")
        changed = Path(directory) / "changed.log"
        changed.write_text("".join(lines))
        report = run(binary, ["--replay", str(changed), "--dialogue", "--action", "configure"], 1)
        assert report["outcome"] == "diverged", report
        assert report["divergence"]["line"] == command + 1, report
        assert report["divergence"]["recorded"].startswith("TX "), report
        assert report["divergence"]["driver"].startswith("wrote "), report
        assert report["replay"]["records"] == command - 2, report
        # The settings are the recording's own: raw receiver output has none, and no option replaces them.
        report = run(binary, ["--replay", str(extracted), "--dialogue", "--action", "configure"], 1)
        assert report["outcome"] == "failed" and "divergence" not in report, report
        assert report["detail"] == "not a receiver recording", report
        run(binary, [*dialogue, "--family", "ublox"], 2)
        run(binary, [*dialogue, "--extract", str(Path(directory) / "again.ubx")], 2)
        run(binary, ["--dialogue", "--action", "configure"], 2)
        # A suite is two sessions of the driver. Each has a recording of its own, which replays as one dialogue.
        report = run(
            binary,
            [
                "--action",
                "suite",
                "--observe-ms",
                "20",
                *CANCEL_SLACK,
                "--record",
                str(Path(directory) / "suite"),
            ],
            3,
        )
        sessions = report["recordings"]
        assert len(sessions) == len(report["stages"]) == len(set(sessions)) == 2, report
        for session in sessions:
            replayed = run(binary, ["--replay", session, "--dialogue", "--action", "configure"], 0)
            assert replayed["outcome"] == "reproduced" and replayed["configured"], replayed

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
    assert not report["replay"]["session_recording"] and "sent_bytes" not in report["replay"], (
        report
    )
    assert report["positions"]["messages"] > 0, report
    # The fixture's fixes have one time and come from several talkers, which makes all but the first a repeat of it
    # for another constellation. Each at its own second is a position.
    with TemporaryDirectory(prefix="gps-fixes-", dir=Path.cwd()) as directory:
        fixes = Path(directory) / "fixes.nmea"
        sentences = capture.read_bytes().splitlines()
        fixes.write_bytes(
            b"".join(at_second(line, second) for second, line in enumerate(sentences))
        )
        report = run(binary, ["--replay", str(fixes), *passive], 0)
    assert report["positions"]["fix_types"]["rtk-fixed"] == 1, report
    # Summaries only: no coordinate field, and none of the synthetic fixes' coordinates.
    assert "latitude" not in json.dumps(report) and "longitude" not in json.dumps(report), report
    assert not {12.5, -12.5, 45.25, -45.25} & set(numbers(report)), report
    paced = run(binary, ["--replay", str(capture), "--replay-baud", "38400", *passive], 0)
    assert paced["replay"]["elapsed_ms"] >= capture.stat().st_size * 10 * 1000 // 38400 - 1, paced
    report = run(binary, ["--replay", str(corpus / "missing.nmea"), *passive], 1)
    assert report["outcome"] == "failed" and "positions" not in report

    run(binary, ["--replay", str(capture), "--transport", "tcp", *passive], 2)
    run(binary, ["--replay", str(capture), "--record", "recordings", *passive], 2)
    # The options of the scripted receiver do not apply to a recording, which takes its place, and the message says
    # which option that is and what to leave out. A recording is not a physical transport, which refuses them too.
    for option in (["--fault", "nak"], ["--model", "m8p"], ["--survey-state", "none"]):
        refused = run(binary, ["--replay", str(capture), *option, *replay], 2)
        assert refused["detail"].startswith(f"{option[0]} describes the scripted receiver"), refused
        assert "physical" not in refused["detail"], refused
        assert f"leave {option[0]} out" in refused["detail"], refused
    refused = run(binary, ["--serve", "0", "--replay", str(capture), "--model", "m8p"], 2)
    assert "leave --model out" in refused["detail"], refused
    refused = run(binary, ["--transport", "tcp", "--device", "127.0.0.1:1", "--model", "m8p"], 2)
    assert refused["detail"].startswith("--model describes the scripted receiver"), refused
    assert "physical transport" in refused["detail"], refused
    run(binary, ["--replay", str(capture), "--action", "suite", *passive[2:]], 2)
    run(binary, ["--replay-baud", "9600"], 2)
    run(binary, ["--extract", "received.ubx"], 2)


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
        # Without a rate, a passive receiver's is found by listening; a rate no receiver has is refused.
        report = run(binary, [*endpoint, "--family", "passive", "--role", "passive"], 0)
        assert report["requested"]["role"] == "passive"
        run(binary, [*endpoint, "--family", "passive", "--role", "passive", "--baud", "100"], 2)
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

    for model, identity in (("f9p", "ZED-F9P HPG 1.32"), ("m8p", "NEO-M8P-2 HPG 1.40")):
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
            # What a qualification row names: the model and firmware the receiver reported.
            assert stage["receiver_identity"] == identity, stage
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
    check_detected_session(binary)
    check_serve(binary)
    print(
        "Native GPS runner safety, survey provenance, failure, record/replay and simulator contracts passed"
    )


if __name__ == "__main__":
    main()
