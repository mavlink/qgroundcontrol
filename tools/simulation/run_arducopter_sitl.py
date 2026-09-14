#!/usr/bin/env python3
"""Start a locally owned ArduCopter Docker container and wait for its TCP listener."""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
import time

IMAGE = "ardupilot-sitl-4.5.6"
VERSION = "Copter-4.5.6"
OWNER_LABEL = "org.qgroundcontrol.sitl"


def inspect(container: str) -> dict:
    result = subprocess.run(
        ["docker", "container", "inspect", container],
        check=True,
        capture_output=True,
        text=True,
        timeout=15,
    )
    return json.loads(result.stdout)[0]


def wait_ready(container: str, timeout: float) -> None:
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = inspect(container)["State"]
        if not state["Running"]:
            raise RuntimeError(f"SITL stopped: {state.get('Error') or state.get('ExitCode')}")
        # Inspect the guest TCP table instead of consuming SITL's first client connection.
        result = subprocess.run(
            ["docker", "exec", container, "cat", "/proc/net/tcp", "/proc/net/tcp6"],
            check=True,
            capture_output=True,
            text=True,
            timeout=15,
        )
        for line in result.stdout.splitlines():
            fields = line.split()
            if len(fields) >= 4 and fields[1].endswith(":1680") and fields[3] == "0A":
                return
        time.sleep(min(0.5, max(0, deadline - time.monotonic())))
    raise TimeoutError(f"SITL TCP port 5760 did not become ready within {timeout}s")


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--with-latency", action="store_true")
    parser.add_argument("--name", default="arducopter-sitl")
    parser.add_argument("--timeout", type=float, default=60)
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    container = ""
    try:
        existing = subprocess.run(
            ["docker", "container", "ls", "-a", "--format", "{{.Names}}"],
            check=True,
            capture_output=True,
            text=True,
        )
        if args.name in existing.stdout.splitlines():
            previous = inspect(args.name)
            if (previous["Config"].get("Labels") or {}).get(OWNER_LABEL) != "true":
                raise ValueError(
                    f"Container {args.name} is not owned by this script; remove or rename it explicitly"
                )
            subprocess.run(["docker", "rm", "-f", previous["Id"]], check=True)
        if subprocess.run(
            ["docker", "image", "inspect", IMAGE],
            check=False,
            stdout=subprocess.DEVNULL,
            stderr=subprocess.DEVNULL,
        ).returncode:
            subprocess.run(
                [
                    "docker",
                    "build",
                    "--tag",
                    IMAGE,
                    "--build-arg",
                    f"COPTER_TAG={VERSION}",
                    "https://github.com/radarku/ardupilot-sitl-docker.git",
                ],
                check=True,
            )
        command = [
            "/ardupilot/build/sitl/bin/arducopter",
            "-S",
            "--model",
            "+",
            "--speedup",
            "1",
            "--defaults",
            "/ardupilot/Tools/autotest/default_params/copter.parm",
            "--home",
            "42.3898,-71.1476,14.0,270.0",
            "--serial0",
            "tcp:0:5760:wait",
        ]
        docker = [
            "docker",
            "run",
            "-d",
            "--name",
            args.name,
            "--label",
            f"{OWNER_LABEL}=true",
            "-p",
            "127.0.0.1:5760:5760",
        ]
        if args.with_latency:
            docker += [
                "--cap-add=NET_ADMIN",
                "--entrypoint",
                "/bin/bash",
                IMAGE,
                "-ec",
                "tc qdisc add dev eth0 root netem delay 50ms 10ms; exec " + shlex.join(command),
            ]
        else:
            docker += ["--entrypoint", command[0], IMAGE, *command[1:]]
        result = subprocess.run(docker, check=True, capture_output=True, text=True)
        container = result.stdout.strip()
        if not container:
            raise RuntimeError("Docker did not return a container ID")
        wait_ready(container, args.timeout)
        print(
            f"ArduCopter {VERSION} is ready. Connect QGC to tcp://localhost:5760\n"
            f"Logs: docker logs -f {args.name}\nStop: docker stop {args.name}"
        )
        return 0
    except (
        OSError,
        ValueError,
        RuntimeError,
        subprocess.SubprocessError,
        KeyboardInterrupt,
    ) as error:
        print(f"SITL startup failed: {error}", file=sys.stderr)
        if container:
            for command in (
                ["docker", "logs", "--tail", "20", container],
                ["docker", "rm", "-f", container],
            ):
                try:
                    subprocess.run(command, check=False, timeout=15)
                except (OSError, subprocess.SubprocessError) as cleanup_error:
                    print(f"Container cleanup failed: {cleanup_error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
