"""Regression checks for consumed configuration and source archive inputs."""

from __future__ import annotations

import copy
import json
import tarfile
from typing import TYPE_CHECKING

if TYPE_CHECKING:
    from pathlib import Path

import pytest
from validate_configs import validate_build_config

from ._helpers import (
    REPO_ROOT,
    commit_files,
    get_step,
    get_steps,
    git,
    load_action,
    load_workflow,
)


def configuration():
    return (
        json.loads((REPO_ROOT / ".github/build-config.json").read_text()),
        json.loads((REPO_ROOT / ".github/build-config.schema.json").read_text()),
    )


def test_current_configuration():
    assert validate_build_config(*configuration()) == []


@pytest.mark.parametrize(
    "section,key",
    [
        ("apple", "macos_deployment_target"),
        ("android", "cmdline_tools"),
        ("qt", "modules"),
        ("tools", "grype"),
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


@pytest.mark.parametrize(
    "key,version,error",
    [
        ("ios", "1.26.0", "gstreamer.version.ios must be at least 1.28.0"),
        (
            "minimum",
            "1.28.0",
            "gstreamer.version.minimum must stay below 1.28.0 for system GStreamer",
        ),
    ],
)
def test_gstreamer_version_floors(key, version, error):
    config, schema = configuration()
    config = copy.deepcopy(config)
    config["gstreamer"]["version"][key] = version
    assert error in validate_build_config(config, schema)


def test_archive_preserves_build_and_license_inputs(git_repo: Path):
    paths = [
        ".github/build-config.json",
        ".github/build-config.schema.json",
        ".github/COPYING.md",
        "test/CMakeLists.txt",
    ]
    files = dict.fromkeys([*paths, ".github/workflows/private.yml"], "fixture\n")
    files[".gitattributes"] = (REPO_ROOT / ".gitattributes").read_text()
    commit_files(git_repo, files, "fixture")
    git(git_repo, "archive", "--output", "source.tar", "HEAD")
    with tarfile.open(git_repo / "source.tar") as archive:
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


def test_link_exception_deadlines_come_from_the_config_lychee_uses(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    import validate_configs

    config = "tools/configs/lychee.toml"
    steps = get_steps(load_workflow("check-links.yml"), "check-links")
    assert f"--config {config} " in get_step(steps, "Check links")["with"]["args"]
    restore = get_step(steps, "Restore link cache")
    assert f"hashFiles('{config}'" in restore["with"]["key"]
    checked: list[str] = []
    monkeypatch.setattr(
        validate_configs,
        "validate_exception_dates",
        lambda text, _today: checked.append(text) or [],
    )
    assert validate_configs.main([]) == 0
    assert checked == [(REPO_ROOT / config).read_text(encoding="utf-8")]


def _uv_pin_tree(tmp_path: Path, required: str, image: str) -> Path:
    (tmp_path / "tools").mkdir()
    (tmp_path / "tools/pyproject.toml").write_text(f'[tool.uv]\nrequired-version = "{required}"\n')
    (tmp_path / "deploy/docker").mkdir(parents=True)
    (tmp_path / "deploy/docker/Dockerfile").write_text(
        f"COPY --from=ghcr.io/astral-sh/uv:{image} /uv /usr/local/bin/uv\n"
    )
    return tmp_path


def test_uv_version_has_one_source():
    from validate_configs import validate_uv_pins

    assert validate_uv_pins(REPO_ROOT) == []
    action = load_action("setup-python")
    (setup_uv,) = (
        step
        for step in action["runs"]["steps"]
        if step.get("uses", "").startswith("astral-sh/setup-uv@")
    )
    assert "version" not in setup_uv["with"]
    assert setup_uv["with"]["version-file"] == "tools/pyproject.toml"
    assert setup_uv["with"]["resolution-strategy"] == "lowest"


@pytest.mark.parametrize(
    "required,image,error",
    [
        (">=0.12.8", "0.13.1", "Dockerfile must copy uv 0.12.8"),
        (">=0.12", "0.12.8", "required-version to '>=X.Y.Z'"),
    ],
)
def test_uv_version_drift_is_rejected(tmp_path: Path, required, image, error):
    from validate_configs import validate_uv_pins

    errors = validate_uv_pins(_uv_pin_tree(tmp_path, required, image))
    assert any(error in message for message in errors), errors


def test_docs_build_triggers_on_its_build_config_input():
    assert (
        "../../.github/build-config.json" in (REPO_ROOT / "docs/.vitepress/config.mjs").read_text()
    )
    workflow = load_workflow("docs.yml")
    for event in ("push", "pull_request"):
        assert ".github/build-config.json" in workflow["on"][event]["paths"]
