"""Tests for tools/qgc_tools/build_artifacts.py."""

from __future__ import annotations

import pytest
from qgc_tools.build_artifacts import (
    BUILDS_URL,
    PUBLISHED_BUILDS,
    RETIRED_BUILD_NAMES,
    companion_names,
    is_distributable_artifact,
    is_public_name,
    is_publishable_name,
    published_build_url,
    workflow_packages,
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


def test_every_published_build_has_a_public_name() -> None:
    assert all(is_public_name(build.name) for build in PUBLISHED_BUILDS)
    assert all(is_public_name(name) for name in RETIRED_BUILD_NAMES)


def test_only_names_this_checkout_lists_are_publishable(monkeypatch: pytest.MonkeyPatch) -> None:
    assert all(is_publishable_name(build.name) for build in PUBLISHED_BUILDS)
    # A package file name by its form, but not one that is listed.
    cross = "QGroundControl-installer-AMD64-ARM64.exe"
    assert is_public_name(cross)
    assert not is_publishable_name(cross)
    assert not is_publishable_name("../QGroundControl.dmg")

    # A name the workflows here no longer mark stays publishable for the branches that do,
    # as long as it is a package file name.
    retired = frozenset({cross, "../QGroundControl.dmg"})
    monkeypatch.setattr("qgc_tools.build_artifacts.RETIRED_BUILD_NAMES", retired)
    assert is_publishable_name(cross)
    assert not is_publishable_name("../QGroundControl.dmg")


def test_a_retired_name_is_one_that_is_no_longer_published() -> None:
    assert not RETIRED_BUILD_NAMES & {build.name for build in PUBLISHED_BUILDS}


def test_a_workflow_s_packages_are_the_published_builds_it_builds() -> None:
    assert workflow_packages("Linux") == [
        "QGroundControl-aarch64.AppImage",
        "QGroundControl-x86_64.AppImage",
    ]
    assert workflow_packages("MacOS") == ["QGroundControl.dmg"]
    assert workflow_packages("iOS") == []
    workflows = ("Linux", "Windows", "MacOS", "Android")
    names = [name for workflow in workflows for name in workflow_packages(workflow)]
    assert sorted(names) == sorted(build.name for build in PUBLISHED_BUILDS)


def test_only_an_appimage_with_a_public_name_has_a_companion() -> None:
    appimage = "QGroundControl-x86_64.AppImage"
    assert companion_names(appimage) == (f"{appimage}.zsync",)
    assert companion_names("QGroundControl.dmg") == ()
    # An AppImage by its suffix, but not a package file name.
    assert not is_public_name("QGroundControl x86_64.AppImage")
    assert companion_names("QGroundControl x86_64.AppImage") == ()


@pytest.mark.parametrize(
    "name",
    [
        "",
        "../QGroundControl.dmg",
        "builds/QGroundControl.dmg",
        "QGroundControl.dmg/..",
        "..\\QGroundControl.exe",
        "QGroundControl..dmg",
        "QGroundControl-.dmg",
        "QGroundControl.dmg\n",
        "QGroundControl x86_64.AppImage",
        "QGroundControl.dmg.sha256",
        "QGroundControl.build.json",
        "QGroundControl.sh",
        "qgroundcontrol.dmg",
        "Custom-QGroundControl.dmg",
        "published.json",
        "QGroundControl-\u00e9.dmg",
    ],
)
def test_names_that_are_not_a_package_are_never_public(name: str) -> None:
    assert not is_public_name(name)
