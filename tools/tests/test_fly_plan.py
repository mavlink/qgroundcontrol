"""Tests for tools/simulation/fly_plan.py (needs pymavlink from the `simulation` group)."""

from __future__ import annotations

import json
import math
from typing import TYPE_CHECKING, Any, cast

import pytest

pytest.importorskip("pymavlink")

from simulation import fly_plan

if TYPE_CHECKING:
    from pathlib import Path

mavlink = fly_plan.mavlink
HOME = [47.0, 8.0, 400.0]


def _simple_item(command: int, frame: int, params: list[float | None], jump_id: int) -> dict:
    return {
        "type": "SimpleItem",
        "command": command,
        "frame": frame,
        "params": params,
        "autoContinue": True,
        "doJumpId": jump_id,
    }


def _waypoint(lat: float, lon: float, jump_id: int) -> dict:
    return _simple_item(
        mavlink.MAV_CMD_NAV_WAYPOINT,
        mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT,
        [0, 0, 0, None, lat, lon, 30],
        jump_id,
    )


def _write_plan(tmp_path: Path, items: list[dict]) -> Path:
    plan = tmp_path / "test.plan"
    plan.write_text(
        json.dumps({"mission": {"items": items, "plannedHomePosition": HOME}}), encoding="utf-8"
    )
    return plan


def _survey(*items: dict) -> dict:
    return {
        "type": "ComplexItem",
        "complexItemType": "survey",
        "TransectStyleComplexItem": {"Items": list(items)},
    }


class FakeMessage:
    def __init__(self, kind: str, src_system: int = 1, **fields: Any) -> None:
        self._kind = kind
        self._src_system = src_system
        self.__dict__.update(fields)

    def __getattr__(self, name: str) -> Any:
        # Only reached for fields the message was not given
        raise AttributeError(name)

    def get_type(self) -> str:
        return self._kind

    def get_srcSystem(self) -> int:
        return self._src_system


class FakeMav:
    def __init__(self) -> None:
        self.sent: list[tuple[str, tuple]] = []

    def __getattr__(self, name: str):
        return lambda *args: self.sent.append((name, args))


class FakeMaster:
    """Delivers queued messages like pymavlink: every message reaches the hooks, matching ones return."""

    target_system = 1
    target_component = 1

    def __init__(self, messages: list[FakeMessage]) -> None:
        self.mav = FakeMav()
        self.message_hooks: list = []
        self.flightmode = "GUIDED"
        self._messages = messages

    def mode_mapping(self) -> dict[str, int]:
        return {"GUIDED": 4, "AUTO": 3}

    def set_mode(self, mode: int) -> None:
        self.mav.sent.append(("set_mode", (mode,)))

    def recv_match(
        self, type: str | list[str] | None = None, blocking: bool = False, timeout: float = 0
    ):
        types = [type] if isinstance(type, str) else type
        while self._messages:
            msg = self._messages.pop(0)
            if msg.get_type() == "HEARTBEAT":
                self.flightmode = msg.mode
            for hook in self.message_hooks:
                hook(self, msg)
            if types is None or msg.get_type() in types:
                return msg
        return None


def test_load_plan_expands_saved_complex_items(tmp_path: Path) -> None:
    takeoff = _simple_item(
        mavlink.MAV_CMD_NAV_TAKEOFF, mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT, [0] * 7, 1
    )
    plan = _write_plan(
        tmp_path, [takeoff, _survey(_waypoint(47.1, 8.1, 7), _waypoint(47.2, 8.2, 8))]
    )

    home, items = fly_plan.load_plan(plan)

    assert home == tuple(HOME)
    assert [item.command for item in items] == [
        mavlink.MAV_CMD_NAV_TAKEOFF,
        mavlink.MAV_CMD_NAV_WAYPOINT,
        mavlink.MAV_CMD_NAV_WAYPOINT,
    ]
    assert math.isnan(items[1].params[3])
    # Only top level items are DO_JUMP targets, as in QGC
    assert [item.do_jump_id for item in items] == [1, 0, 0]


def test_load_plan_rejects_complex_items_without_saved_items(tmp_path: Path) -> None:
    plan = _write_plan(tmp_path, [{"type": "ComplexItem", "complexItemType": "StructureScan"}])

    with pytest.raises(ValueError, match="StructureScan"):
        fly_plan.load_plan(plan)
    with pytest.raises(SystemExit):
        fly_plan.main([str(plan), "--connect", "udpin:127.0.0.1:0"])


@pytest.mark.parametrize(
    ("autopilot", "first_seq"),
    [(mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA, 1), (mavlink.MAV_AUTOPILOT_PX4, 0)],
    ids=["ardupilot", "px4"],
)
def test_mission_items_number_like_qgc(tmp_path: Path, autopilot: int, first_seq: int) -> None:
    jump = _simple_item(
        mavlink.MAV_CMD_DO_JUMP, mavlink.MAV_FRAME_MISSION, [5, 2, 0, 0, 0, 0, 0], 6
    )
    plan = _write_plan(tmp_path, [_waypoint(47.1, 8.1, 4), _waypoint(47.2, 8.2, 5), jump])
    home, items = fly_plan.load_plan(plan)

    mission = fly_plan.mission_items(autopilot, home, items)

    assert len(mission) == first_seq + 3
    if first_seq:
        assert mission[0].command == mavlink.MAV_CMD_NAV_WAYPOINT
        assert mission[0].params[4:] == HOME
    assert mission[-1].params[:2] == [first_seq + 1, 2]


def test_upload_answers_requests() -> None:
    items = [
        fly_plan.PlanItem(
            mavlink.MAV_CMD_NAV_WAYPOINT,
            mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT,
            [0, 0, 0, math.nan, 47.1234567, 8.7654321, 30],
        ),
        fly_plan.PlanItem(
            mavlink.MAV_CMD_DO_SET_CAM_TRIGG_DIST, mavlink.MAV_FRAME_MISSION, [10, 0, 1, 0, 0, 0, 0]
        ),
    ]
    master = FakeMaster(
        [
            FakeMessage("MISSION_REQUEST_INT", seq=0),
            FakeMessage("MISSION_REQUEST", seq=1),
            FakeMessage("MISSION_ACK", type=mavlink.MAV_MISSION_ACCEPTED),
        ]
    )

    fly_plan.upload(cast("Any", master), items)

    sent = master.mav.sent
    assert sent[0] == ("mission_count_send", (1, 1, 2, mavlink.MAV_MISSION_TYPE_MISSION))
    waypoint, trigger = sent[1][1], sent[2][1]
    assert waypoint[2:5] == (0, mavlink.MAV_FRAME_GLOBAL_RELATIVE_ALT, mavlink.MAV_CMD_NAV_WAYPOINT)
    assert waypoint[11:13] == (471234567, 87654321)
    assert trigger[7:9] == (10, 0)


def test_upload_reports_rejected_item() -> None:
    items = [fly_plan.PlanItem(196, mavlink.MAV_FRAME_MISSION, [0] * 7)]
    master = FakeMaster(
        [
            FakeMessage("MISSION_REQUEST_INT", seq=0),
            FakeMessage("MISSION_ACK", type=mavlink.MAV_MISSION_UNSUPPORTED),
        ]
    )

    with pytest.raises(RuntimeError, match=r"at item 0 \(196\): MAV_MISSION_UNSUPPORTED"):
        fly_plan.upload(cast("Any", master), items)


def test_recording_tracks_images_and_completion() -> None:
    recording = fly_plan.Recording()
    for msg in [
        FakeMessage("MISSION_CURRENT", seq=3, mission_state=3),
        FakeMessage(
            "GLOBAL_POSITION_INT", lat=471000000, lon=81000000, relative_alt=30000, hdg=9050
        ),
        FakeMessage("CAMERA_FEEDBACK", img_idx=1, lat=471000000, lng=81000000, alt_rel=30.0),
        FakeMessage(
            "CAMERA_IMAGE_CAPTURED", image_index=2, lat=471000100, lon=81000100, relative_alt=29500
        ),
        FakeMessage(
            "CAMERA_IMAGE_CAPTURED", image_index=2, lat=471000100, lon=81000100, relative_alt=29500
        ),
        FakeMessage("STATUSTEXT", text="Mission: 4 WP"),
    ]:
        recording.handle(msg, 1.0)
    assert not recording.complete

    recording.handle(FakeMessage("MISSION_CURRENT", seq=4, mission_state=5), 2.0)

    assert recording.complete
    # A complete state left over from a previous mission does not end a new one
    stale = fly_plan.Recording()
    stale.handle(FakeMessage("MISSION_CURRENT", seq=9, mission_state=5), 0.0)
    assert not stale.complete
    assert recording.track == [
        {"time": 1.0, "lat": 47.1, "lon": 8.1, "alt": 30.0, "heading": 90.5, "seq": 3}
    ]
    assert [image["index"] for image in recording.images] == [1, 2]
    assert recording.images[1]["alt"] == 29.5
    assert recording.statustext == ["Mission: 4 WP"]


def test_write_svg_plots_paths_and_images(tmp_path: Path) -> None:
    mission = [
        fly_plan.PlanItem(
            mavlink.MAV_CMD_NAV_WAYPOINT, mavlink.MAV_FRAME_GLOBAL, [0, 0, 0, 0, 47.001, 8.001, 30]
        ),
        fly_plan.PlanItem(
            mavlink.MAV_CMD_DO_SET_ROI_LOCATION,
            mavlink.MAV_FRAME_GLOBAL,
            [0, 0, 0, 0, 47.5, 8.5, 30],
        ),
        fly_plan.PlanItem(mavlink.MAV_CMD_NAV_RETURN_TO_LAUNCH, mavlink.MAV_FRAME_MISSION, [0] * 7),
    ]
    recording = fly_plan.Recording(
        track=[{"lat": 47.0, "lon": 8.0}, {"lat": 47.001, "lon": 8.001}],
        images=[{"lat": 47.0005, "lon": 8.0005}],
    )
    svg = tmp_path / "flight.svg"

    fly_plan.write_svg(svg, "a <b> plan", (47.0, 8.0, 400.0), mission, recording)

    text = svg.read_text(encoding="utf-8")
    assert "a &lt;b&gt; plan" in text
    assert text.count("<polyline") == 2
    assert text.count("<circle") == 1
    # Only navigation items are on the planned path
    assert fly_plan.planned_path(mission) == [(47.001, 8.001)]


def test_recording_keeps_messages_read_while_starting(capsys: pytest.CaptureFixture[str]) -> None:
    master = FakeMaster(
        [
            FakeMessage("CAMERA_FEEDBACK", img_idx=1, lat=471000000, lng=81000000, alt_rel=20.0),
            FakeMessage("CAMERA_FEEDBACK", src_system=2, img_idx=7, lat=0, lng=0, alt_rel=0.0),
            FakeMessage("HEARTBEAT", mode="AUTO"),
            FakeMessage("MISSION_CURRENT", seq=5, mission_state=3),
            FakeMessage("MISSION_CURRENT", seq=6, mission_state=5),
        ]
    )
    recording = fly_plan.start_recording(cast("Any", master))

    # An image reported before the heartbeat confirming AUTO is still recorded
    fly_plan.set_mode(cast("Any", master), "AUTO")
    assert [image["index"] for image in recording.images] == [1]

    fly_plan.wait_for_completion(cast("Any", master), recording, timeout=5)
    assert recording.complete
    assert "mission item 6" in capsys.readouterr().out


def test_camera_feedback_is_not_streamed(monkeypatch: pytest.MonkeyPatch) -> None:
    requested: list[float] = []
    monkeypatch.setattr(
        fly_plan,
        "command_long",
        lambda _master, command, *params, **_: requested.append(params[0]) or 0,
    )

    fly_plan.request_messages(cast("Any", FakeMaster([])))

    assert mavlink.MAVLINK_MSG_ID_CAMERA_FEEDBACK not in requested
    assert mavlink.MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED in requested
