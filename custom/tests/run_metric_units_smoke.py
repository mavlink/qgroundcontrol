#!/usr/bin/env python3
"""Verify forced metric settings in a real AppImage, isolated from aircraft.

Run inside Docker with --network none, no device passthrough, and a writable
scratch directory. Does not establish visual or aircraft-control acceptance.
"""

import argparse
import configparser
import os
import signal
import subprocess
from pathlib import Path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("appimage", type=Path)
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
    settings_file = root / "config/CODRONE/CODRONEControl Daily.ini"
    settings_file.parent.mkdir(parents=True, exist_ok=True)
    settings_file.write_text(
        "[General]\nMetricSmokeMarker=preserve\nvirtualJoystick=false\nsendGCSHeartbeat=true\n"
        "[Units]\nhorizontalDistanceUnits=0\nverticalDistanceUnits=0\nareaUnits=5\n"
        "speedUnits=2\ntemperatureUnits=1\nweightUnits=3\ncoordinateFormat=2\n"
    )
    env = dict(
        os.environ, HOME=str(root / "home"), XDG_CONFIG_HOME=str(root / "config"),
        XDG_DATA_HOME=str(root / "data"), XDG_CACHE_HOME=str(root / "cache"),
        XDG_RUNTIME_DIR=str(root / "runtime"), QT_QPA_PLATFORM="offscreen",
        QSG_RHI_BACKEND="software", QML_IMPORT_TRACE="1", LANG="en_US.UTF-8", LC_ALL="en_US.UTF-8",
    )
    with (root / "app.log").open("w") as log:
        process = subprocess.Popen(
            [str(args.appimage.resolve()), "--appimage-extract-and-run"], env=env,
            stdout=log, stderr=subprocess.STDOUT, start_new_session=True,
        )
        try:
            try:
                process.wait(timeout=20)
            except subprocess.TimeoutExpired:
                pass
            if process.poll() is not None:
                raise RuntimeError(f"AppImage exited with {process.returncode}; inspect {root / 'app.log'}")
        finally:
            if process.poll() is None:
                os.killpg(process.pid, signal.SIGTERM)
            try:
                process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                os.killpg(process.pid, signal.SIGKILL)
                process.wait()
    text = (root / "app.log").read_text(errors="replace")
    fatal_patterns = ("QQmlApplicationEngine failed", "ReferenceError:", "TypeError:", "is not a type", "Cannot assign")
    failures = [line for line in text.splitlines() if any(pattern in line for pattern in fatal_patterns)]
    if failures:
        raise RuntimeError(f"QML errors: {failures[:5]}")
    settings = configparser.ConfigParser()
    settings.read(settings_file)
    expected_units = dict(horizontalDistanceUnits=1, verticalDistanceUnits=1, areaUnits=1,
                          speedUnits=1, temperatureUnits=0, weightUnits=1)
    for name, value in expected_units.items():
        actual = settings.getint("Units", name)
        if actual != value:
            raise RuntimeError(f"{name}: expected forced metric enum {value}, got {actual}")
    if settings.getint("Units", "coordinateFormat") != 2:
        raise RuntimeError("Unrelated coordinate-format preference changed")
    if settings.get("General", "MetricSmokeMarker") != "preserve":
        raise RuntimeError("Unrelated settings marker changed")
    if not settings.getboolean("General", "virtualJoystick") or settings.getboolean("General", "sendGCSHeartbeat"):
        raise RuntimeError("Existing joystick/heartbeat policies regressed")
    print("PASS: packaged app overrides all six saved imperial preferences with metric units")
    print("PASS: coordinate format preserved, joysticks on, heartbeat off, no detected QML errors")
    print("SETTINGS", settings_file)
    print("LOG_DIRECTORY", root)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
