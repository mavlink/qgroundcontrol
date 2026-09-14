#!/usr/bin/env python3
"""
CPM helper for CI: cache configuration and dependency fingerprinting.

Subcommands:
    fingerprint     Hash CMake files that declare CPM/FetchContent dependencies
    configure-cache Configure CPM_SOURCE_CACHE env + GitHub output
    create-seed     Populate a runner image seed through CMake configuration
    seed-cache      Copy a compatible seed into an empty job cache
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from pathlib import Path

from ci_bootstrap import ensure_tools_dir

ensure_tools_dir(__file__)

from common.gh_actions import append_github_env, github_cache_path, write_github_output


def compute_cpm_fingerprint(root: Path) -> str:
    """Hash dependency declarations, option defaults, and source patches."""
    candidates: list[Path] = []
    for exact in [
        root / "CMakeLists.txt",
        root / "CMakePresets.json",
        root / "cmake/modules/CPM.cmake",
        root / ".github/build-config.json",
    ]:
        if exact.exists():
            candidates.append(exact)

    for directory in ("cmake",):
        base = root / directory
        if base.exists():
            candidates.extend(base.rglob("*.cmake"))
            candidates.extend(base.rglob("*.patch"))
            candidates.extend((base / "presets").glob("*.json"))

    for directory in ("src", "test"):
        base = root / directory
        if base.exists():
            candidates.extend(base.rglob("CMakeLists.txt"))
            candidates.extend(base.rglob("*.patch"))

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
        )  # Windows autocrlf parity for cpm-modules-shared- key.
        digest.update(rel_path.encode("utf-8"))
        digest.update(b"\0")
        digest.update(content)
        digest.update(b"\0")
    return digest.hexdigest()


def configure_cpm_cache(path_value: str) -> Path:
    """Normalize and create the CPM cache path and export it."""
    cache_path = Path(path_value.replace("\\", "/"))
    cache_path = (Path(os.environ.get("GITHUB_WORKSPACE", ".")) / cache_path).resolve()
    cache_path.mkdir(parents=True, exist_ok=True)
    posix_path = cache_path.as_posix()
    append_github_env(
        {"CPM_SOURCE_CACHE": posix_path, "CPM_CACHE_PATH": github_cache_path(cache_path)}
    )
    write_github_output({"path": posix_path})
    return cache_path


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
    (seed / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest))


def seed_cache(root: Path, destination: Path, seed: Path) -> bool:
    manifest_path = seed / "manifest.json"
    if not manifest_path.is_file():
        print(f"No preinstalled CPM seed at {seed}")
        return False
    if destination == seed or seed in destination.parents or destination in seed.parents:
        raise ValueError("CPM cache and seed must be separate directories")
    manifest = json.loads(manifest_path.read_text())
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
        description="CPM helper for CI: cache configuration and dependency fingerprinting",
        formatter_class=argparse.RawDescriptionHelpFormatter,
    )
    sub = parser.add_subparsers(dest="command")

    fp = sub.add_parser("fingerprint", help="Compute CPM dependency fingerprint")
    fp.add_argument("--root", type=Path, default=Path("."), help="Repository root")

    cfg = sub.add_parser("configure-cache", help="Configure CPM source cache path")
    cfg.add_argument("--path", required=True)

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

    if args.command == "fingerprint":
        fingerprint = compute_cpm_fingerprint(args.root.resolve())
        print(fingerprint)
        write_github_output({"fingerprint": fingerprint})
        return 0

    if args.command == "configure-cache":
        cache_path = configure_cpm_cache(args.path)
        print(cache_path)
        return 0

    print("Error: a subcommand is required (fingerprint, configure-cache)", file=sys.stderr)
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
