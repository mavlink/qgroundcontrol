#!/usr/bin/env python3
"""Exercise both AppImages concurrently in a network/device-isolated container.

Requires --network none, no aircraft devices, and a writable scratch directory.
Does not prove telemetry sharing or physical aircraft control.
"""

import argparse
import os
import signal
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("official", type=Path)
    parser.add_argument("custom", type=Path)
    parser.add_argument("scratch", type=Path)
    args = parser.parse_args()
    if not Path("/.dockerenv").exists() or set(os.listdir("/sys/class/net")) != {"lo"}:
        raise SystemExit("Run only in a container with --network none")
    if list(Path("/dev").glob("ttyACM*")) or list(Path("/dev").glob("ttyUSB*")):
        raise SystemExit("Aircraft serial devices must not be exposed")
    root = args.scratch.resolve()
    for name in ("home", "config", "data", "cache", "runtime"):
        (root / name).mkdir(parents=True, exist_ok=True)
    (root / "runtime").chmod(0o700)
    official_config = root / "config/QGroundControl"
    official_config.mkdir(exist_ok=True)
    for name in ("QGroundControl.ini", "QGroundControl Daily.ini"):
        (official_config / name).write_text(
            "[General]\nsideBySideMarker=official\nvirtualJoystick=false\nsendGCSHeartbeat=true\n"
        )
    env = dict(os.environ, HOME=str(root / "home"), XDG_CONFIG_HOME=str(root / "config"),
               XDG_DATA_HOME=str(root / "data"), XDG_CACHE_HOME=str(root / "cache"),
               XDG_RUNTIME_DIR=str(root / "runtime"), QT_QPA_PLATFORM="offscreen",
               QSG_RHI_BACKEND="software", QML_IMPORT_TRACE="1")
    processes = []
    logs = []
    try:
        for label, binary in (("official", args.official), ("custom", args.custom)):
            log = (root / f"{label}.log").open("w")
            logs.append(log)
            processes.append(subprocess.Popen(
                [str(binary.resolve()), "--appimage-extract-and-run"], env=env,
                stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
            ))
        try:
            processes[0].wait(timeout=20)
        except subprocess.TimeoutExpired:
            pass
        for label, process in zip(("official", "custom"), processes):
            print(label, "PID", process.pid, "RUNNING", process.poll() is None, flush=True)
        if any(process.poll() is not None for process in processes):
            raise RuntimeError("An application exited; inspect the isolated logs")
        print("PASS: both AppImages remain running in the same IPC and settings environment", flush=True)
    finally:
        for process in processes:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
        for process in processes:
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
        for log in logs:
            log.close()
    for label in ("official", "custom"):
        text = (root / f"{label}.log").read_text(errors="replace")
        fatal_patterns = ("QQmlApplicationEngine failed", "ReferenceError:", "TypeError:", "is not a type", "Cannot assign")
        failures = [line for line in text.splitlines() if any(pattern in line for pattern in fatal_patterns)]
        if failures:
            raise RuntimeError(f"{label} QML errors: {failures[:5]}")
    custom_configs = list((root / "config/CODRONE").glob("CODRONEControl*.ini"))
    if len(custom_configs) != 1:
        raise RuntimeError(f"Expected a separate custom settings file, found {custom_configs}")
    settings = custom_configs[0].read_text()
    if "virtualJoystick=true" not in settings or "sendGCSHeartbeat=false" not in settings:
        raise RuntimeError("Custom joystick/heartbeat policy was not preserved")
    for config in official_config.glob("*.ini"):
        text = config.read_text()
        if "sideBySideMarker=official" not in text or "sendGCSHeartbeat=true" not in text:
            raise RuntimeError(f"Official settings changed unexpectedly: {config}")
    print("PASS: separate settings; official heartbeat unchanged; custom heartbeat off and joysticks on")
    print("CUSTOM_SETTINGS", custom_configs[0])
    print("LOG_DIRECTORY", root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
