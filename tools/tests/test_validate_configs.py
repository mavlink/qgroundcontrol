"""Regression checks for consumed configuration and source archive inputs."""

from __future__ import annotations

import copy
import io
import json
import subprocess
import tarfile
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pathlib import Path

import pytest
from validate_configs import ROOT, validate_build_config


def configuration():
    return (
        json.loads((ROOT / ".github/build-config.json").read_text()),
        json.loads((ROOT / ".github/build-config.schema.json").read_text()),
    )


def test_current_configuration():
    assert validate_build_config(*configuration()) == []


@pytest.mark.parametrize(
    "section,key",
    [
        ("apple", "macos_deployment_target"),
        ("android", "cmdline_tools"),
        ("qt", "modules"),
    ],
)
def test_consumed_fields_are_required(section, key):
    config, schema = configuration()
    del config[section][key]
    assert validate_build_config(config, schema)


@pytest.mark.parametrize("mutation", ["qt_minimum", "sdk_order", "gst_checksum", "gst_platform"])
def test_inconsistent_versions_are_rejected(mutation):
    config, schema = configuration()
    config = copy.deepcopy(config)
    if mutation == "qt_minimum":
        config["qt"]["minimum_version"] = "99.0.0"
    elif mutation == "sdk_order":
        config["android"]["min_sdk"] = "99"
    elif mutation == "gst_checksum":
        config["gstreamer"]["version"]["android"] = "1.99.0"
    else:
        del config["gstreamer"]["version"]["windows"]
    assert validate_build_config(config, schema)


def test_archive_preserves_build_and_license_inputs(tmp_path: Path):
    paths = [
        ".github/build-config.json",
        ".github/build-config.schema.json",
        ".github/COPYING.md",
        "test/CMakeLists.txt",
    ]
    (tmp_path / ".gitattributes").write_text((ROOT / ".gitattributes").read_text())
    for name in [*paths, ".github/workflows/private.yml"]:
        path = tmp_path / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text("fixture\n")

    def git(*args):
        return subprocess.run(["git", "-C", str(tmp_path), *args], check=True, capture_output=True)

    git("init", "-q")
    git("add", ".")
    git("-c", "user.name=Test", "-c", "user.email=test@example.invalid", "commit", "-qm", "fixture")
    with tarfile.open(fileobj=io.BytesIO(git("archive", "HEAD").stdout)) as archive:
        names = archive.getnames()
    assert set(paths) <= set(names)
    assert ".github/workflows/private.yml" not in names


def test_link_exceptions_expire():
    from datetime import date

    from validate_configs import validate_exception_dates

    text = "# Expires: 2026-12-01. Legacy links"
    assert not validate_exception_dates(text, date(2026, 11, 30))
    assert validate_exception_dates(text, date(2026, 12, 1))
    assert validate_exception_dates("no expiry", date(2026, 12, 1))


def test_quality_tool_pins_agree():
    from validate_configs import validate_tool_pins

    assert validate_tool_pins(ROOT) == []
