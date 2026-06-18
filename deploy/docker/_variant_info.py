#!/usr/bin/env python3
"""Emit eval-able bash describing a Docker build variant for run-docker.sh.

Reads deploy/docker/variants.json (the shared variant source) and prints shell
assignments: target, default_image, fuse, and a build_args array of --build-arg
flags. With no argument (or an unknown id) it lists the valid ids and exits 1.
"""

from __future__ import annotations

import json
import shlex
import sys
from pathlib import Path

VARIANTS_JSON = Path(__file__).resolve().parent / "variants.json"


def main(argv: list[str]) -> int:
    variants = json.loads(VARIANTS_JSON.read_text())["variants"]
    by_id = {v["id"]: v for v in variants}

    requested = argv[1] if len(argv) > 1 else ""
    variant = by_id.get(requested)
    if variant is None:
        sys.stderr.write(f"Unknown variant: {requested or '(none)'}\n")
        sys.stderr.write(f"Valid variants: {' | '.join(by_id)}\n")
        return 1

    flags: list[str] = []
    for key, value in variant["build_args"].items():
        flags += ["--build-arg", shlex.quote(f"{key}={value}")]

    print(f"target={shlex.quote(variant['target'])}")
    print(f"default_image={shlex.quote(variant['image'])}")
    print(f"fuse={'1' if variant['fuse'] else '0'}")
    print(f"build_args=({' '.join(flags)})")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
