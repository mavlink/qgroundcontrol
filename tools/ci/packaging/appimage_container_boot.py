#!/usr/bin/env python3
"""Boot-test an AppImage in a clean container of a distribution it supports.

The container holds only what an end user's desktop gives the AppImage: glibc and libstdc++, the
X11, OpenGL/EGL, GBM, Wayland client, ALSA, font and com_err libraries it leaves to the system, the
CA certificates GnuTLS verifies servers with, and Xvfb.
No GStreamer or Qt is installed, and no GST_PLUGIN_* or GST_REGISTRY* variable is set, so a boot
proves the AppImage brings its own. It runs with --appimage-extract-and-run (a container has no
FUSE) under xvfb-run, like verify_executable.py's AppImage boot.
"""

from __future__ import annotations

import argparse
import re
import stat
import subprocess
import sys
import time
import uuid
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.format import format_duration
from common.gh_actions import gh_error, gh_group, write_step_summary
from common.markdown import code_block, code_span, summary_section
from common.proc import output_text, run_captured

from ci.packaging.verify_executable import BOOT_LOGGING_CATEGORIES, BOOT_TIMEOUT_EXIT_CODE

# Debian and Ubuntu package names; libasound2t64 needs Ubuntu 24.04 or Debian 13. Debian's base image,
# unlike Ubuntu's, lacks libcom-err2, which every desktop install has.
RUNTIME_PACKAGES = (
    "ca-certificates",
    "libasound2t64",
    "libcom-err2",
    "libegl1",
    "libfontconfig1",
    "libfribidi0",
    "libgbm1",
    "libgl1",
    "libharfbuzz0b",
    "libwayland-client0",
    "libx11-xcb1",
    "libxcb-dri3-0",
    "libxcb-glx0",
    "libxrandr2",
    "xauth",
    "xvfb",
)
SETUP_FAILED_EXIT_CODE = 90
HOST_GSTREAMER_EXIT_CODE = 91
DOCKER_EXIT_CODES = (125, 126, 127)
MOUNT_POINT = "/appimage"
BOOT_USER = "qgc"
SETUP_TIMEOUT_SECONDS = 600
OUTPUT_TAIL_LINES = 40
# $1 is the AppImage's file name; the packages follow.
CONTAINER_SCRIPT = f"""\
set -uo pipefail
appimage="{MOUNT_POINT}/$1"
shift
export DEBIAN_FRONTEND=noninteractive
apt_get() {{ apt-get -y -qq -o Acquire::Retries=3 "$@" > /dev/null; }}
apt_get update && apt_get install --no-install-recommends "$@" || exit {SETUP_FAILED_EXIT_CODE}
for library in libgstreamer-1.0.so libQt6Core.so; do
    if compgen -G "/usr/lib/*/$library*" > /dev/null; then
        echo "The runtime packages installed $library; the boot would not prove the AppImage's own" >&2
        exit {HOST_GSTREAMER_EXIT_CODE}
    fi
done
unset GST_PLUGIN_PATH GST_PLUGIN_PATH_1_0 GST_PLUGIN_SYSTEM_PATH GST_PLUGIN_SYSTEM_PATH_1_0 \\
    GST_PLUGIN_SCANNER GST_PLUGIN_SCANNER_1_0 GST_REGISTRY GST_REGISTRY_1_0
# QGroundControl refuses to run as root, which a container runs as.
useradd --create-home {BOOT_USER} || exit {SETUP_FAILED_EXIT_CODE}
grep '^PRETTY_NAME=' /etc/os-release
echo "Booting $appimage"
exec runuser -u {BOOT_USER} -- timeout --kill-after=10 "$QGC_BOOT_TIMEOUT" \\
    xvfb-run -a "$appimage" --appimage-extract-and-run --simple-boot-test "--logging:{BOOT_LOGGING_CATEGORIES}"
"""


def docker_command(appimage: Path, image: str, name: str, boot_timeout: int) -> list[str]:
    """Return the docker run command that installs the runtime packages and boots *appimage*."""
    return [
        "docker",
        "run",
        "--rm",
        "--init",
        "--name",
        name,
        "--volume",
        f"{appimage.parent.resolve()}:{MOUNT_POINT}:ro",
        "--env",
        f"QGC_BOOT_TIMEOUT={boot_timeout}",
        "--env",
        "QT_FORCE_STDERR_LOGGING=1",
        image,
        "bash",
        "-c",
        CONTAINER_SCRIPT,
        "boot",
        appimage.name,
        *RUNTIME_PACKAGES,
    ]


def describe_result(code: int, boot_timeout: int) -> str:
    if code == 0:
        return "passed"
    if code == SETUP_FAILED_EXIT_CODE:
        return "failed to install the runtime packages"
    if code == HOST_GSTREAMER_EXIT_CODE:
        return "failed: the runtime packages installed GStreamer or Qt"
    if code == BOOT_TIMEOUT_EXIT_CODE:
        return f"timed out after {boot_timeout}s"
    if code in DOCKER_EXIT_CODES:
        return f"failed: docker run exited {code}"
    return f"failed (exit code {code})"


def run_container(cmd: list[str], name: str, timeout: int) -> tuple[int, str]:
    """Run *cmd*; return its exit code (124 after a timeout kills the container) and output."""
    try:
        result = subprocess.run(
            cmd,
            stdout=subprocess.PIPE,
            stderr=subprocess.STDOUT,
            text=True,
            encoding="utf-8",
            errors="replace",
            timeout=timeout,
            check=False,
        )
    except subprocess.TimeoutExpired as error:
        # Killing the docker client leaves its container running.
        run_captured(["docker", "kill", name], timeout=60)
        return BOOT_TIMEOUT_EXIT_CODE, output_text(error.output)
    return result.returncode, result.stdout


def _distribution(output: str) -> str:
    match = re.search(r'^PRETTY_NAME="?([^"\n]+)"?$', output, flags=re.MULTILINE)
    return match[1] if match else ""


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--appimage", type=Path, required=True)
    parser.add_argument("--image", required=True, help="Container image, e.g. ubuntu:24.04")
    parser.add_argument("--timeout", type=int, default=120, help="Boot test timeout in seconds")
    args = parser.parse_args(argv)
    if not args.appimage.is_file():
        gh_error(f"AppImage not found: {args.appimage}")
        return 1
    # Artifact downloads drop the executable bit.
    args.appimage.chmod(args.appimage.stat().st_mode | stat.S_IXUSR | stat.S_IXGRP | stat.S_IXOTH)

    name = f"qgc-appimage-boot-{uuid.uuid4().hex[:12]}"
    cmd = docker_command(args.appimage, args.image, name, args.timeout)
    print(f"Booting {args.appimage.name} in {args.image}")
    start = time.monotonic()
    code, output = run_container(cmd, name, SETUP_TIMEOUT_SECONDS + args.timeout)
    elapsed = time.monotonic() - start
    result = describe_result(code, args.timeout)
    with gh_group("Container output"):
        print(output)
    facts = [
        ("Result", result),
        ("Image", code_span(args.image)),
        ("Distribution", _distribution(output)),
        ("AppImage", code_span(args.appimage.name)),
        ("Duration", format_duration(elapsed)),
    ]
    collapsed = []
    if code != 0:
        tail = output.strip().splitlines()[-OUTPUT_TAIL_LINES:]
        collapsed.append((f"Last {len(tail)} lines of output", code_block("\n".join(tail))))
        gh_error(f"AppImage boot in {args.image} {result}", title="AppImage boot")
    else:
        print(f"AppImage boot in {args.image} passed in {elapsed:.1f}s")
    write_step_summary(
        summary_section(f"AppImage boot in {args.image}", facts, collapsed=collapsed)
    )
    return 0 if code == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
