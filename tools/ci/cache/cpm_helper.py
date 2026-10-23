#!/usr/bin/env python3
"""
CPM helper for CI: dependency fingerprints, cache paths, and runner image seeds.

cache_plan.py imports the CPM and GStreamer SDK fingerprints and the cache path setup.

Subcommands:
    create-seed     Populate a runner image seed through CMake configuration
    seed-cache      Copy a compatible seed into an empty job cache
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import re
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

import _bootstrap as _bootstrap
from common.gh_actions import github_cache_path
from common.io import read_json, write_json

# A dependency declaration outside a comment
_DEPENDENCY_DECLARATION = re.compile(
    rb"^[^#\n]*\b(?:CPMAddPackage|CPMDeclarePackage|CPMFindPackage|FetchContent_Declare)\s*\(",
    re.MULTILINE,
)


def _declares_dependency(path: Path) -> bool:
    try:
        return _DEPENDENCY_DECLARATION.search(path.read_bytes()) is not None
    except OSError:
        return False


def compute_cpm_fingerprint(root: Path) -> str:
    """Hash dependency declarations, option defaults, and source patches.

    Only CMake files that declare a dependency take part, so ordinary build-script
    edits keep the fingerprint and reuse the default branch's source snapshot.
    """
    candidates: list[Path] = []
    for exact in [
        root / "CMakeLists.txt",
        root / "CMakePresets.json",
        root / "cmake/CustomOptions.cmake",
        root / "cmake/modules/CPM.cmake",
        root / "cmake/modules/CPMPatchCache.cmake",
        root / ".github/build-config.json",
    ]:
        if exact.exists():
            candidates.append(exact)

    presets = root / "cmake/presets"
    if presets.exists():
        candidates.extend(presets.glob("*.json"))

    for directory in ("cmake", "src", "test"):
        base = root / directory
        if base.exists():
            candidates.extend(base.rglob("*.patch"))
            for pattern in ("CMakeLists.txt", "*.cmake"):
                candidates.extend(
                    path for path in base.rglob(pattern) if _declares_dependency(path)
                )

    dep_files = [path for path in candidates if path.is_file()]

    digest = hashlib.sha256()
    rel_paths = sorted({path.relative_to(root).as_posix() for path in dep_files})
    for rel_path in rel_paths:
        path = root / rel_path
        try:
            content = path.read_bytes()
        except OSError:
            continue
        content = content.replace(b"\r\n", b"\n").replace(
            b"\r", b"\n"
        )  # Windows autocrlf parity for shared CPM keys.
        digest.update(rel_path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(content)
        digest.update(b"\0")
    return digest.hexdigest()


def compute_gstreamer_fingerprint(root: Path) -> str:
    """Hash the build-config.json gstreamer section: the SDK versions, checksums and CA bundle."""
    config = read_json(root / ".github/build-config.json")
    section = json.dumps(config.get("gstreamer", {}), sort_keys=True, separators=(",", ":"))
    return hashlib.sha256(section.encode("utf-8")).hexdigest()


def prepare_cpm_cache(path_value: str, workspace: Path) -> dict[str, str]:
    """Create the CPM cache and return its absolute CMake and workspace-relative archive paths."""
    cache_path = Path(path_value.replace("\\", "/"))
    cache_path = (workspace / cache_path).resolve()
    cache_path.mkdir(parents=True, exist_ok=True)
    return {
        "CPM_SOURCE_CACHE": cache_path.as_posix(),
        "CPM_CACHE_PATH": github_cache_path(cache_path),
    }


def cache_size(path: Path) -> int:
    return sum(
        item.stat().st_size for item in path.rglob("*") if item.is_file() and not item.is_symlink()
    )


def create_seed(root: Path, qt_root: Path, seed: Path) -> None:
    if seed.exists() and any(seed.iterdir()):
        raise ValueError(f"Refusing to replace an existing CPM seed: {seed}")
    sources = seed / "sources"
    sources.mkdir(parents=True, exist_ok=True)
    started = time.monotonic()
    env = os.environ | {"QT_ROOT_DIR": str(qt_root), "CPM_SOURCE_CACHE": str(sources)}
    with tempfile.TemporaryDirectory(prefix="qgc-cpm-seed-") as temporary:
        subprocess.run(
            [
                str(qt_root / "bin/qt-cmake"),
                "--preset",
                "Linux",
                "-S",
                str(root),
                "-B",
                temporary,
                "-DQGC_BUILD_TESTING=OFF",
                "-DQGC_STABLE_BUILD=OFF",
                f"-DPython3_EXECUTABLE={sys.executable}",
            ],
            check=True,
            env=env,
        )
    if not any(sources.iterdir()):
        raise ValueError("CMake produced an empty CPM source cache")
    commit = subprocess.run(
        ["git", "-C", str(root), "rev-parse", "HEAD"], check=True, capture_output=True, text=True
    ).stdout.strip()
    manifest = {
        "schema": 1,
        "fingerprint": compute_cpm_fingerprint(root),
        "source_commit": commit,
        "bytes": cache_size(sources),
        "prepare_seconds": round(time.monotonic() - started, 2),
    }
    write_json(seed / "manifest.json", manifest, indent=2)
    print(json.dumps(manifest))


def seed_cache(root: Path, destination: Path, seed: Path) -> bool:
    manifest_path = seed / "manifest.json"
    if not manifest_path.is_file():
        print(f"No preinstalled CPM seed at {seed}")
        return False
    if destination == seed or seed in destination.parents or destination in seed.parents:
        raise ValueError("CPM cache and seed must be separate directories")
    manifest = read_json(manifest_path)
    if not isinstance(manifest, dict):
        raise ValueError("Invalid CPM seed manifest")
    if manifest.get("schema") != 1 or manifest.get("fingerprint") != compute_cpm_fingerprint(root):
        print(
            "Preinstalled CPM seed does not match this checkout; using normal dependency downloads"
        )
        return False
    if destination.exists() and any(destination.iterdir()):
        print("CPM cache already populated; keeping restored sources")
        return False
    sources = seed / "sources"
    if not sources.is_dir() or not any(sources.iterdir()):
        raise ValueError(f"Preinstalled CPM seed has no sources: {seed}")
    started = time.monotonic()
    destination.parent.mkdir(parents=True, exist_ok=True)
    # Copy into a temporary sibling so an interrupted copy cannot become a usable cache.
    with tempfile.TemporaryDirectory(prefix=".qgc-cpm-copy-", dir=destination.parent) as temporary:
        staged = Path(temporary) / "sources"
        shutil.copytree(sources, staged, symlinks=True)
        for item in [staged, *staged.rglob("*")]:
            if not item.is_symlink():
                item.chmod(item.stat().st_mode | 0o200)
        if destination.exists():
            destination.rmdir()
        staged.rename(destination)
    duration = round(time.monotonic() - started, 2)
    print(
        f"Seeded CPM cache: {manifest['bytes']} bytes in {duration}s (prepared in {manifest['prepare_seconds']}s)"
    )
    return True


def parse_args(argv: list[str] | None = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="CPM helper for CI: runner image CPM source seeds",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    create = sub.add_parser(
        "create-seed", help="Configure the image's source revision to preload CPM sources"
    )
    create.add_argument("--root", type=Path, required=True)
    create.add_argument("--qt-root", type=Path, required=True)
    create.add_argument("--seed", type=Path, required=True)
    seed = sub.add_parser(
        "seed-cache", help="Populate an empty cache from a compatible runner image seed"
    )
    seed.add_argument("--root", type=Path, required=True)
    seed.add_argument("--path", type=Path, required=True)
    seed.add_argument("--seed", type=Path, required=True)

    return parser.parse_args(argv)


def main(argv: list[str] | None = None) -> int:
    args = parse_args(argv)

    if args.command in ("create-seed", "seed-cache"):
        try:
            if args.command == "create-seed":
                create_seed(args.root.resolve(), args.qt_root.resolve(), args.seed.resolve())
            else:
                seed_cache(args.root.resolve(), args.path.resolve(), args.seed.resolve())
            return 0
        except (OSError, ValueError, subprocess.CalledProcessError) as error:
            print(f"CPM seed operation failed: {error}", file=sys.stderr)
            return 1

    print("Error: a subcommand is required (create-seed, seed-cache)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
