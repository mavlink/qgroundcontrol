#!/usr/bin/env python3
"""Generate a CycloneDX SBOM from a CMake build directory's CPM package metadata.

Parses CMakeCache.txt for CPM_PACKAGES and per-package VERSION/SOURCE_DIR,
then reads git metadata (remote URL, commit hash) from each source directory.

Usage:
    generate_cpm_sbom.py --build-dir DIR [--output FILE]

Outputs CycloneDX 1.6 JSON to stdout or the specified file.
"""

from __future__ import annotations

import argparse
import json
import re
import subprocess
import sys
import uuid
from datetime import datetime, timezone
from pathlib import Path


def parse_cmake_cache(cache_path: Path) -> dict[str, str]:
    """Extract typed entries from CMakeCache.txt into a flat dict."""
    entries: dict[str, str] = {}
    for line in cache_path.read_text(encoding="utf-8").splitlines():
        m = re.match(r"^([A-Za-z0-9_.\-]+):[A-Z]+=(.*)$", line)
        if m:
            entries[m.group(1)] = m.group(2)
    return entries


def git_info(source_dir: Path) -> tuple[str, str]:
    """Return (remote_url, commit_hash) from a git working directory."""
    url = ""
    commit = ""
    try:
        result = subprocess.run(
            ["git", "-C", str(source_dir), "remote", "get-url", "origin"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            url = result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    try:
        result = subprocess.run(
            ["git", "-C", str(source_dir), "rev-parse", "HEAD"],
            capture_output=True, text=True, timeout=5,
        )
        if result.returncode == 0:
            commit = result.stdout.strip()
    except (OSError, subprocess.TimeoutExpired):
        pass
    return url, commit


def normalize_git_url(url: str) -> str:
    """Convert git URL to HTTPS browse URL for purl."""
    url = url.removesuffix(".git")
    if url.startswith("git@github.com:"):
        url = "https://github.com/" + url[len("git@github.com:"):]
    return url


def make_purl(name: str, version: str, url: str, commit: str) -> str:
    """Build a Package URL (purl) for a CPM dependency."""
    normalized = normalize_git_url(url)
    m = re.match(r"https://github\.com/([^/]+/[^/]+)", normalized)
    if m:
        repo_path = m.group(1).lower()
        ref = version if version and version != "0" else commit[:12]
        purl = f"pkg:github/{repo_path}@{ref}" if ref else f"pkg:github/{repo_path}"
        return purl
    m = re.match(r"https://gitlab\.com/([^/]+/[^/]+)", normalized)
    if m:
        repo_path = m.group(1).lower()
        ref = version if version and version != "0" else commit[:12]
        return f"pkg:gitlab/{repo_path}@{ref}" if ref else f"pkg:gitlab/{repo_path}"
    return f"pkg:generic/{name.lower()}@{version}" if version and version != "0" else f"pkg:generic/{name.lower()}"


def generate_sbom(build_dir: Path) -> dict:
    cache_path = build_dir / "CMakeCache.txt"
    if not cache_path.exists():
        print(f"Error: {cache_path} not found", file=sys.stderr)
        sys.exit(1)

    cache = parse_cmake_cache(cache_path)

    packages_str = cache.get("CPM_PACKAGES", "")
    if not packages_str:
        print("Warning: No CPM_PACKAGES found in cache", file=sys.stderr)

    package_names = [p for p in packages_str.split(";") if p]

    components = []
    for name in package_names:
        version = cache.get(f"CPM_PACKAGE_{name}_VERSION", "")
        source_dir = cache.get(f"CPM_PACKAGE_{name}_SOURCE_DIR", "")

        url, commit = "", ""
        if source_dir:
            url, commit = git_info(Path(source_dir))

        display_version = version if version and version != "0" else commit[:12] if commit else "unknown"

        component: dict = {
            "type": "library",
            "name": name,
            "version": display_version,
        }

        purl = make_purl(name, version, url, commit)
        component["purl"] = purl
        component["bom-ref"] = purl

        external_refs = []
        if url:
            browse_url = normalize_git_url(url)
            external_refs.append({"type": "vcs", "url": browse_url})
        if external_refs:
            component["externalReferences"] = external_refs

        if commit:
            component["hashes"] = [
                {"alg": "SHA-1", "content": commit}
            ]

        components.append(component)

    sbom = {
        "$schema": "http://cyclonedx.org/schema/bom-1.6.schema.json",
        "bomFormat": "CycloneDX",
        "specVersion": "1.6",
        "version": 1,
        "serialNumber": f"urn:uuid:{uuid.uuid4()}",
        "metadata": {
            "timestamp": datetime.now(timezone.utc).isoformat(),
            "component": {
                "type": "application",
                "name": "QGroundControl",
                "bom-ref": "pkg:github/mavlink/qgroundcontrol",
            },
            "tools": {
                "components": [
                    {
                        "type": "application",
                        "name": "generate_cpm_sbom",
                        "version": "1.0.0",
                    }
                ]
            },
        },
        "components": components,
    }

    return sbom


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, required=True, help="CMake build directory")
    parser.add_argument("--output", "-o", type=Path, help="Output file (default: stdout)")
    args = parser.parse_args()

    sbom = generate_sbom(args.build_dir)
    output = json.dumps(sbom, indent=2)

    if args.output:
        args.output.write_text(output, encoding="utf-8")
        print(f"SBOM written to {args.output} ({len(sbom['components'])} components)")
    else:
        print(output)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
