#!/usr/bin/env python3
"""Build QGC in a provisioned VM, keeping build output off the host mount."""

from __future__ import annotations

import argparse
import json
import os
import subprocess
import sys
from pathlib import Path


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument(
        "--skip-dependencies", action="store_true", help="Reuse a provisioned VM environment"
    )
    args = parser.parse_args(argv)
    repo = Path(__file__).resolve().parents[2]
    output = Path(os.environ.get("BUILD_DIR", Path.home() / "qgc-build")).resolve()
    build_type = os.environ.get("BUILD_TYPE", "Release")
    qt_out = Path(os.environ.get("QT_OUT", Path.home() / "Qt")).resolve()
    try:
        if build_type not in ("Release", "Debug", "RelWithDebInfo", "MinSizeRel"):
            raise ValueError(f"Invalid build type: {build_type}")
        os.chdir(repo)
        os.environ["PATH"] = f"{Path.home() / '.local/bin'}:{os.environ['PATH']}"
        environment = Path(os.environ.get("QGC_PYTHON_ENV", str(repo / "tools/.venv"))).resolve()
        if not args.skip_dependencies:
            subprocess.run([sys.executable, "tools/setup/install_uv.py"], check=True)
            subprocess.run(
                [
                    sys.executable,
                    "tools/setup/install_python.py",
                    "build,qt",
                    "--environment",
                    str(environment),
                ],
                check=True,
            )
        python = str(environment / "bin/python")
        os.environ["PATH"] = f"{environment / 'bin'}:{os.environ['PATH']}"
        if not args.skip_dependencies:
            subprocess.run(
                [python, "tools/setup/install_dependencies", "--platform", "debian"], check=True
            )
        config = json.loads((repo / ".github/build-config.json").read_text())["qt"]
        subprocess.run(
            [
                python,
                "tools/setup/install_qt.py",
                "install",
                "--version",
                config["version"],
                "--host",
                "linux",
                "--arch",
                "linux_gcc_64",
                "--modules",
                config["modules"],
                "--outdir",
                str(qt_out),
            ],
            check=True,
        )
        qt_root = qt_out / config["version"] / "gcc_64"
        os.environ["QT_ROOT_DIR"] = str(qt_root)
        presets = {
            "Release": "Linux",
            "Debug": "Linux-debug",
            "RelWithDebInfo": "Linux-relwithdebinfo",
        }
        preset = ["--preset", presets[build_type]] if build_type in presets else []
        subprocess.run(
            [
                str(qt_root / "bin/qt-cmake"),
                *preset,
                "-S",
                str(repo),
                "-B",
                str(output),
                "-G",
                "Ninja",
                f"-DCMAKE_BUILD_TYPE={build_type}",
                "-DQGC_BUILD_TESTING=OFF",
                "-DQGC_STABLE_BUILD=OFF",
                f"-DCMAKE_PREFIX_PATH={qt_root}",
            ],
            check=True,
        )
        jobs = [os.environ["JOBS"]] if os.environ.get("JOBS") else []
        subprocess.run(
            ["cmake", "--build", str(output), "--config", build_type, "--parallel", *jobs],
            check=True,
        )
        subprocess.run(["cmake", "--install", str(output), "--config", build_type], check=True)
        artifacts = sorted(output.glob("*.AppImage"))
        if len(artifacts) != 1:
            raise ValueError(f"Expected one AppImage in {output}, found {len(artifacts)}")
        (Path.home() / "qgc-appimage-path").write_text(f"{artifacts[0]}\n")
        print(f"AppImage: {artifacts[0]}")
        return 0
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f"VM build failed: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
