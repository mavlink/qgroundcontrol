#!/usr/bin/env python3
"""Validate a Build GStreamer dispatch and resolve its runner settings before the long build.

build-gstreamer.yml runs this first, so an architecture the platform's runner cannot build fails
in seconds instead of after a two-hour build or with a mislabeled archive. The workflow's
``runs-on`` picks an arm64 runner for Linux ``aarch64`` and Windows ``arm64``; desktop builds are
native (macOS also builds ``x86_64`` slices and ``universal``), and Cerbero cross-builds Android
and iOS. Step outputs:

- ``qt-host``, ``qt-arch``: the Qt that ``build-setup`` installs for desktop builds (empty for
  Android and iOS, which skip it)
- ``apt-category``: the ``install_dependencies`` category of a Linux-hosted build (empty on macOS
  and Windows)
- ``simulator``: ``true`` for an iOS simulator build, else empty (other platforms ignore the input)
- ``cmake-version``, ``ninja-version``: build-config's CMake and Ninja pins for Android, which skips
  ``build-setup`` (where every other CI build gets them); empty elsewhere
"""

from __future__ import annotations

import argparse
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.build_config import load_build_config
from common.gh_actions import gh_error, parse_bool, write_github_output
from setup.gstreamer.cerbero import CerberoBuilder
from setup.gstreamer.cli import resolve_arch

PLATFORMS = ("linux", "macos", "windows", "android", "ios")

# (qt-host, qt-arch) by platform and resolved arch.
DESKTOP_QT: dict[str, dict[str, tuple[str, str]]] = {
    "linux": {
        "x86_64": ("linux", "linux_gcc_64"),
        "aarch64": ("linux_arm64", "linux_gcc_arm64"),
    },
    "macos": dict.fromkeys(("x86_64", "arm64", "universal"), ("mac", "clang_64")),
    "windows": {
        "x64": ("windows", "win64_msvc2022_64"),
        "arm64": ("windows_arm64", "win64_msvc2022_arm64"),
    },
}

APT_CATEGORIES = {"linux": "gstreamer_build", "android": "gstreamer_build_android"}


def plan(platform: str, arch: str, simulator: bool) -> dict[str, str]:
    """Return the step outputs for a platform and requested arch; raise ValueError if unsupported."""
    if platform not in PLATFORMS:
        raise ValueError(f"Unknown platform {platform!r}; expected one of {', '.join(PLATFORMS)}")
    if not arch:
        raise ValueError("An architecture is required")
    simulator = simulator and platform == "ios"
    resolved = resolve_arch(platform, arch)
    if platform in DESKTOP_QT:
        supported = tuple(DESKTOP_QT[platform])
    else:
        supported = CerberoBuilder.CERBERO_ARCHS[(platform, simulator)]
    if resolved not in supported:
        target = f"{platform}{' simulator' if simulator else ''}"
        raise ValueError(
            f"{target} cannot build arch {arch!r}; choose one of {', '.join(supported)}"
        )
    qt_host, qt_arch = DESKTOP_QT.get(platform, {}).get(resolved, ("", ""))
    pins = load_build_config()["build"] if platform == "android" else {}
    return {
        "qt-host": qt_host,
        "qt-arch": qt_arch,
        "apt-category": APT_CATEGORIES.get(platform, ""),
        "simulator": "true" if simulator else "",
        "cmake-version": pins.get("cmake_version", ""),
        "ninja-version": pins.get("ninja_version", ""),
    }


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--platform", required=True, help="Target platform")
    parser.add_argument("--arch", required=True, help="Requested architecture")
    parser.add_argument(
        "--simulator",
        type=parse_bool,
        default=False,
        help="Whether an iOS simulator build was requested (true/false)",
    )
    args = parser.parse_args(argv)
    try:
        outputs = plan(args.platform, args.arch, args.simulator)
    except ValueError as error:
        gh_error(str(error), title="Unsupported GStreamer build")
        return 1
    print(", ".join(f"{name}={value or '(none)'}" for name, value in outputs.items()))
    write_github_output(outputs)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
