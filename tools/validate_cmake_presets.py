"""Validate CMake preset files against CMake's presets schema.

Usage:
    python tools/validate_cmake_presets.py [FILE...]

Without files, checks CMakePresets.json and cmake/presets/*.json. The schema ships with the `build`
group's cmake package, which tools/uv.lock pins, so the check works offline.
"""

from __future__ import annotations

import argparse
import json
from importlib import metadata
from pathlib import Path
from typing import Any

import _bootstrap as _bootstrap
from common.logging import log_error, log_ok
from jsonschema import validators

ROOT = Path(__file__).resolve().parents[1]
SCHEMA_SUFFIX = "Help/manual/presets/schema.json"


def presets_schema_path() -> Path:
    """Return the presets schema installed with the cmake Python package."""
    for file in metadata.files("cmake") or ():
        if str(file).endswith(SCHEMA_SUFFIX):
            return Path(str(file.locate()))
    raise FileNotFoundError(f"cmake package has no {SCHEMA_SUFFIX}")


def validate_presets(files: list[Path], schema: dict[str, Any]) -> list[str]:
    validator_class = validators.validator_for(schema)
    validator_class.check_schema(schema)
    validator = validator_class(schema)
    errors: list[str] = []
    for file in files:
        document = json.loads(file.read_text(encoding="utf-8"))
        for error in validator.iter_errors(document):
            location = "/".join(map(str, error.absolute_path)) or "<root>"
            errors.append(f"{file}: {location}: {error.message}")
    return errors


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Validate CMake preset files against CMake's schema."
    )
    parser.add_argument("files", nargs="*", type=Path)
    args = parser.parse_args(argv)
    files = args.files or [
        ROOT / "CMakePresets.json",
        *sorted((ROOT / "cmake/presets").glob("*.json")),
    ]
    try:
        schema_path = presets_schema_path()
    except (metadata.PackageNotFoundError, FileNotFoundError) as error:
        log_error(f"{error}; run with `uv run --frozen --project tools --group lint --group build`")
        return 1
    errors = validate_presets(files, json.loads(schema_path.read_text(encoding="utf-8")))
    for error in errors:
        log_error(error)
    if not errors:
        log_ok(f"{len(files)} preset files match the {schema_path.parents[3].name} presets schema")
    return int(bool(errors))


if __name__ == "__main__":
    raise SystemExit(main())
