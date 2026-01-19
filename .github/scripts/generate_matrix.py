#!/usr/bin/env python3
"""
Generate GitHub Actions matrix from build-config.json.

Usage:
    ./generate_matrix.py --platform linux
    ./generate_matrix.py --platform windows --build-type Release
    ./generate_matrix.py --platform linux --event-name pull_request
    ./generate_matrix.py --platform android --pr-minimal

Outputs JSON matrix for GitHub Actions.
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any


def load_config() -> dict[str, Any]:
    """Load build-config.json from the repository root."""
    script_dir = Path(__file__).parent
    config_path = script_dir.parent / "build-config.json"

    if not config_path.exists():
        # Try alternate location
        config_path = Path(__file__).parent.parent / "build-config.json"

    if not config_path.exists():
        print(f"Error: build-config.json not found at {config_path}", file=sys.stderr)
        sys.exit(1)

    with open(config_path) as f:
        return json.load(f)


def generate_matrix(
    config: dict[str, Any],
    platform: str,
    build_type: str | None = None,
    event_name: str | None = None,
    pr_minimal: bool = False,
    variant_filter: str | None = None,
) -> dict[str, Any]:
    """
    Generate matrix configuration for a platform.

    Args:
        config: The build-config.json contents
        platform: Platform to generate matrix for (linux, windows, macos, android, ios)
        build_type: Override build types (from workflow_dispatch input)
        event_name: GitHub event name (pull_request, push, etc.)
        pr_minimal: If True, use minimal configuration for PRs
        variant_filter: Only include specific variant by name

    Returns:
        Matrix configuration dict with 'include' array
    """
    platforms = config.get("platforms", {})

    if platform not in platforms:
        print(f"Error: Platform '{platform}' not found in config", file=sys.stderr)
        print(f"Available platforms: {', '.join(platforms.keys())}", file=sys.stderr)
        sys.exit(1)

    platform_config = platforms[platform]
    variants = platform_config.get("variants", [])

    if not variants:
        print(f"Error: No variants defined for platform '{platform}'", file=sys.stderr)
        sys.exit(1)

    # Determine which build types to use
    if build_type:
        # Explicit override from workflow_dispatch
        requested_build_types = [build_type]
    elif pr_minimal or event_name == "pull_request":
        # For PRs: prefer Debug for testing, fallback to first available
        requested_build_types = None  # Will be determined per-variant
    else:
        # Full build: use all defined build types
        requested_build_types = None

    include = []

    for variant in variants:
        # Filter by variant name if specified
        if variant_filter and variant.get("name") != variant_filter:
            continue

        variant_build_types = variant.get("build_types", ["Release"])

        # Determine build types for this variant
        if requested_build_types:
            # Use explicitly requested build types (filtered by what's available)
            types_to_build = [t for t in requested_build_types if t in variant_build_types]
            if not types_to_build:
                # Skip this variant if requested type not available
                continue
        elif pr_minimal or event_name == "pull_request":
            # For PRs: prefer Debug, fallback to first available
            if "Debug" in variant_build_types:
                types_to_build = ["Debug"]
            else:
                types_to_build = [variant_build_types[0]]
        else:
            # Full build: all types
            types_to_build = variant_build_types

        # Generate matrix entries for each build type
        for bt in types_to_build:
            entry = {
                "name": variant.get("name", "default"),
                "os": variant.get("runner"),
                "host": variant.get("host"),
                "arch": variant.get("arch"),
                "build_type": bt,
            }

            # Add optional fields if present
            if "package" in variant:
                entry["package"] = variant["package"]

            if "qt_version" in variant:
                entry["qt_version"] = variant["qt_version"]

            if "qt_host_path" in variant:
                entry["qt_host_path"] = variant["qt_host_path"]

            if "shell" in variant:
                entry["shell"] = variant["shell"]

            if "primary" in variant:
                entry["primary"] = variant["primary"]

            if "abis" in variant:
                entry["abis"] = variant["abis"]

            if "gstreamer" in variant:
                entry["gstreamer"] = variant["gstreamer"]

            include.append(entry)

    if not include:
        print(f"Warning: No matrix entries generated for {platform}", file=sys.stderr)

    return {
        "include": include,
        "timeout_minutes": platform_config.get("timeout_minutes", 120),
    }


def main() -> None:
    parser = argparse.ArgumentParser(
        description="Generate GitHub Actions matrix from build-config.json"
    )
    parser.add_argument(
        "--platform",
        required=True,
        choices=["linux", "windows", "macos", "android", "ios"],
        help="Target platform",
    )
    parser.add_argument(
        "--build-type",
        help="Override build type (Debug or Release)",
    )
    parser.add_argument(
        "--event-name",
        help="GitHub event name (pull_request, push, etc.)",
    )
    parser.add_argument(
        "--pr-minimal",
        action="store_true",
        help="Use minimal configuration for PRs (prefer Debug)",
    )
    parser.add_argument(
        "--variant",
        help="Filter to specific variant by name",
    )
    parser.add_argument(
        "--output",
        choices=["json", "github"],
        default="json",
        help="Output format (json or github for GITHUB_OUTPUT)",
    )

    args = parser.parse_args()

    config = load_config()

    matrix = generate_matrix(
        config=config,
        platform=args.platform,
        build_type=args.build_type,
        event_name=args.event_name,
        pr_minimal=args.pr_minimal,
        variant_filter=args.variant,
    )

    if args.output == "github":
        # Output for GITHUB_OUTPUT
        github_output = os.environ.get("GITHUB_OUTPUT")
        if github_output:
            with open(github_output, "a") as f:
                f.write(f"matrix={json.dumps(matrix['include'])}\n")
                f.write(f"timeout_minutes={matrix['timeout_minutes']}\n")
        else:
            # Fallback to stdout for testing
            print(f"matrix={json.dumps(matrix['include'])}")
            print(f"timeout_minutes={matrix['timeout_minutes']}")
    else:
        # Pretty print JSON
        print(json.dumps(matrix, indent=2))


if __name__ == "__main__":
    main()
