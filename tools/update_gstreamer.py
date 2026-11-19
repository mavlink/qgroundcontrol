#!/usr/bin/env python3
"""Pin a GStreamer SDK release in .github/build-config.json.

Sets ``gstreamer.version.<platform>`` of every SDK platform to the release and replaces
``gstreamer.checksums`` with the SHA-256 of its packages, read from the ``.sha256sum`` file upstream
publishes beside each one. Nothing is written unless upstream has every package of the release;
``check_deps.py --gstreamer`` names the latest release it has for every platform.

The checksums come from the server that serves the packages, so they record what upstream published
when the pin was made: review them like any other pin. From then on configure and the S3 mirror
(``ci/gstreamer/mirror_gstreamer.py``) hold every download to them.
"""

from __future__ import annotations

import argparse
import re

import _bootstrap as _bootstrap
from common.build_config import find_build_config
from common.gstreamer import PLATFORM_PACKAGES, upstream_checksum
from common.io import read_json, write_json
from common.logging import log_error, log_ok


def _release_version(value: str) -> str:
    if not re.fullmatch(r"\d+\.\d+\.\d+", value):
        raise argparse.ArgumentTypeError(f"{value!r} is not <major>.<minor>.<micro>")
    return value


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("version", type=_release_version, help="GStreamer release, e.g. 1.28.9")
    version = parser.parse_args(argv).version

    checksums: dict[str, str] = {}
    for packages in PLATFORM_PACKAGES.values():
        for package in packages:
            try:
                checksums[package] = upstream_checksum(package, version)
            except (OSError, RuntimeError) as exc:
                log_error(f"No checksum for the {package} package of GStreamer {version}: {exc}")
                return 1

    config_file = find_build_config()
    config = read_json(config_file)
    for platform in PLATFORM_PACKAGES:
        config["gstreamer"]["version"][platform] = version
    config["gstreamer"]["checksums"] = {version: checksums}
    write_json(config_file, config)
    log_ok(f"Pinned GStreamer {version} in {config_file}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
