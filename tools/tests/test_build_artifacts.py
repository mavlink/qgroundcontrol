"""Tests for tools/qgc_tools/build_artifacts.py."""

from __future__ import annotations

import pytest
from qgc_tools.build_artifacts import (
    BUILDS_URL,
    PUBLISHED_BUILDS,
    is_distributable_artifact,
    published_build_url,
)


@pytest.mark.parametrize(
    ("name", "expected"),
    [
        ("QGroundControl-x86_64", True),
        ("QGroundControl-linux-arm64-v8a-armeabi-v7a-aab", True),
        ("Custom-QGroundControl", True),
        ("qgc-binary-linux", True),
        ("coverage-report", False),
        ("test-results-linux-coverage-unit", False),
        ("emulator-diagnostics-34", False),
        ("sbom-QGroundControl-x86_64", False),
        ("build-profile-abc", False),
    ],
)
def test_distributable_artifacts_are_packages(name: str, expected: bool) -> None:
    assert is_distributable_artifact(name) is expected


def test_published_builds_have_unique_public_urls() -> None:
    names = [build.name for build in PUBLISHED_BUILDS]
    assert len(names) == len(set(names))
    assert published_build_url("master", "QGroundControl.dmg") == (
        "https://d176tv9ibo4jno.cloudfront.net/builds/master/QGroundControl.dmg"
    )
    assert BUILDS_URL.startswith("https://")
