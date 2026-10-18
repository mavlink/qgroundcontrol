#!/usr/bin/env python3
"""Generate, validate and upload QGC terrarium elevation tiles from Copernicus GLO-30.

Usage:
    uv run --project tools --group terrain python tools/terrain_tiles.py generate --out build/terrain --area alps
    uv run --project tools --group terrain python tools/terrain_tiles.py validate --out build/terrain
    uv run --project tools --group terrain python tools/terrain_tiles.py upload --out build/terrain --prefix validation --data-version 0.1.0
"""

from __future__ import annotations

import sys

from _bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from terrain.cli import main

if __name__ == "__main__":
    sys.exit(main())
