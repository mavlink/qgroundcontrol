#!/usr/bin/env python3
"""Build GStreamer from source for QGroundControl: Meson on desktop, Cerbero on Android and iOS.

Usage:
    python3 tools/setup/build_gstreamer.py --platform linux [OPTIONS]
    python3 tools/setup/build_gstreamer.py --help

The implementation is the setup/gstreamer package.
"""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from setup.gstreamer.cli import main

if __name__ == "__main__":
    raise SystemExit(main())
