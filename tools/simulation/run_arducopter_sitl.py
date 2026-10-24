#!/usr/bin/env python3
"""Start a locally owned ArduCopter Docker container, or a native SITL build, and wait for its TCP listener."""

from __future__ import annotations

import argparse
import json
import shlex
import subprocess
import sys
import tempfile
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.proc import wait_until

IMAGE = "ardupilot-sitl-4.5.6"
VERSION = "Copter-4.5.6"
OWNER_LABEL = "org.qgroundcontrol.sitl"
# Third-party Dockerfile repository: pin a reviewed full commit so builds cannot change silently.
BUILD_CONTEXT = (
    "https://github.com/radarku/ardupilot-sitl-docker.git#eff32c1f98152ac3d1dc09a1e475733b73ce569f"
)


def sitl_command(binary: str, defaults: str) -> list[str]:
    return [
        binary,
        "-S",
        "--model",
        "+",
        "--speedup",
        "1",
        "--defaults",
        defaults,
        "--home",
        "42.3898,-71.1476,14.0,270.0",
        "--serial0",
        "tcp:0:5760:wait",
    ]


def inspect(container: str) -> dict:
    result = subprocess.run(
        ["docker", "container", "inspect", container],
        check=True,
        capture_output=True,
        text=True,
        timeout=15,
    )
    return json.loads(result.stdout)[0]


def listening(container: str) -> bool:
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
    return any(
        len(fields) >= 4 and fields[1].endswith(":1680") and fields[3] == "0A"
        for fields in map(str.split, result.stdout.splitlines())
    )


def wait_ready(container: str, timeout: float) -> None:
    wait_until(
        lambda: listening(container),
        timeout=timeout,
        interval=0.5,
        description="SITL TCP port 5760",
    )


# SITL logs this once it is listening, so readiness never takes its client connection or depends on
# the port's state
NATIVE_READY_LINE = "SERIAL0 on TCP port 5760"


def _last_line(log: Path) -> str:
    lines = log.read_text(encoding="utf-8", errors="replace").strip().splitlines()
    return lines[-1] if lines else "no output"


def run_native(ardupilot: Path, timeout: float) -> None:
    """Run a SITL build in the foreground until Ctrl+C, since SITL exits with its parent process."""
    # SITL runs in a temporary directory, so relative paths would no longer resolve
    ardupilot = ardupilot.resolve()
    binary = ardupilot / "build" / "sitl" / "bin" / "arducopter"
    if not binary.is_file():
        raise ValueError(
            f"{binary} not found; build it in {ardupilot} with "
            "./waf configure --board sitl && ./waf copter"
        )
    workdir = Path(tempfile.mkdtemp(prefix="arducopter-sitl-"))
    log = workdir / "sitl.log"
    defaults = ardupilot / "Tools" / "autotest" / "default_params" / "copter.parm"
    with log.open("wb") as output:
        process = subprocess.Popen(
            sitl_command(str(binary), str(defaults)),
            cwd=workdir,
            stdin=subprocess.DEVNULL,
            stdout=output,
            stderr=subprocess.STDOUT,
        )

    def exited() -> RuntimeError:
        return RuntimeError(f"SITL exited with {process.returncode} ({_last_line(log)}); see {log}")

    def listening() -> bool:
        if process.poll() is not None:
            raise exited()
        return NATIVE_READY_LINE in log.read_text(encoding="utf-8", errors="replace")

    try:
        wait_until(listening, timeout=timeout, interval=0.5, description="SITL TCP port 5760")
        print(
            "ArduCopter SITL is ready. Connect QGC to tcp://localhost:5760\n"
            f"Logs: {log}\nStop: Ctrl+C",
            flush=True,
        )
        try:
            process.wait()
        except KeyboardInterrupt:
            return
        raise exited()
    finally:
        if process.poll() is None:
            process.kill()
            process.wait()


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--with-latency", action="store_true")
    parser.add_argument("--name", default="arducopter-sitl")
    parser.add_argument("--timeout", type=float, default=60)
    parser.add_argument(
        "--native",
        type=Path,
        metavar="ARDUPILOT_DIR",
        help="Run the SITL build in an ArduPilot checkout in the foreground instead of Docker",
    )
    args = parser.parse_args(argv)
    if args.timeout <= 0:
        parser.error("--timeout must be positive")
    if args.native and args.with_latency:
        parser.error("--with-latency needs Docker")
    if args.native:
        try:
            run_native(args.native, args.timeout)
        except (
            OSError,
            ValueError,
            RuntimeError,
            subprocess.SubprocessError,
            KeyboardInterrupt,
        ) as error:
            print(f"SITL failed: {error}", file=sys.stderr)
            return 1
        return 0
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
                    BUILD_CONTEXT,
                ],
                check=True,
            )
        command = sitl_command(
            "/ardupilot/build/sitl/bin/arducopter",
            "/ardupilot/Tools/autotest/default_params/copter.parm",
        )
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
