#!/usr/bin/env python3
"""
Read build configuration from .github/build-config.json

This script provides cross-platform access to the centralized build config.
It can be used directly or called by shell wrappers for environment export.

Usage:
    read_config.py                      # Print all config as KEY=VALUE
    read_config.py --get qt_version     # Get single value
    read_config.py --json               # Output as JSON
    read_config.py --export bash        # Output as bash export statements
    read_config.py --export powershell  # Output as PowerShell $env: statements
    read_config.py --github-output      # Write to GITHUB_OUTPUT (CI only)

Environment:
    CONFIG_FILE: Override config file path (default: auto-detect)
"""

from __future__ import annotations

import argparse
import json
import os
import sys
from pathlib import Path
from typing import Any

try:
    from .setup_bootstrap import ensure_setup_imports
except ImportError:
    setup_dir = Path(__file__).resolve().parent
    if str(setup_dir) not in sys.path:
        sys.path.insert(0, str(setup_dir))
    from setup_bootstrap import ensure_setup_imports

ensure_setup_imports()

from common.build_config import (
    export_build_config_values,
    find_build_config,
    format_github_output,
    load_build_config,
)


def _escape_bash_value(value: str) -> str:
    """Escape characters that are special inside double-quoted bash strings."""
    for ch, esc in (("\\", "\\\\"), ('"', '\\"'), ("$", "\\$"), ("`", "\\`")):
        value = value.replace(ch, esc)
    return value


def _escape_powershell_value(value: str) -> str:
    """Escape characters that are special inside double-quoted PowerShell strings."""
    for ch, esc in (('`', '``'), ('"', '`"'), ("$", "`$")):
        value = value.replace(ch, esc)
    return value


def format_bash_export(values: dict[str, str]) -> str:
    """Format as bash export statements."""
    lines = []
    for key, value in sorted(values.items()):
        lines.append(f'export {key}="{_escape_bash_value(value)}"')
    return "\n".join(lines)


def format_powershell_export(values: dict[str, str]) -> str:
    """Format as PowerShell environment variable assignments."""
    lines = []
    for key, value in sorted(values.items()):
        lines.append(f'$env:{key} = "{_escape_powershell_value(value)}"')
    return "\n".join(lines)


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Read build configuration from .github/build-config.json"
    )
    parser.add_argument(
        "--get", metavar="KEY", help="Get a single value by key (e.g., qt_version)"
    )
    parser.add_argument(
        "--json", action="store_true", help="Output full config as JSON"
    )
    parser.add_argument(
        "--export",
        choices=["bash", "powershell", "sh"],
        help="Output as shell export statements",
    )
    parser.add_argument(
        "--github-output",
        action="store_true",
        help="Write to GITHUB_OUTPUT (GitHub Actions only)",
    )
    parser.add_argument(
        "--config", metavar="FILE", help="Path to config file (overrides auto-detect)"
    )

    args = parser.parse_args()

    try:
        if args.config:
            config_file = Path(args.config)
        else:
            script_dir = Path(__file__).parent
            config_file = find_build_config(
                script_dir,
                extra_candidates=[script_dir / "build-config.json"],
            )

        config = load_build_config(config_file)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    except json.JSONDecodeError as e:
        print(f"Error: Invalid JSON in config file: {e}", file=sys.stderr)
        return 1

    # Handle --get for single value
    if args.get:
        key = args.get.lower()  # Normalize to lowercase
        if key in config:
            print(config[key])
            return 0
        # Legacy alias: gstreamer_version -> gstreamer_default_version
        if key == "gstreamer_version" and "gstreamer_default_version" in config:
            print(config["gstreamer_default_version"])
            return 0
        else:
            print(f"Error: Key '{key}' not found in config", file=sys.stderr)
            print(f"Available keys: {', '.join(sorted(config.keys()))}", file=sys.stderr)
            return 1

    # Handle --json for full config
    if args.json:
        print(json.dumps(config, indent=2))
        return 0

    # Get export values
    values = export_build_config_values(config)

    # Handle --export for shell statements
    if args.export:
        if args.export in ("bash", "sh"):
            print(format_bash_export(values))
        elif args.export == "powershell":
            print(format_powershell_export(values))
        return 0

    # Handle --github-output
    if args.github_output:
        github_output = os.environ.get("GITHUB_OUTPUT")
        github_env = os.environ.get("GITHUB_ENV")

        if not github_output:
            print(
                "Error: GITHUB_OUTPUT not set (not running in GitHub Actions?)",
                file=sys.stderr,
            )
            return 1

        output = format_github_output(values)

        with open(github_output, "a") as f:
            f.write(output + "\n")

        # Also write QT_VERSION to GITHUB_ENV
        if github_env and "QT_VERSION" in values:
            with open(github_env, "a") as f:
                f.write(f"QT_VERSION={values['QT_VERSION']}\n")

        return 0

    # Default: print all values as KEY=VALUE
    print(f"Build Configuration (from {config_file}):")
    for key, value in sorted(values.items()):
        print(f"  {key}={value}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
