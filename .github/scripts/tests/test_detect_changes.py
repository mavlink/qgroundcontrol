#!/usr/bin/env python3
"""Behavioral contracts for CI path filtering."""

from __future__ import annotations

from detect_changes import has_relevant_changes, workflow_name_for_platform


def _assert_relevance(
    platform: str, relevant: list[str], irrelevant: list[str] | tuple[()] = ()
) -> None:
    for path in relevant:
        assert has_relevant_changes([path], platform), (platform, path)
    for path in irrelevant:
        assert not has_relevant_changes([path], platform), (platform, path)


def test_workflow_names() -> None:
    expected = {
        "linux": "linux",
        "windows": "windows",
        "macos": "macos",
        "docker-linux": "docker",
        "docker-android": "docker",
    }
    for platform, workflow in expected.items():
        assert workflow_name_for_platform(platform) == workflow


def test_common_changes_trigger_every_platform() -> None:
    paths = [
        "src/Vehicle.cc",
        "CMakeLists.txt",
        "CMakePresets.json",
        "cmake/FindFoo.cmake",
        "resources/qgc.qrc",
        "test/UnitTest.cc",
        ".github/actions/qt-install/action.yml",
        ".github/scripts/foo.py",
        ".github/workflows/_detect-changes.yml",
        ".github/build-config.json",
        "tools/configure.py",
        "tools/run_tests.py",
        "tools/common/proc.py",
    ]
    for platform in ("linux", "windows", "macos", "android", "ios"):
        _assert_relevance(platform, paths)


def test_ci_test_files_do_not_trigger_platform_builds() -> None:
    paths = [
        ".github/scripts/tests/test_detect_changes.py",
        ".github/scripts/tests/test_plan_docker_builds.py",
        "tools/tests/test_gh_actions.py",
    ]
    for platform in (
        "linux",
        "windows",
        "macos",
        "android",
        "ios",
        "custom-build",
        "docker-linux",
        "docker-android",
    ):
        _assert_relevance(platform, [], paths)


def test_docker_ci_inputs_only_trigger_docker_builds() -> None:
    paths = [
        ".github/scripts/docker_helper.py",
        ".github/scripts/plan_docker_builds.py",
        "deploy/docker/_variants.py",
        "deploy/docker/variants.json",
    ]
    for platform in ("docker-linux", "docker-android"):
        _assert_relevance(platform, paths)
    for platform in ("linux", "windows", "macos", "android", "ios", "custom-build"):
        _assert_relevance(platform, [], paths)


def test_platform_specific_changes() -> None:
    cases = {
        "linux": (
            [
                ".github/workflows/linux.yml",
                "deploy/linux/AppImage.sh",
                "tools/setup/install_dependencies/_debian.py",
                "tools/setup/foo-debian.sh",
            ],
            ["android/AndroidManifest.xml", "tools/setup/foo-windows.ps1"],
        ),
        "windows": (
            [".github/workflows/windows.yml", "tools/setup/foo-windows.ps1"],
            [".github/workflows/linux.yml", "tools/setup/foo-debian.sh"],
        ),
        "macos": (
            ["deploy/macos/Info.plist", "tools/setup/foo-macos.sh"],
            ["deploy/linux/AppImage.sh"],
        ),
        "ios": (
            ["tools/setup/foo-ios.sh", "tools/setup/foo-macos.sh"],
            ["tools/setup/foo-debian.sh"],
        ),
        "android": (
            ["android/AndroidManifest.xml", "tools/setup/anything.py"],
            ["deploy/linux/AppImage.sh"],
        ),
        "docker-linux": (
            [
                ".github/workflows/docker.yml",
                "deploy/docker/Dockerfile",
                "deploy/docker/entrypoint.sh",
                "deploy/docker/install-sysroot-aarch64.sh",
                "deploy/docker/_docker-exec.sh",
                "deploy/docker/lib/retry.sh",
                "deploy/linux/AppImage.sh",
            ],
            [".github/workflows/linux.yml"],
        ),
        "docker-android": (
            [
                ".github/workflows/docker.yml",
                "deploy/docker/Dockerfile",
                "deploy/docker/lib/build-type.sh",
                "android/build.gradle",
            ],
            [".github/workflows/android.yml"],
        ),
    }
    for platform, (relevant, irrelevant) in cases.items():
        _assert_relevance(platform, relevant, irrelevant)


def test_empty_and_documentation_only_changes_are_ignored() -> None:
    assert not has_relevant_changes([], "linux")
    assert not has_relevant_changes(["", "README.md", "docs/guide.md"], "linux")
