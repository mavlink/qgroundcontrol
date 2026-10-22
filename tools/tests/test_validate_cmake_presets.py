"""Tests for tools/validate_cmake_presets.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
import validate_cmake_presets
from validate_cmake_presets import validate_presets

if TYPE_CHECKING:
    from pathlib import Path

SCHEMA = {
    "$schema": "http://json-schema.org/draft-07/schema#",
    "type": "object",
    "required": ["version"],
    "properties": {"version": {"type": "integer"}, "configurePresets": {"type": "array"}},
    "additionalProperties": False,
}


def _write(tmp_path: Path, name: str, document: object) -> Path:
    path = tmp_path / name
    path.write_text(json.dumps(document), encoding="utf-8")
    return path


def test_reports_schema_violations_with_their_location(tmp_path: Path) -> None:
    valid = _write(tmp_path, "valid.json", {"version": 8, "configurePresets": []})
    invalid = _write(tmp_path, "invalid.json", {"version": "8", "unknown": True})

    assert validate_presets([valid], SCHEMA) == []
    errors = validate_presets([invalid], SCHEMA)
    assert len(errors) == 2
    assert any(f"{invalid}: version: '8' is not of type 'integer'" == error for error in errors)
    assert any(error.startswith(f"{invalid}: <root>: Additional properties") for error in errors)


def test_repository_presets_match_the_locked_cmake_schema() -> None:
    try:
        validate_cmake_presets.presets_schema_path()
    except validate_cmake_presets.metadata.PackageNotFoundError:
        pytest.skip("the build group's cmake package is not installed")
    assert validate_cmake_presets.main([]) == 0


def test_missing_cmake_package_fails(monkeypatch: pytest.MonkeyPatch) -> None:
    def missing(_name: str) -> None:
        raise validate_cmake_presets.metadata.PackageNotFoundError("cmake")

    monkeypatch.setattr(validate_cmake_presets.metadata, "files", missing)
    assert validate_cmake_presets.main([]) == 1
