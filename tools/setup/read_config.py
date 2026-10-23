#!/usr/bin/env python3
"""
Read build configuration from .github/build-config.json

This script provides cross-platform access to the centralized build config.
It can be used directly or called by shell wrappers for environment export.

Usage:
    read_config.py                      # Print all config as KEY=VALUE
    read_config.py --get qt.version     # Get single value
    read_config.py --export bash        # Output as bash export statements
    read_config.py --github-output      # Write to GITHUB_OUTPUT (CI only)
    read_config.py --github-output qt.version tools.packer.version
                                        # Write only these, as qt_version and tools_packer_version

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

sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

import _bootstrap as _bootstrap
from common.build_config import (
    export_build_config_values,
    find_build_config,
    github_output_values,
    load_build_config,
    lookup_dotted,
)
from common.gh_actions import append_github_env, write_github_output


def _escape_bash_value(value: str) -> str:
    """Escape characters that are special inside double-quoted bash strings."""
    for ch, esc in (("\\", "\\\\"), ('"', '\\"'), ("$", "\\$"), ("`", "\\`")):
        value = value.replace(ch, esc)
    return value


def format_bash_export(values: dict[str, str]) -> str:
    """Format as bash export statements."""
    return "\n".join(
        f'export {key}="{_escape_bash_value(value)}"' for key, value in sorted(values.items())
    )


def _format_value(value: Any) -> str:
    return str(value) if isinstance(value, (str, int, float, bool)) else json.dumps(value)


def _missing_key(key: str, config: dict[str, Any]) -> int:
    print(f"Error: Key '{key}' not found in config", file=sys.stderr)
    print(f"Available keys: {', '.join(sorted(config.keys()))}", file=sys.stderr)
    return 1


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Read build configuration from .github/build-config.json"
    )
    parser.add_argument("--get", metavar="KEY", help="Get a single value by key (e.g., qt.version)")
    parser.add_argument("--export", choices=["bash"], help="Output as shell export statements")
    parser.add_argument(
        "--github-output",
        nargs="*",
        metavar="KEY",
        help="Write to GITHUB_OUTPUT (GitHub Actions only): the given dotted keys with dots "
        "as underscores, else every exported value",
    )

    args = parser.parse_args()

    try:
        config_file = find_build_config()
        config = load_build_config(config_file)
    except FileNotFoundError as e:
        print(f"Error: {e}", file=sys.stderr)
        return 1
    except (json.JSONDecodeError, ValueError) as e:
        print(f"Error: Invalid JSON in config file: {e}", file=sys.stderr)
        return 1

    if args.get:
        key = args.get.lower()  # Normalize to lowercase
        value: Any = lookup_dotted(config, key, None)
        if value is None:
            return _missing_key(key, config)
        print(_format_value(value))
        return 0

    values = export_build_config_values(config)

    if args.export:
        print(format_bash_export(values))
        return 0

    if args.github_output is not None:
        if not os.environ.get("GITHUB_OUTPUT"):
            print(
                "Error: GITHUB_OUTPUT not set (not running in GitHub Actions?)",
                file=sys.stderr,
            )
            return 1
        if args.github_output:
            outputs = {}
            for key in (key.lower() for key in args.github_output):
                value = lookup_dotted(config, key, None)
                if value is None:
                    return _missing_key(key, config)
                outputs[key.replace(".", "_")] = _format_value(value)
            write_github_output(outputs)
            return 0
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
