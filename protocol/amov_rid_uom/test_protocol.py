#!/usr/bin/env python3
"""Exchange activation bridge frames across independent common/all dialect builds."""

import argparse
import hashlib
import importlib.util
import shutil
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

HERE = Path(__file__).resolve().parent
SETTINGS = None
HARNESS = r"""
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <mavlink.h>

static int send_message(mavlink_message_t *message)
{
    uint8_t wire[MAVLINK_MAX_PACKET_LEN];
    unsigned length = mavlink_msg_to_send_buffer(wire, message);
    return fwrite(wire, 1, length, stdout) == length ? 0 : 1;
}

int main(int argc, char **argv)
{
    (void)argv;
    mavlink_amov_device_request_t request;
    mavlink_amov_device_response_t response;
    memset(&request, 0, sizeof(request));
    memset(&response, 0, sizeof(response));
    request.session_id = UINT64_C(0xfedcba9876543210);
    request.request_id = 0xfedcba98;
    request.target_system = 42;
    request.target_component = 1;
    request.protocol_version = 1;
    request.operation = AMOV_DEVICE_OPERATION_TRANSFER;
    request.node_id = 127;
    request.payload_length = 192;
    response.session_id = request.session_id;
    response.request_id = request.request_id;
    response.target_system = 255;
    response.target_component = 190;
    response.protocol_version = 1;
    response.operation = AMOV_DEVICE_OPERATION_TRANSFER;
    response.node_id = 127;
    response.payload_length = 192;
    response.status = AMOV_DEVICE_STATUS_OK;
    for (unsigned n = 0; n < 192; ++n) {
        request.payload[n] = (uint8_t)(0x80 + n);
        response.payload[n] = (uint8_t)(0x20 + n);
    }
    mavlink_message_t message;
    if (argc == 1) {
        mavlink_msg_amov_device_request_encode(255, 190, &message, &request);
        if (send_message(&message)) return 1;
        mavlink_msg_amov_device_response_encode(42, 1, &message, &response);
        return send_message(&message);
    }
    int count = 0, byte;
    mavlink_status_t parse_status;
    memset(&parse_status, 0, sizeof(parse_status));
    while ((byte = getchar()) != EOF) {
        if (!mavlink_parse_char(MAVLINK_COMM_0, (uint8_t)byte, &message, &parse_status)) continue;
        if (argc == 3) { ++count; continue; }
        if (count == 0 && message.msgid == MAVLINK_MSG_ID_AMOV_DEVICE_REQUEST) {
            mavlink_amov_device_request_t value;
            mavlink_msg_amov_device_request_decode(&message, &value);
            if (memcmp(&value, &request, MAVLINK_MSG_ID_AMOV_DEVICE_REQUEST_LEN)) return 2;
        } else if (count == 1 && message.msgid == MAVLINK_MSG_ID_AMOV_DEVICE_RESPONSE) {
            mavlink_amov_device_response_t value;
            mavlink_msg_amov_device_response_decode(&message, &value);
            if (memcmp(&value, &response, MAVLINK_MSG_ID_AMOV_DEVICE_RESPONSE_LEN)) return 3;
        } else return 4;
        ++count;
    }
    if (argc == 3) { printf("%d\n", count); return 0; }
    return count == 2 ? 0 : 5;
}
"""


def digest_tree(directory):
    return {p.name: hashlib.sha256(p.read_bytes()).digest() for p in directory.glob("*.xml")}


class ProtocolTest(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.temporary = tempfile.TemporaryDirectory(prefix="amov-protocol-")
        cls.addClassCleanup(cls.temporary.cleanup)
        cls.build = Path(cls.temporary.name)
        cls.executables = []
        source = cls.build / "exchange.c"
        source.write_text(HARNESS, encoding="utf-8")
        for label, definitions, dialect, generator_root in (
            ("common", SETTINGS.definitions, "common", SETTINGS.pymavlink_root),
            (
                "all",
                SETTINGS.definitions,
                "all",
                SETTINGS.pymavlink_root,
            ),
        ):
            before = digest_tree(definitions)
            output = cls.build / label
            command = [
                sys.executable,
                str(HERE / "generate.py"),
                "--definitions",
                str(definitions),
                "--dialect",
                dialect,
                "--output",
                str(output),
                "--work-dir",
                str(cls.build / (label + "-xml")),
            ]
            if generator_root:
                command += ["--pymavlink-root", str(generator_root)]
            subprocess.run(command, check=True, stdout=subprocess.DEVNULL)
            if before != digest_tree(definitions):
                raise AssertionError("generator changed upstream definitions")
            executable = cls.build / (label + "-exchange")
            subprocess.run(
                [
                    SETTINGS.cc,
                    "-std=c11",
                    "-Wall",
                    "-Wextra",
                    "-Werror",
                    "-Wno-address-of-packed-member",
                    "-I" + str(output / dialect),
                    str(source),
                    "-o",
                    str(executable),
                ],
                check=True,
            )
            cls.executables.append(executable)

    def test_bidirectional_frames(self):
        frames = []
        for encoder in self.executables:
            wire = subprocess.check_output([str(encoder)])
            frames.append(wire)
            for decoder in self.executables:
                subprocess.run([str(decoder), "decode"], input=wire, check=True)
        self.assertEqual(frames[0], frames[1])

    def test_corrupt_frame_rejected(self):
        wire = bytearray(subprocess.check_output([str(self.executables[0])]))
        wire[12] ^= 0x40
        for decoder in self.executables:
            count = subprocess.check_output([str(decoder), "decode", "count"], input=wire)
            self.assertEqual(count.strip(), b"1")

    def test_wire_contract(self):
        for label, dialect in (("common", "common"), ("all", "all")):
            for name, length, crc in (
                ("AMOV_DEVICE_REQUEST", 210, 199),
                ("AMOV_DEVICE_RESPONSE", 211, 62),
            ):
                header = (
                    self.build / label / dialect / ("mavlink_msg_" + name.lower() + ".h")
                ).read_text()
                self.assertIn("#define MAVLINK_MSG_ID_" + name + "_LEN " + str(length), header)
                self.assertIn("#define MAVLINK_MSG_ID_" + name + "_CRC " + str(crc), header)

    def test_collision_rejected(self):
        spec = importlib.util.spec_from_file_location("amov_generate", HERE / "generate.py")
        generator = importlib.util.module_from_spec(spec)
        spec.loader.exec_module(generator)
        upstream = self.build / "conflict"
        upstream.mkdir(exist_ok=True)
        shutil.copyfile(HERE / "amov.xml", upstream / "common.xml")
        with self.assertRaisesRegex(ValueError, "collision"):
            generator.overlay(
                upstream, "common", HERE / "amov.xml", self.build / "collision-output"
            )


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--definitions", required=True, type=Path)
    parser.add_argument("--pymavlink-root", type=Path)
    parser.add_argument("--cc", default="cc")
    SETTINGS, rest = parser.parse_known_args()
    unittest.main(argv=[sys.argv[0], *rest])
