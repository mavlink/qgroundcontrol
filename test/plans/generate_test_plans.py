#!/usr/bin/env python3
"""Generate manual-test .plan files around each MockLink home location.

The plans are not meant to be flyable. Each one mixes altitude frames and mission item
types so the 2D/3D mission display can be checked against terrain, for each PX4 and
ArduPilot vehicle type MockLink can simulate.
"""

from __future__ import annotations

import argparse
import json
import math
from dataclasses import dataclass
from pathlib import Path
from typing import Any

PLANS_DIR = Path(__file__).resolve().parent

# MAVLink values
MAV_AUTOPILOT_ARDUPILOTMEGA = 3
MAV_AUTOPILOT_PX4 = 12
MAV_TYPE_FIXED_WING = 1
MAV_TYPE_QUADROTOR = 2
MAV_TYPE_GROUND_ROVER = 10
MAV_TYPE_SUBMARINE = 12
MAV_FRAME_GLOBAL = 0
MAV_FRAME_MISSION = 2
MAV_FRAME_GLOBAL_RELATIVE_ALT = 3
MAV_FRAME_GLOBAL_TERRAIN_ALT = 10
MAV_CMD_NAV_WAYPOINT = 16
MAV_CMD_NAV_LOITER_TURNS = 18
MAV_CMD_NAV_LOITER_TIME = 19
MAV_CMD_NAV_RETURN_TO_LAUNCH = 20
MAV_CMD_NAV_LAND = 21
MAV_CMD_NAV_TAKEOFF = 22
MAV_CMD_NAV_LOITER_TO_ALT = 31
MAV_CMD_NAV_SPLINE_WAYPOINT = 82
MAV_CMD_DO_CHANGE_SPEED = 178
MAV_CMD_DO_SET_ROI_LOCATION = 195
MAV_CMD_DO_SET_ROI = 201

# QGroundControlQmlGlobal::AltitudeFrame
ALT_MIXED = 0
ALT_RELATIVE = 1
ALT_ABSOLUTE = 2
ALT_CALC_ABOVE_TERRAIN = 3
ALT_TERRAIN = 4

# Offsets (north m, east m) from home. ~3 km legs in every direction so each plan crosses
# whatever terrain variation surrounds the home location.
A = (1500.0, 1500.0)
B = (0.0, 3000.0)
C = (-3000.0, 3000.0)
D = (-3000.0, 0.0)
E = (-3000.0, -3000.0)
F = (0.0, -3000.0)
G = (2500.0, -2000.0)
NEAR_HOME = (300.0, 0.0)


@dataclass(frozen=True)
class Home:
    name: str
    latitude: float
    longitude: float
    altitude: float


# Must match MockConfiguration::homeCoordinate()
HOMES = [
    Home("PX4SITL", 47.397, 8.5455, 488.056),
    Home("ArduPilotSITL", -35.363261, 149.165230, 584.0),
    Home("TerrainTest", 47.6305111, -122.0863806, 9.26),
]


@dataclass(frozen=True)
class Vehicle:
    name: str
    firmware: int
    vehicle: int


PX4_MULTIROTOR = Vehicle("PX4-MultiRotor", MAV_AUTOPILOT_PX4, MAV_TYPE_QUADROTOR)
ARDUCOPTER = Vehicle("ArduCopter", MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_QUADROTOR)
ARDUPLANE = Vehicle("ArduPlane", MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_FIXED_WING)
ARDUROVER = Vehicle("ArduRover", MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_GROUND_ROVER)
ARDUSUB = Vehicle("ArduSub", MAV_AUTOPILOT_ARDUPILOTMEGA, MAV_TYPE_SUBMARINE)
VEHICLES = [PX4_MULTIROTOR, ARDUCOPTER, ARDUPLANE, ARDUROVER, ARDUSUB]


class PlanBuilder:
    def __init__(self, home: Home) -> None:
        self._home = home
        self.items: list[dict[str, Any]] = []

    def _coordinate(self, offset: tuple[float, float]) -> tuple[float, float]:
        north, east = offset
        latitude = self._home.latitude + north / 111320.0
        longitude = self._home.longitude + east / (
            111320.0 * math.cos(math.radians(self._home.latitude))
        )
        return round(latitude, 7), round(longitude, 7)

    def nav(
        self,
        command: int,
        offset: tuple[float, float],
        alt_frame: int,
        altitude: float,
        params: tuple[float | None, ...] = (0, 0, 0, None),
    ) -> None:
        """Adds a positional item. Absolute altitudes are given relative to home."""
        if alt_frame == ALT_ABSOLUTE:
            altitude += self._home.altitude
        frame = {
            ALT_RELATIVE: MAV_FRAME_GLOBAL_RELATIVE_ALT,
            ALT_ABSOLUTE: MAV_FRAME_GLOBAL,
            ALT_CALC_ABOVE_TERRAIN: MAV_FRAME_GLOBAL,
            ALT_TERRAIN: MAV_FRAME_GLOBAL_TERRAIN_ALT,
        }[alt_frame]
        # QGC fills in the AMSL value for calculated-above-terrain items once terrain data arrives
        param7 = None if alt_frame == ALT_CALC_ABOVE_TERRAIN else altitude
        latitude, longitude = self._coordinate(offset)
        self.items.append(
            {
                "AMSLAltAboveTerrain": None,
                "Altitude": altitude,
                "AltitudeMode": alt_frame,
                "autoContinue": True,
                "command": command,
                "doJumpId": len(self.items) + 1,
                "frame": frame,
                "params": [*params, latitude, longitude, param7],
                "type": "SimpleItem",
            }
        )

    def command(self, command: int, params: tuple[float, ...]) -> None:
        self.items.append(
            {
                "autoContinue": True,
                "command": command,
                "doJumpId": len(self.items) + 1,
                "frame": MAV_FRAME_MISSION,
                "params": [*params, 0, 0, 0],
                "type": "SimpleItem",
            }
        )

    def change_speed(self, speed: float) -> None:
        self.command(MAV_CMD_DO_CHANGE_SPEED, (1, speed, -1, 0))


def multirotor_items(plan: PlanBuilder, vehicle: Vehicle) -> None:
    apm = vehicle.firmware == MAV_AUTOPILOT_ARDUPILOTMEGA
    plan.nav(MAV_CMD_NAV_TAKEOFF, NEAR_HOME, ALT_RELATIVE, 30, (0, 0, 0, None))
    plan.nav(MAV_CMD_NAV_WAYPOINT, A, ALT_RELATIVE, 60)
    plan.change_speed(10)
    plan.nav(MAV_CMD_NAV_WAYPOINT, B, ALT_ABSOLUTE, 150)
    if apm:
        plan.nav(MAV_CMD_DO_SET_ROI, C, ALT_RELATIVE, 0, (0, 0, 0, 0))
    else:
        plan.nav(MAV_CMD_DO_SET_ROI_LOCATION, C, ALT_RELATIVE, 0, (0, 0, 0, 0))
    plan.nav(MAV_CMD_NAV_WAYPOINT, C, ALT_CALC_ABOVE_TERRAIN, 80)
    plan.nav(MAV_CMD_NAV_LOITER_TIME, D, ALT_RELATIVE, 100, (30, 0, 50, 0))
    if apm:
        plan.nav(MAV_CMD_NAV_WAYPOINT, E, ALT_TERRAIN, 50)
        plan.nav(MAV_CMD_NAV_SPLINE_WAYPOINT, F, ALT_RELATIVE, 70, (0, 0, 0, 0))
    else:
        plan.nav(MAV_CMD_NAV_WAYPOINT, E, ALT_CALC_ABOVE_TERRAIN, 40)
        plan.nav(MAV_CMD_NAV_WAYPOINT, F, ALT_RELATIVE, 70)
    # Low enough that legs are likely to run into rising terrain
    plan.nav(MAV_CMD_NAV_WAYPOINT, G, ALT_RELATIVE, 15)
    plan.nav(MAV_CMD_NAV_LAND, NEAR_HOME, ALT_RELATIVE, 0, (0, 0, 0, None))


def fixed_wing_items(plan: PlanBuilder) -> None:
    plan.nav(MAV_CMD_NAV_TAKEOFF, NEAR_HOME, ALT_RELATIVE, 50, (15, 0, 0, None))
    plan.nav(MAV_CMD_NAV_WAYPOINT, A, ALT_RELATIVE, 120)
    plan.nav(MAV_CMD_NAV_WAYPOINT, B, ALT_ABSOLUTE, 200)
    plan.nav(MAV_CMD_NAV_LOITER_TURNS, C, ALT_CALC_ABOVE_TERRAIN, 150, (2, 0, 250, 1))
    plan.nav(MAV_CMD_NAV_WAYPOINT, D, ALT_TERRAIN, 100)
    plan.nav(MAV_CMD_NAV_LOITER_TO_ALT, E, ALT_RELATIVE, 80, (0, -200, 0, 0))
    plan.change_speed(18)
    plan.nav(MAV_CMD_NAV_WAYPOINT, F, ALT_RELATIVE, 60)
    plan.nav(MAV_CMD_NAV_WAYPOINT, G, ALT_RELATIVE, 40)
    plan.nav(MAV_CMD_NAV_LAND, (-300.0, 0.0), ALT_RELATIVE, 0, (0, 0, 0, None))


def rover_items(plan: PlanBuilder) -> None:
    plan.nav(MAV_CMD_NAV_WAYPOINT, A, ALT_RELATIVE, 0)
    plan.nav(MAV_CMD_NAV_WAYPOINT, B, ALT_ABSOLUTE, 0)
    plan.change_speed(5)
    plan.nav(MAV_CMD_NAV_LOITER_TIME, C, ALT_RELATIVE, 0, (20, 0, 10, 0))
    plan.nav(MAV_CMD_NAV_WAYPOINT, D, ALT_RELATIVE, 0)
    plan.nav(MAV_CMD_NAV_WAYPOINT, E, ALT_RELATIVE, 0)
    plan.nav(MAV_CMD_NAV_WAYPOINT, F, ALT_RELATIVE, 0)
    plan.command(MAV_CMD_NAV_RETURN_TO_LAUNCH, (0, 0, 0, 0))


def sub_items(plan: PlanBuilder) -> None:
    plan.nav(MAV_CMD_NAV_WAYPOINT, A, ALT_RELATIVE, -5)
    plan.nav(MAV_CMD_NAV_WAYPOINT, B, ALT_ABSOLUTE, -15)
    plan.nav(MAV_CMD_NAV_WAYPOINT, C, ALT_RELATIVE, -10)
    plan.change_speed(1)
    plan.nav(MAV_CMD_NAV_LOITER_TIME, D, ALT_RELATIVE, -20, (20, 0, 0, 0))
    plan.nav(MAV_CMD_NAV_WAYPOINT, E, ALT_ABSOLUTE, -5)
    plan.nav(MAV_CMD_NAV_WAYPOINT, NEAR_HOME, ALT_RELATIVE, 0)


def build_plan(home: Home, vehicle: Vehicle) -> dict[str, Any]:
    plan = PlanBuilder(home)
    if vehicle.vehicle == MAV_TYPE_QUADROTOR:
        multirotor_items(plan, vehicle)
    elif vehicle.vehicle == MAV_TYPE_FIXED_WING:
        fixed_wing_items(plan)
    elif vehicle.vehicle == MAV_TYPE_GROUND_ROVER:
        rover_items(plan)
    else:
        sub_items(plan)

    return {
        "fileType": "Plan",
        "geoFence": {"circles": [], "polygons": [], "version": 2},
        "groundStation": "QGroundControl",
        "mission": {
            "cruiseSpeed": 15,
            "firmwareType": vehicle.firmware,
            "globalPlanAltitudeMode": ALT_MIXED,
            "hoverSpeed": 5,
            "items": plan.items,
            "plannedHomePosition": [home.latitude, home.longitude, home.altitude],
            "vehicleType": vehicle.vehicle,
            "version": 2,
        },
        "rallyPoints": {"points": [], "version": 2},
        "version": 1,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path, default=PLANS_DIR, help="Output directory")
    args = parser.parse_args()

    args.output.mkdir(parents=True, exist_ok=True)
    for home in HOMES:
        for vehicle in VEHICLES:
            path = args.output / f"{home.name}-{vehicle.name}.plan"
            path.write_text(json.dumps(build_plan(home, vehicle), indent=4) + "\n")
            print(path)


if __name__ == "__main__":
    main()
