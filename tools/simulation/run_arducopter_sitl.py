#!/usr/bin/env python3
"""
ArduCopter SITL Test Environment.

Runs ArduCopter in Docker for testing QGC without hardware.

Usage:
    python tools/simulation/run_arducopter_sitl.py              # Start SITL
    python tools/simulation/run_arducopter_sitl.py --with-latency  # With network latency
    python tools/simulation/run_arducopter_sitl.py --stop       # Stop container
    python tools/simulation/run_arducopter_sitl.py --logs       # View logs
"""

from __future__ import annotations

import argparse
import shutil
import subprocess
import sys
import time
from dataclasses import dataclass, field


@dataclass
class SITLConfig:
    """Configuration for ArduCopter SITL."""

    copter_version: str = "Copter-4.5.6"
    image_name: str = "ardupilot-sitl-4.5.6"
    container_name: str = "arducopter-sitl"
    port: int = 5760
    with_latency: bool = False
    latency_ms: int = 50  # One-way latency for 100ms RTT
    home_location: str = "42.3898,-71.1476,14.0,270.0"
    speedup: int = 1
    docker_repo: str = "https://github.com/radarku/ardupilot-sitl-docker.git"


def has_docker() -> bool:
    """Check if Docker is available."""
    return shutil.which("docker") is not None


def image_exists(image_name: str) -> bool:
    """Check if Docker image exists."""
    result = subprocess.run(
        ["docker", "image", "inspect", image_name],
        capture_output=True,
    )
    return result.returncode == 0


def build_image(image_name: str, copter_version: str, docker_repo: str | None = None) -> None:
    """Build ArduCopter SITL Docker image."""
    if docker_repo is None:
        docker_repo = SITLConfig.docker_repo

    print(f"Building ArduCopter {copter_version} SITL image...")
    print("This may take 10-15 minutes on first run...")

    subprocess.run(
        [
            "docker", "build",
            "--tag", image_name,
            "--build-arg", f"COPTER_TAG={copter_version}",
            docker_repo,
        ],
        check=True,
    )


def stop_container(container_name: str) -> None:
    """Stop and remove container."""
    subprocess.run(
        ["docker", "rm", "-f", container_name],
        capture_output=True,
    )


def container_is_running(container_name: str) -> bool:
    """Check if container is running."""
    result = subprocess.run(
        [
            "docker", "ps",
            "--filter", f"name={container_name}",
            "--format", "{{.Status}}",
        ],
        capture_output=True,
        text=True,
    )
    return result.returncode == 0 and "Up" in result.stdout


def get_container_logs(container_name: str, lines: int = 20) -> str:
    """Get container logs."""
    result = subprocess.run(
        ["docker", "logs", "--tail", str(lines), container_name],
        capture_output=True,
        text=True,
    )
    return result.stdout + result.stderr


def get_arducopter_args(config: SITLConfig) -> list[str]:
    """Get ArduCopter command-line arguments."""
    return [
        "-S",
        "--model", "+",
        "--speedup", str(config.speedup),
        "--defaults", "/ardupilot/Tools/autotest/default_params/copter.parm",
        "--home", config.home_location,
        "--serial0", f"tcp:0:{config.port}:wait",
    ]


def get_run_command(config: SITLConfig) -> list[str]:
    """Build Docker run command."""
    cmd = [
        "docker", "run", "-d",
        "--name", config.container_name,
        "-p", f"{config.port}:{config.port}",
    ]

    arducopter_path = "/ardupilot/build/sitl/bin/arducopter"
    arducopter_args = get_arducopter_args(config)
    arducopter_cmd = f"{arducopter_path} {' '.join(arducopter_args)}"

    if config.with_latency:
        # Add NET_ADMIN capability for tc (traffic control)
        cmd.extend(["--cap-add=NET_ADMIN"])
        # Run shell with tc setup then exec arducopter
        tc_cmd = f"tc qdisc add dev eth0 root netem delay {config.latency_ms}ms 10ms 2>/dev/null || true"
        cmd.extend([
            "--entrypoint", "/bin/bash",
            config.image_name,
            "-c", f"{tc_cmd}; exec {arducopter_cmd}",
        ])
    else:
        cmd.extend([
            "--entrypoint", arducopter_path,
            config.image_name,
        ])
        cmd.extend(arducopter_args)

    return cmd


def run_sitl(config: SITLConfig) -> int:
    """Run ArduCopter SITL container."""
    if not has_docker():
        print("Error: Docker is not installed or not in PATH", file=sys.stderr)
        return 1

    # Build image if needed
    if not image_exists(config.image_name):
        try:
            build_image(config.image_name, config.copter_version, config.docker_repo)
        except subprocess.CalledProcessError:
            print("Error: Failed to build Docker image", file=sys.stderr)
            return 1

    # Stop existing container
    stop_container(config.container_name)

    # Start container
    if config.with_latency:
        print()
        print("Starting SITL with simulated latency (100ms round-trip)...")
        print("This simulates Herelink-like network conditions.")
        print()
    else:
        print()
        print("Starting SITL (no latency simulation)...")
        print("Use --with-latency to simulate Herelink network conditions.")
        print()

    cmd = get_run_command(config)
    result = subprocess.run(cmd, capture_output=True)

    if result.returncode != 0:
        print("Error: Failed to start container", file=sys.stderr)
        print(result.stderr.decode(), file=sys.stderr)
        return 1

    # Wait for startup
    time.sleep(3)

    # Check if running
    if container_is_running(config.container_name):
        print()
        print("=" * 44)
        print(f"ArduCopter {config.copter_version} SITL is running!")
        print("=" * 44)
        print()
        print(f"Connect QGC to: tcp://localhost:{config.port}")
        print()
        print("Test procedure:")
        print("  1. Open QGC and connect to localhost:5760")
        print("  2. Go to Plan view")
        print("  3. Create a mission with waypoints (no geofence)")
        print("  4. Click Upload")
        print("  5. Verify no 'Geofence Transfer failed' error appears")
        print()
        print("Commands:")
        print(f"  View logs:  docker logs -f {config.container_name}")
        print(f"  Stop SITL:  docker stop {config.container_name}")
        print(f"  Remove:     docker rm {config.container_name}")
        print()
        return 0
    else:
        print("Error: SITL container failed to start", file=sys.stderr)
        print("Logs:")
        print(get_container_logs(config.container_name))
        return 1


def parse_args(args: list[str] | None = None) -> argparse.Namespace:
    """Parse command-line arguments."""
    parser = argparse.ArgumentParser(
        description="Run ArduCopter SITL in Docker for QGC testing.",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="""
Examples:
  %(prog)s                    # Start SITL
  %(prog)s --with-latency     # Simulate 100ms RTT latency
  %(prog)s --port 5761        # Use custom port
  %(prog)s --stop             # Stop running container
  %(prog)s --logs             # View container logs
""",
    )

    parser.add_argument(
        "--with-latency",
        action="store_true",
        help="Simulate 100ms round-trip latency (Herelink-like conditions)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=5760,
        help="TCP port for MAVLink connection (default: 5760)",
    )
    parser.add_argument(
        "--stop",
        action="store_true",
        help="Stop and remove the SITL container",
    )
    parser.add_argument(
        "--logs",
        action="store_true",
        help="Show container logs",
    )
    parser.add_argument(
        "--follow",
        "-f",
        action="store_true",
        help="Follow logs (use with --logs)",
    )

    return parser.parse_args(args)


def main() -> int:
    """Main entry point."""
    args = parse_args()

    config = SITLConfig(
        port=args.port,
        with_latency=args.with_latency,
    )

    if args.stop:
        print(f"Stopping container: {config.container_name}")
        stop_container(config.container_name)
        print("Done")
        return 0

    if args.logs:
        if args.follow:
            subprocess.run(["docker", "logs", "-f", config.container_name])
        else:
            print(get_container_logs(config.container_name, lines=50))
        return 0

    return run_sitl(config)


if __name__ == "__main__":
    sys.exit(main())
