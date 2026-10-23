"""Tests for tools/simulation/mock_vehicle.py (needs pymavlink from the `simulation` group)."""

from __future__ import annotations

import threading
from typing import TYPE_CHECKING

import pytest

pytest.importorskip("pymavlink")

from pymavlink import mavutil
from pymavlink.dialects.v20 import common as mavlink
from simulation import mock_vehicle

if TYPE_CHECKING:
    from collections.abc import Iterator

TIMEOUT_SECONDS = 5


@pytest.fixture
def connections(monkeypatch: pytest.MonkeyPatch) -> list[tuple[str, str, int]]:
    calls: list[tuple[str, str, int]] = []
    monkeypatch.setattr(
        mock_vehicle.MockVehicle,
        "connect_udp",
        lambda _self, host, port: calls.append(("udp", host, port)),
    )
    monkeypatch.setattr(
        mock_vehicle.MockVehicle,
        "connect_tcp",
        lambda _self, host, port: calls.append(("tcp", host, port)),
    )
    monkeypatch.setattr(mock_vehicle.MockVehicle, "run", lambda _self, rate: None)
    return calls


@pytest.mark.parametrize(
    ("argv", "expected"),
    [
        ([], ("udp", "127.0.0.1", 14550)),
        (["--tcp", "--port", "5761"], ("tcp", "127.0.0.1", 5761)),
        (["--sitl", "--port", "5761"], ("tcp", "127.0.0.1", 5760)),
    ],
    ids=["udp", "tcp", "sitl"],
)
def test_main_selects_transport(
    connections, argv: list[str], expected: tuple[str, str, int]
) -> None:
    assert mock_vehicle.main(argv) == 0
    assert connections == [expected]


@pytest.fixture
def tcp_vehicle() -> Iterator[tuple[mock_vehicle.MockVehicle, int]]:
    vehicle = mock_vehicle.MockVehicle()
    vehicle.connect_tcp("127.0.0.1", 0)
    listener = vehicle.connection
    assert isinstance(listener, mavutil.mavtcpin)
    port = listener.listen.getsockname()[1]
    runner = threading.Thread(target=vehicle.run, kwargs={"rate": 50}, daemon=True)
    runner.start()
    yield vehicle, port
    vehicle.running = False
    runner.join(TIMEOUT_SECONDS)
    vehicle.connection.close()
    assert not runner.is_alive()


def test_tcp_mode_listens_for_a_client_and_handles_commands(tcp_vehicle) -> None:
    vehicle, port = tcp_vehicle
    client = mavutil.mavlink_connection(f"tcp:127.0.0.1:{port}", source_system=255)
    assert isinstance(client, mavutil.mavtcp)
    try:
        assert (
            client.recv_match(type="SYS_STATUS", blocking=True, timeout=TIMEOUT_SECONDS) is not None
        )

        client.mav.command_long_send(
            1, 1, mavlink.MAV_CMD_COMPONENT_ARM_DISARM, 0, 1, 0, 0, 0, 0, 0, 0
        )
        ack = client.recv_match(type="COMMAND_ACK", blocking=True, timeout=TIMEOUT_SECONDS)
        assert ack is not None
        assert (ack.command, ack.result) == (
            mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
            mavlink.MAV_RESULT_ACCEPTED,
        )
        assert vehicle.state.armed

        client.mav.set_mode_send(
            1, mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED, vehicle.modes["GUIDED"]
        )
        # The vehicle handles one message per loop in arrival order, so this ack follows SET_MODE.
        client.mav.command_long_send(
            1, 1, mavlink.MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES, 0, 1, 0, 0, 0, 0, 0, 0
        )
        assert (
            client.recv_match(type="AUTOPILOT_VERSION", blocking=True, timeout=TIMEOUT_SECONDS)
            is not None
        )
        assert vehicle.state.mode == "GUIDED"
    finally:
        client.close()
