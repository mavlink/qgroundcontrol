#!/usr/bin/env python3
"""
Mock MAVLink Vehicle for QGroundControl Testing

This script simulates a MAVLink vehicle that QGC can connect to for testing
without actual hardware. Useful for UI development and testing.

Usage:
    ./mock_vehicle.py                     # UDP on localhost:14550
    ./mock_vehicle.py --port 14551        # Custom port
    ./mock_vehicle.py --tcp               # TCP connection
    ./mock_vehicle.py --sitl              # SITL-compatible mode
    ./mock_vehicle.py --system-id 2       # Custom system ID
    ./mock_vehicle.py --lat 37.7749 --lon -122.4194  # San Francisco

Environment variables:
    MOCK_VEHICLE_LAT    Initial latitude (default: 47.3977, Zurich)
    MOCK_VEHICLE_LON    Initial longitude (default: 8.5456, Zurich)
    MOCK_VEHICLE_ALT    Initial altitude in meters (default: 100)

Requirements:
    pip install pymavlink

Based on MAVLink protocol v2.0
"""

import argparse
import math
import os
import sys
import time
from dataclasses import dataclass, field

try:
    from pymavlink import mavutil
    from pymavlink.dialects.v20 import common as mavlink
except ImportError:
    print("Error: pymavlink not installed")
    print("Install with: pip install pymavlink")
    sys.exit(1)

# Default location (Zurich, Switzerland)
DEFAULT_LAT = 47.3977
DEFAULT_LON = 8.5456
DEFAULT_ALT = 100.0


def _get_default_lat() -> float:
    """Get default latitude from env var or use Zurich."""
    return float(os.environ.get("MOCK_VEHICLE_LAT", DEFAULT_LAT))


def _get_default_lon() -> float:
    """Get default longitude from env var or use Zurich."""
    return float(os.environ.get("MOCK_VEHICLE_LON", DEFAULT_LON))


def _get_default_alt() -> float:
    """Get default altitude from env var."""
    return float(os.environ.get("MOCK_VEHICLE_ALT", DEFAULT_ALT))


@dataclass
class VehicleState:
    """Current vehicle state."""

    lat: float = field(default_factory=_get_default_lat)
    lon: float = field(default_factory=_get_default_lon)
    alt: float = field(default_factory=_get_default_alt)
    heading: float = 0.0
    roll: float = 0.0
    pitch: float = 0.0
    yaw: float = 0.0
    groundspeed: float = 0.0
    airspeed: float = 0.0
    climb_rate: float = 0.0
    battery_voltage: float = 12.6
    battery_remaining: int = 100
    armed: bool = False
    mode: str = "STABILIZE"
    gps_fix: int = 3  # 3D fix
    satellites: int = 12


class MockVehicle:
    """Simulates a MAVLink vehicle."""

    def __init__(self, system_id=1, component_id=1):
        self.system_id = system_id
        self.component_id = component_id
        self.state = VehicleState()
        self.running = False
        self.connection = None
        self.start_time = time.time()

        # Mode mapping
        self.modes = {
            "STABILIZE": 0,
            "ACRO": 1,
            "ALT_HOLD": 2,
            "AUTO": 3,
            "GUIDED": 4,
            "LOITER": 5,
            "RTL": 6,
            "LAND": 9,
        }

    def connect_udp(self, host="127.0.0.1", port=14550):
        """Connect via UDP."""
        self.connection = mavutil.mavlink_connection(
            f"udpout:{host}:{port}",
            source_system=self.system_id,
            source_component=self.component_id,
        )
        print(f"UDP connection to {host}:{port}")

    def connect_tcp(self, host="127.0.0.1", port=5760):
        """Connect via TCP (SITL-style)."""
        self.connection = mavutil.mavlink_connection(
            f"tcp:{host}:{port}",
            source_system=self.system_id,
            source_component=self.component_id,
        )
        print(f"TCP connection on {host}:{port}")

    def send_heartbeat(self):
        """Send heartbeat message."""
        base_mode = mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED
        if self.state.armed:
            base_mode |= mavlink.MAV_MODE_FLAG_SAFETY_ARMED

        custom_mode = self.modes.get(self.state.mode, 0)

        self.connection.mav.heartbeat_send(
            mavlink.MAV_TYPE_QUADROTOR,
            mavlink.MAV_AUTOPILOT_ARDUPILOTMEGA,
            base_mode,
            custom_mode,
            mavlink.MAV_STATE_ACTIVE,
        )

    def send_sys_status(self):
        """Send system status."""
        voltage = int(self.state.battery_voltage * 1000)
        self.connection.mav.sys_status_send(
            0xFFFFFFFF,  # sensors present
            0xFFFFFFFF,  # sensors enabled
            0xFFFFFFFF,  # sensors health
            500,  # load (50%)
            voltage,  # voltage_battery (mV)
            -1,  # current_battery
            self.state.battery_remaining,
            0,
            0,
            0,
            0,
            0,
            0,
        )

    def send_gps(self):
        """Send GPS position."""
        lat = int(self.state.lat * 1e7)
        lon = int(self.state.lon * 1e7)
        alt = int(self.state.alt * 1000)

        self.connection.mav.gps_raw_int_send(
            int((time.time() - self.start_time) * 1e6),
            self.state.gps_fix,
            lat,
            lon,
            alt,
            65535,  # eph
            65535,  # epv
            int(self.state.groundspeed * 100),
            int(self.state.heading * 100),
            self.state.satellites,
        )

    def send_global_position(self):
        """Send global position."""
        lat = int(self.state.lat * 1e7)
        lon = int(self.state.lon * 1e7)
        alt = int(self.state.alt * 1000)
        relative_alt = int(self.state.alt * 1000)

        self.connection.mav.global_position_int_send(
            int((time.time() - self.start_time) * 1000),
            lat,
            lon,
            alt,
            relative_alt,
            int(self.state.groundspeed * 100),  # vx
            0,  # vy
            int(self.state.climb_rate * 100),  # vz
            int(self.state.heading * 100),
        )

    def send_attitude(self):
        """Send attitude."""
        self.connection.mav.attitude_send(
            int((time.time() - self.start_time) * 1000),
            math.radians(self.state.roll),
            math.radians(self.state.pitch),
            math.radians(self.state.yaw),
            0,
            0,
            0,  # roll/pitch/yaw rates
        )

    def send_vfr_hud(self):
        """Send VFR HUD data."""
        self.connection.mav.vfr_hud_send(
            self.state.airspeed,
            self.state.groundspeed,
            int(self.state.heading),
            50,  # throttle
            self.state.alt,
            self.state.climb_rate,
        )

    def send_battery_status(self):
        """Send battery status."""
        voltages = [int(self.state.battery_voltage * 1000)] + [65535] * 9
        self.connection.mav.battery_status_send(
            0,  # id
            mavlink.MAV_BATTERY_FUNCTION_ALL,
            mavlink.MAV_BATTERY_TYPE_LIPO,
            25,  # temperature (0.1 C)
            voltages,
            -1,  # current
            -1,  # current consumed
            -1,  # energy consumed
            self.state.battery_remaining,
        )

    def send_home_position(self):
        """Send home position."""
        lat = int(self.state.lat * 1e7)
        lon = int(self.state.lon * 1e7)

        self.connection.mav.home_position_send(
            lat,
            lon,
            0,  # home position
            0,
            0,
            0,  # local position
            [1, 0, 0, 0],  # quaternion
            0,
            0,
            0,  # approach
        )

    def update_simulation(self, dt):
        """Update simulated vehicle state."""
        # Simulate slow position drift
        if self.state.armed:
            self.state.heading = (self.state.heading + 1) % 360
            self.state.yaw = self.state.heading

            # Simulate slight movement
            self.state.lat += 0.00001 * math.cos(math.radians(self.state.heading))
            self.state.lon += 0.00001 * math.sin(math.radians(self.state.heading))

        # Battery drain
        if self.state.armed:
            self.state.battery_remaining = max(0, self.state.battery_remaining - 0.01)
            self.state.battery_voltage = 10.5 + (self.state.battery_remaining / 100) * 2.1

    def handle_commands(self):
        """Handle incoming MAVLink commands."""
        msg = self.connection.recv_match(blocking=False)
        if msg is None:
            return

        msg_type = msg.get_type()

        if msg_type == "COMMAND_LONG":
            self.handle_command_long(msg)
        elif msg_type == "SET_MODE":
            self.handle_set_mode(msg)
        elif msg_type == "PARAM_REQUEST_LIST":
            self.send_parameters()

    def handle_command_long(self, msg):
        """Handle COMMAND_LONG message."""
        cmd = msg.command

        if cmd == mavlink.MAV_CMD_COMPONENT_ARM_DISARM:
            arm = msg.param1 == 1
            self.state.armed = arm
            print(f"Vehicle {'armed' if arm else 'disarmed'}")
            self.send_command_ack(cmd, mavlink.MAV_RESULT_ACCEPTED)

        elif cmd == mavlink.MAV_CMD_REQUEST_AUTOPILOT_CAPABILITIES:
            self.send_autopilot_version()
            self.send_command_ack(cmd, mavlink.MAV_RESULT_ACCEPTED)

        else:
            self.send_command_ack(cmd, mavlink.MAV_RESULT_ACCEPTED)

    def handle_set_mode(self, msg):
        """Handle SET_MODE message."""
        custom_mode = msg.custom_mode
        for name, mode_id in self.modes.items():
            if mode_id == custom_mode:
                self.state.mode = name
                print(f"Mode changed to {name}")
                break

    def send_command_ack(self, command, result):
        """Send command acknowledgment."""
        self.connection.mav.command_ack_send(command, result)

    def send_autopilot_version(self):
        """Send autopilot version info."""
        self.connection.mav.autopilot_version_send(
            mavlink.MAV_PROTOCOL_CAPABILITY_MISSION_FLOAT
            | mavlink.MAV_PROTOCOL_CAPABILITY_PARAM_FLOAT
            | mavlink.MAV_PROTOCOL_CAPABILITY_COMMAND_INT,
            0x04000000,  # flight_sw_version (v4.0.0)
            0,  # middleware_sw_version
            0,  # os_sw_version
            0,  # board_version
            bytes(8),  # flight_custom_version
            bytes(8),  # middleware_custom_version
            bytes(8),  # os_custom_version
            0,  # vendor_id
            0,  # product_id
            0,  # uid
        )

    def send_parameters(self):
        """Send parameter list."""
        params = [
            ("SYSID_THISMAV", self.system_id),
            ("BATT_CAPACITY", 5000),
            ("BATT_LOW_VOLT", 10.5),
            ("RTL_ALT", 30),
        ]

        for i, (name, value) in enumerate(params):
            self.connection.mav.param_value_send(
                name.encode("utf-8"),
                float(value),
                mavlink.MAV_PARAM_TYPE_REAL32,
                len(params),
                i,
            )

    def run(self, rate=10):
        """Main loop."""
        self.running = True
        dt = 1.0 / rate
        last_heartbeat = 0
        last_telemetry = 0

        print(f"Mock vehicle running (System ID: {self.system_id})")
        print("Press Ctrl+C to stop")

        try:
            while self.running:
                now = time.time()

                # Handle incoming commands
                self.handle_commands()

                # Send heartbeat at 1Hz
                if now - last_heartbeat >= 1.0:
                    self.send_heartbeat()
                    last_heartbeat = now

                # Send telemetry at specified rate
                if now - last_telemetry >= dt:
                    self.send_sys_status()
                    self.send_gps()
                    self.send_global_position()
                    self.send_attitude()
                    self.send_vfr_hud()
                    self.update_simulation(dt)
                    last_telemetry = now

                time.sleep(0.01)

        except KeyboardInterrupt:
            print("\nStopping...")
        finally:
            self.running = False


def main():
    parser = argparse.ArgumentParser(
        description="Mock MAVLink vehicle for QGroundControl testing",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )
    parser.add_argument("--host", default="127.0.0.1", help="Host address (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=14550, help="Port number (default: 14550)")
    parser.add_argument("--tcp", action="store_true", help="Use TCP instead of UDP")
    parser.add_argument("--system-id", type=int, default=1, help="MAVLink system ID (default: 1)")
    parser.add_argument("--rate", type=int, default=10, help="Telemetry rate in Hz (default: 10)")
    parser.add_argument("--sitl", action="store_true", help="SITL-compatible mode (TCP on 5760)")
    parser.add_argument(
        "--lat",
        type=float,
        default=None,
        help=f"Initial latitude (default: env MOCK_VEHICLE_LAT or {DEFAULT_LAT})",
    )
    parser.add_argument(
        "--lon",
        type=float,
        default=None,
        help=f"Initial longitude (default: env MOCK_VEHICLE_LON or {DEFAULT_LON})",
    )
    parser.add_argument(
        "--alt",
        type=float,
        default=None,
        help=f"Initial altitude in meters (default: env MOCK_VEHICLE_ALT or {DEFAULT_ALT})",
    )

    args = parser.parse_args()

    vehicle = MockVehicle(system_id=args.system_id)

    # Override location from CLI if provided
    if args.lat is not None:
        vehicle.state.lat = args.lat
    if args.lon is not None:
        vehicle.state.lon = args.lon
    if args.alt is not None:
        vehicle.state.alt = args.alt

    print(
        f"Initial position: {vehicle.state.lat:.4f}, {vehicle.state.lon:.4f} @ {vehicle.state.alt:.0f}m"
    )

    if args.sitl:
        vehicle.connect_tcp(args.host, 5760)
    elif args.tcp:
        vehicle.connect_tcp(args.host, args.port)
    else:
        vehicle.connect_udp(args.host, args.port)

    vehicle.run(rate=args.rate)


if __name__ == "__main__":
    main()
