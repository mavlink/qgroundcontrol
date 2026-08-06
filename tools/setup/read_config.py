#!/usr/bin/env python3
"""
Read build configuration from .github/build-config.json

This script provides cross-platform access to the centralized build config.
It can be used directly or called by shell wrappers for environment export.

Usage:
    read_config.py                      # Print all config as KEY=VALUE
    read_config.py --get qt.version     # Get single value
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

_tools_dir = Path(__file__).resolve().parents[1]
if str(_tools_dir) not in sys.path:
    sys.path.insert(0, str(_tools_dir))

from _bootstrap import ensure_tools_dir  # noqa: E402

ensure_tools_dir(__file__)

from common.build_config import (  # noqa: E402
    export_build_config_values,
    find_build_config,
    github_output_values,
    load_build_config,
)
from common.gh_actions import append_github_env, write_github_output  # noqa: E402


def find_config_file() -> Path:
    """Find build-config.json: env override, script-dir (Docker layout), then repo walk."""
    script_dir = Path(__file__).parent
    return find_build_config(start=script_dir, extra_candidates=[script_dir / "build-config.json"])


def _escape_bash_value(value: str) -> str:
    """Escape characters that are special inside double-quoted bash strings."""
    for ch, esc in (("\\", "\\\\"), ('"', '\\"'), ("$", "\\$"), ("`", "\\`")):
        value = value.replace(ch, esc)
    return value


def _escape_powershell_value(value: str) -> str:
    """Escape characters that are special inside double-quoted PowerShell strings."""
    for ch, esc in (("`", "``"), ('"', '`"'), ("$", "`$")):
        value = value.replace(ch, esc)
    return value


def format_bash_export(values: dict[str, str]) -> str:
    """Format as bash export statements."""
    return "\n".join(
        f'export {key}="{_escape_bash_value(value)}"' for key, value in sorted(values.items())
    )


def format_powershell_export(values: dict[str, str]) -> str:
    """Format as PowerShell environment variable assignments."""
    return "\n".join(
        f'$env:{key} = "{_escape_powershell_value(value)}"' for key, value in sorted(values.items())
    )


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Read build configuration from .github/build-config.json"
    )
    parser.add_argument("--get", metavar="KEY", help="Get a single value by key (e.g., qt.version)")
    parser.add_argument("--json", action="store_true", help="Output full config as JSON")
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
        config_file = Path(args.config) if args.config else find_config_file()
        config = load_build_config(config_file)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    except (json.JSONDecodeError, ValueError) as e:
        print(f"Error: Invalid JSON in config file: {e}", file=sys.stderr)
        return 1

    if args.get:
        key = args.get.lower()  # Normalize to lowercase
        if key == "gstreamer_version":
            key = "gstreamer.version.default"
        value: Any = config
        for part in key.split("."):
            if isinstance(value, dict) and part in value:
                value = value[part]
            else:
                value = None
                break
        if value is not None:
            print(value if isinstance(value, (str, int, float, bool)) else json.dumps(value))
            return 0
        print(f"Error: Key '{key}' not found in config", file=sys.stderr)
        print(f"Available keys: {', '.join(sorted(config.keys()))}", file=sys.stderr)
        return 1

    if args.json:
        print(json.dumps(config, indent=2))
        return 0

    values = export_build_config_values(config)

    if args.export:
        if args.export in ("bash", "sh"):
            print(format_bash_export(values))
        elif args.export == "powershell":
            print(format_powershell_export(values))
        return 0

    if args.github_output:
        if not os.environ.get("GITHUB_OUTPUT"):
            print(
                "Error: GITHUB_OUTPUT not set (not running in GitHub Actions?)",
                file=sys.stderr,
            )
            return 1
        write_github_output(github_output_values(values))
        if "QT_VERSION" in values:
            append_github_env({"QT_VERSION": values["QT_VERSION"]})
        return 0

    print(f"Build Configuration (from {config_file}):")
    for key, value in sorted(values.items()):
        print(f"  {key}={value}")

    return 0


if __name__ == "__main__":
    sys.exit(main())
