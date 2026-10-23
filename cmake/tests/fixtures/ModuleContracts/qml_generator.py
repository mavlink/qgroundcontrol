"""Minimal QML generator used by configure-time CMake contract tests."""

from __future__ import annotations

import argparse
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", type=Path)
    parser.add_argument("--write-output", action="append", default=[])
    parser.add_argument("--list-outputs", action="store_true")
    args = parser.parse_args()

    if args.list_outputs:
        print("\n".join(args.write_output))
        return 0
    for relative_path in args.write_output:
        output = args.output_dir / relative_path
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text("import QtQuick\nItem {}\n", encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
