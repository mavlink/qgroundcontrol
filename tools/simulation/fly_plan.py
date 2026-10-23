#!/usr/bin/env python3
"""
Fly a QGroundControl .plan in ArduPilot or PX4 SITL and record where it flew and took images.

Uploads the plan's mission, including the items QGC saved for surveys and corridor scans, flies
it in AUTO, and writes the flown track and the position of every image the autopilot reports to
JSON, optionally with an SVG of the planned and flown paths. This checks mission generation
against a real autopilot without driving the QGC user interface.

Usage:
    ./fly_plan.py survey.plan --connect tcp:127.0.0.1:5762            # ArduPilot SITL SERIAL1
    ./fly_plan.py survey.plan --connect udpin:0.0.0.0:14540 --svg flight.svg  # PX4 SITL

Images are only reported with a camera configured: ArduPilot CAM1_TYPE=1 (reboot after setting
it), PX4 TRIG_MODE=4 and TRIG_INTERFACE=3.

Requirements:
    python3 tools/setup/install_python.py simulation
"""

from __future__ import annotations

import argparse
import json
import math
import os
import sys
import time
from dataclasses import dataclass, field, replace
from pathlib import Path
from typing import Any
from xml.sax.saxutils import escape

try:
    from pymavlink import mavutil
    from pymavlink.dialects.v20 import ardupilotmega as mavlink
except ImportError:
    print("Error: pymavlink not installed")
    print("Install with: python3 tools/setup/install_python.py simulation")
    sys.exit(1)

AUTOPILOT_NAMES = {
    mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA: "ArduPilot",
    mavlink.MAV_AUTOPILOT_PX4: "PX4",
}
EARTH_RADIUS = 6378137.0
MISSION_STATE_ACTIVE = 3
MISSION_STATE_COMPLETE = 5
UINT16_MAX = 65535
RECORDED_MESSAGES = [
    "GLOBAL_POSITION_INT",
    "MISSION_CURRENT",
    "CAMERA_FEEDBACK",
    "CAMERA_IMAGE_CAPTURED",
    "STATUSTEXT",
]


@dataclass
class PlanItem:
    command: int
    frame: int
    params: list[float]
    autocontinue: bool = True
    do_jump_id: int = 0


def _plan_item(item: dict[str, Any], jump_target: bool) -> PlanItem:
    return PlanItem(
        command=int(item["command"]),
        frame=int(item["frame"]),
        params=[math.nan if value is None else float(value) for value in item["params"]],
        autocontinue=bool(item.get("autoContinue", True)),
        do_jump_id=int(item.get("doJumpId", 0)) if jump_target else 0,
    )


def load_plan(path: Path) -> tuple[tuple[float, float, float], list[PlanItem]]:
    """Return the planned home position and the mission items, with complex items expanded."""
    mission = json.loads(path.read_text(encoding="utf-8"))["mission"]
    items: list[PlanItem] = []
    for item in mission["items"]:
        if item["type"] == "SimpleItem":
            items.append(_plan_item(item, jump_target=True))
            continue
        saved_items = item.get("TransectStyleComplexItem", {}).get("Items")
        if not saved_items:
            raise ValueError(
                f"the plan does not save the mission items of its {item.get('complexItemType')} "
                "item; upload it from QGC instead"
            )
        items.extend(_plan_item(saved_item, jump_target=False) for saved_item in saved_items)
    home = mission["plannedHomePosition"]
    return (float(home[0]), float(home[1]), float(home[2])), items


def mission_items(
    autopilot: int, home: tuple[float, float, float], items: list[PlanItem]
) -> list[PlanItem]:
    """Number the mission as QGC uploads it, with DO_JUMP targets as sequence numbers."""
    # ArduPilot takes the home position as item 0; PX4 does not
    send_home = autopilot == mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA
    first_seq = 1 if send_home else 0
    jump_targets = {
        item.do_jump_id: seq
        for seq, item in enumerate(items, start=first_seq)
        if item.do_jump_id > 0
    }
    mission: list[PlanItem] = []
    if send_home:
        mission.append(
            PlanItem(mavlink.MAV_CMD_NAV_WAYPOINT, mavlink.MAV_FRAME_GLOBAL, [0, 0, 0, 0, *home])
        )
    for item in items:
        if item.command == mavlink.MAV_CMD_DO_JUMP:
            target = jump_targets.get(int(item.params[0]))
            if target is None:
                raise ValueError(f"DO_JUMP target {int(item.params[0])} is not in the plan")
            item = replace(item, params=[float(target), *item.params[1:]])
        mission.append(item)
    return mission


def _position(item: PlanItem, index: int) -> int:
    value = item.params[index]
    if math.isnan(value):
        return 0
    return int(value) if item.frame == mavlink.MAV_FRAME_MISSION else round(value * 1e7)


def upload(master: mavutil.mavfile, items: list[PlanItem], timeout: float = 10.0) -> None:
    master.mav.mission_count_send(
        master.target_system,
        master.target_component,
        len(items),
        mavlink.MAV_MISSION_TYPE_MISSION,
    )
    last_seq = None
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        msg = master.recv_match(
            type=["MISSION_REQUEST_INT", "MISSION_REQUEST", "MISSION_ACK"],
            blocking=True,
            timeout=1,
        )
        if msg is None:
            continue
        if msg.get_type() == "MISSION_ACK":
            if msg.type == mavlink.MAV_MISSION_ACCEPTED:
                return
            result = mavlink.enums["MAV_MISSION_RESULT"][msg.type].name
            detail = "" if last_seq is None else f" at item {last_seq} ({items[last_seq].command})"
            raise RuntimeError(f"mission rejected{detail}: {result}")
        if not 0 <= msg.seq < len(items):
            raise RuntimeError(f"vehicle requested item {msg.seq} of {len(items)}")
        item = items[msg.seq]
        master.mav.mission_item_int_send(
            master.target_system,
            master.target_component,
            msg.seq,
            item.frame,
            item.command,
            0,
            int(item.autocontinue),
            *item.params[:4],
            _position(item, 4),
            _position(item, 5),
            item.params[6],
            mavlink.MAV_MISSION_TYPE_MISSION,
        )
        last_seq = msg.seq
        deadline = time.monotonic() + timeout
    raise TimeoutError("mission upload timed out")


def command_long(
    master: mavutil.mavfile, command: int, *params: float, timeout: float = 5.0
) -> int | None:
    values = [*params, *([0.0] * (7 - len(params)))]
    master.mav.command_long_send(master.target_system, master.target_component, command, 0, *values)
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        ack = master.recv_match(type="COMMAND_ACK", blocking=True, timeout=1)
        if ack is not None and ack.command == command:
            return ack.result
    return None


def wait_autopilot(master: mavutil.mavfile, timeout: float = 30.0) -> int:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        msg = master.recv_match(type="HEARTBEAT", blocking=True, timeout=1)
        if msg is not None and msg.autopilot in AUTOPILOT_NAMES:
            master.target_system = msg.get_srcSystem()
            master.target_component = msg.get_srcComponent()
            return msg.autopilot
    raise TimeoutError("no ArduPilot or PX4 heartbeat")


def set_mode(master: mavutil.mavfile, mode: str, timeout: float = 10.0) -> None:
    mapping = master.mode_mapping() or {}
    if mode not in mapping:
        raise RuntimeError(f"vehicle has no {mode} mode")
    master.set_mode(mapping[mode])
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        master.recv_match(type="HEARTBEAT", blocking=True, timeout=1)
        if master.flightmode == mode:
            return
    raise TimeoutError(f"vehicle did not change to {mode}")


def arm(master: mavutil.mavfile, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        result = command_long(master, mavlink.MAV_CMD_COMPONENT_ARM_DISARM, 1)
        if result == mavlink.MAV_RESULT_ACCEPTED:
            return
        time.sleep(2)
    raise TimeoutError("vehicle did not arm; check its pre-arm messages")


def start_mission(
    master: mavutil.mavfile, autopilot: int, takeoff_alt: float, timeout: float
) -> None:
    if autopilot == mavlink.MAV_AUTOPILOT_PX4:
        arm(master, timeout)
        if command_long(master, mavlink.MAV_CMD_MISSION_START, 0, 0) != mavlink.MAV_RESULT_ACCEPTED:
            raise RuntimeError("vehicle did not start the mission")
        return

    # ArduCopter does not arm in AUTO by default, so take off in GUIDED first
    set_mode(master, "GUIDED")
    arm(master, timeout)
    result = command_long(master, mavlink.MAV_CMD_NAV_TAKEOFF, 0, 0, 0, 0, 0, 0, takeoff_alt)
    if result != mavlink.MAV_RESULT_ACCEPTED:
        raise RuntimeError("vehicle did not take off")
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        position = master.recv_match(type="GLOBAL_POSITION_INT", blocking=True, timeout=1)
        if position is not None and position.relative_alt / 1000 >= takeoff_alt * 0.9:
            set_mode(master, "AUTO")
            return
    raise TimeoutError(f"vehicle did not reach {takeoff_alt} m")


def request_messages(master: mavutil.mavfile) -> None:
    # ArduPilot already sends CAMERA_FEEDBACK for each image, and streaming it at an interval would
    # repeat the last image, or an empty one at 0,0 before the first
    for message_id, rate_hz in (
        (mavlink.MAVLINK_MSG_ID_GLOBAL_POSITION_INT, 5),
        (mavlink.MAVLINK_MSG_ID_MISSION_CURRENT, 2),
        (mavlink.MAVLINK_MSG_ID_CAMERA_IMAGE_CAPTURED, 50),
    ):
        command_long(master, mavlink.MAV_CMD_SET_MESSAGE_INTERVAL, message_id, 1e6 / rate_hz)


@dataclass
class Recording:
    track: list[dict[str, Any]] = field(default_factory=list)
    images: list[dict[str, Any]] = field(default_factory=list)
    statustext: list[str] = field(default_factory=list)
    current_seq: int = 0
    started: bool = False
    complete: bool = False

    def handle(self, msg: Any, elapsed: float) -> None:
        kind = msg.get_type()
        if kind == "GLOBAL_POSITION_INT":
            self.track.append(
                {
                    "time": round(elapsed, 2),
                    "lat": msg.lat / 1e7,
                    "lon": msg.lon / 1e7,
                    "alt": msg.relative_alt / 1000,
                    "heading": None if msg.hdg == UINT16_MAX else msg.hdg / 100,
                    "seq": self.current_seq,
                }
            )
        elif kind == "MISSION_CURRENT":
            self.current_seq = msg.seq
            # Recording starts before the mission, while a previous mission may still report complete
            state = getattr(msg, "mission_state", 0)
            self.started = self.started or state == MISSION_STATE_ACTIVE
            self.complete = self.started and state == MISSION_STATE_COMPLETE
        elif kind == "CAMERA_FEEDBACK":
            self._add_image(elapsed, msg.img_idx, msg.lat, msg.lng, msg.alt_rel)
        elif kind == "CAMERA_IMAGE_CAPTURED":
            self._add_image(elapsed, msg.image_index, msg.lat, msg.lon, msg.relative_alt / 1000)
        elif kind == "STATUSTEXT":
            self.statustext.append(msg.text)

    def _add_image(self, elapsed: float, index: int, lat: int, lon: int, alt: float) -> None:
        # PX4 can report an image more than once
        if any(image["index"] == index for image in self.images):
            return
        self.images.append(
            {
                "time": round(elapsed, 2),
                "index": index,
                "lat": lat / 1e7,
                "lon": lon / 1e7,
                "alt": alt,
                "seq": self.current_seq,
            }
        )


def start_recording(master: mavutil.mavfile) -> Recording:
    """Record the vehicle's messages as pymavlink receives them, including those read by other waits."""
    recording = Recording()
    start = time.monotonic()

    def hook(_master: mavutil.mavfile, msg: Any) -> None:
        if msg.get_type() not in RECORDED_MESSAGES or msg.get_srcSystem() != master.target_system:
            return
        previous_seq = recording.current_seq
        recording.handle(msg, time.monotonic() - start)
        if recording.current_seq != previous_seq:
            print(f"mission item {recording.current_seq}", flush=True)
        if msg.get_type() == "STATUSTEXT":
            print(f"vehicle: {msg.text}", flush=True)

    master.message_hooks.append(hook)
    return recording


def wait_for_completion(master: mavutil.mavfile, recording: Recording, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while not recording.complete and time.monotonic() < deadline:
        master.recv_match(blocking=True, timeout=1)


def _local(home: tuple[float, float, float], lat: float, lon: float) -> tuple[float, float]:
    east = math.radians(lon - home[1]) * EARTH_RADIUS * math.cos(math.radians(home[0]))
    north = math.radians(lat - home[0]) * EARTH_RADIUS
    return east, north


def planned_path(mission: list[PlanItem]) -> list[tuple[float, float]]:
    """Latitude and longitude of each navigation item with a position."""
    return [
        (item.params[4], item.params[5])
        for item in mission
        if item.command < mavlink.MAV_CMD_NAV_LAST
        and item.frame != mavlink.MAV_FRAME_MISSION
        and not math.isnan(item.params[4])
        and not math.isnan(item.params[5])
        and (item.params[4], item.params[5]) != (0, 0)
    ]


def write_svg(
    path: Path,
    title: str,
    home: tuple[float, float, float],
    mission: list[PlanItem],
    recording: Recording,
    size: int = 800,
    margin: int = 30,
) -> None:
    planned = [_local(home, lat, lon) for lat, lon in planned_path(mission)]
    flown = [_local(home, point["lat"], point["lon"]) for point in recording.track]
    images = [_local(home, image["lat"], image["lon"]) for image in recording.images]
    points = planned + flown + images or [(0.0, 0.0)]
    min_east, max_east = min(p[0] for p in points), max(p[0] for p in points)
    min_north, max_north = min(p[1] for p in points), max(p[1] for p in points)
    scale = (size - 2 * margin) / max(max_east - min_east, max_north - min_north, 1.0)

    def svg_points(local_points: list[tuple[float, float]]) -> str:
        return " ".join(
            f"{margin + (east - min_east) * scale:.1f},{size - margin - (north - min_north) * scale:.1f}"
            for east, north in local_points
        )

    elements = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{size}" height="{size}" '
        f'viewBox="0 0 {size} {size}">',
        '<rect width="100%" height="100%" fill="white"/>',
        f'<text x="{margin}" y="20" font-family="sans-serif" font-size="14">{escape(title)}</text>',
        f'<polyline points="{svg_points(planned)}" fill="none" stroke="#1f77b4" '
        'stroke-width="2" stroke-dasharray="6 4"/>',
        f'<polyline points="{svg_points(flown)}" fill="none" stroke="#555" stroke-width="1"/>',
    ]
    for image in svg_points(images).split():
        x, y = image.split(",")
        elements.append(f'<circle cx="{x}" cy="{y}" r="3" fill="#d62728"/>')
    elements.append("</svg>")
    path.write_text("\n".join(elements) + "\n", encoding="utf-8")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("plan", type=Path, help="QGC .plan file")
    parser.add_argument(
        "--connect",
        required=True,
        help="pymavlink connection, e.g. tcp:127.0.0.1:5762 or udpin:0.0.0.0:14540",
    )
    parser.add_argument("--output", type=Path, default=Path("flight.json"), help="JSON output")
    parser.add_argument("--svg", type=Path, help="Also plot the planned and flown paths")
    parser.add_argument(
        "--takeoff-alt",
        type=float,
        default=10.0,
        help="ArduPilot takeoff altitude before switching to AUTO (m)",
    )
    parser.add_argument(
        "--timeout", type=float, default=1800, help="Seconds to wait for the mission to complete"
    )
    parser.add_argument("--no-rtl", action="store_true", help="Do not return to launch afterwards")
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    try:
        home, items = load_plan(args.plan)
    except (OSError, ValueError, KeyError, TypeError) as error:
        parser.error(f"{args.plan}: {error}")

    # CAMERA_IMAGE_CAPTURED and the MISSION_CURRENT mission state need MAVLink 2
    os.environ["MAVLINK20"] = "1"
    mavutil.set_dialect("ardupilotmega")
    master = mavutil.mavlink_connection(args.connect, source_system=255, source_component=191)
    if not isinstance(master, mavutil.mavfile):
        parser.error(f"--connect {args.connect} is not a MAVLink connection")
    try:
        autopilot = wait_autopilot(master)
        name = AUTOPILOT_NAMES[autopilot]
        mission = mission_items(autopilot, home, items)
        upload(master, mission)
        print(f"uploaded {len(mission)} items to {name}", flush=True)
        request_messages(master)
        recording = start_recording(master)
        start_mission(master, autopilot, args.takeoff_alt, timeout=120)
        wait_for_completion(master, recording, args.timeout)
        if not args.no_rtl:
            command_long(master, mavlink.MAV_CMD_NAV_RETURN_TO_LAUNCH)
    except (RuntimeError, TimeoutError, ValueError) as error:
        print(f"Error: {error}", file=sys.stderr)
        return 1
    finally:
        master.close()

    args.output.write_text(
        json.dumps(
            {
                "autopilot": name,
                "complete": recording.complete,
                "mission": [
                    {
                        "seq": seq,
                        "command": item.command,
                        "frame": item.frame,
                        "params": [None if math.isnan(value) else value for value in item.params],
                    }
                    for seq, item in enumerate(mission)
                ],
                "track": recording.track,
                "images": recording.images,
                "statustext": recording.statustext,
            },
            indent=1,
        )
        + "\n",
        encoding="utf-8",
    )
    if args.svg:
        title = f"{args.plan.name} on {name}: {len(recording.images)} images"
        write_svg(args.svg, title, home, mission, recording)
    state = "complete" if recording.complete else "incomplete after --timeout"
    print(f"mission {state}: {len(recording.track)} positions, {len(recording.images)} images")
    return 0 if recording.complete else 1


if __name__ == "__main__":
    sys.exit(main())
