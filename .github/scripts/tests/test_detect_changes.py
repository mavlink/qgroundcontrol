#!/usr/bin/env python3
"""Tests for detect_changes.py."""

from __future__ import annotations

import pytest

from detect_changes import (
    build_patterns,
    has_relevant_changes,
    workflow_name_for_platform,
)


class TestWorkflowNameForPlatform:
    def test_regular_platform(self) -> None:
        assert workflow_name_for_platform("linux") == "linux"
        assert workflow_name_for_platform("windows") == "windows"
        assert workflow_name_for_platform("macos") == "macos"

    def test_docker_prefix(self) -> None:
        assert workflow_name_for_platform("docker-linux") == "docker"
        assert workflow_name_for_platform("docker-android") == "docker"


class TestBuildPatterns:
    def test_common_patterns_present(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("^src/" in p for p in pattern_strs)
        assert any("CMakeLists" in p for p in pattern_strs)

    def test_platform_workflow_pattern(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("linux\\.yml" in p for p in pattern_strs)

    def test_docker_workflow_pattern(self) -> None:
        patterns = build_patterns("docker-linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("docker\\.yml" in p for p in pattern_strs)

    def test_android_extra_pattern(self) -> None:
        patterns = build_patterns("android")
        pattern_strs = [p.pattern for p in patterns]
        assert any("^android/" in p for p in pattern_strs)

    def test_deploy_pattern_uses_platform(self) -> None:
        patterns = build_patterns("linux")
        pattern_strs = [p.pattern for p in patterns]
        assert any("deploy/linux/" in p for p in pattern_strs)


class TestHasRelevantChanges:
    def test_src_change_triggers_any_platform(self) -> None:
        assert has_relevant_changes(["src/Vehicle.cc"], "linux")
        assert has_relevant_changes(["src/Vehicle.cc"], "windows")
        assert has_relevant_changes(["src/Vehicle.cc"], "android")

    def test_cmakelists_triggers(self) -> None:
        assert has_relevant_changes(["CMakeLists.txt"], "linux")

    def test_cmake_dir_triggers(self) -> None:
        assert has_relevant_changes(["cmake/FindFoo.cmake"], "macos")

    def test_qrc_triggers(self) -> None:
        assert has_relevant_changes(["resources/qgc.qrc"], "linux")

    def test_unrelated_file_does_not_trigger(self) -> None:
        assert not has_relevant_changes(["README.md"], "linux")
        assert not has_relevant_changes(["docs/guide.md"], "windows")

    def test_workflow_file_triggers_own_platform(self) -> None:
        assert has_relevant_changes([".github/workflows/linux.yml"], "linux")
        assert not has_relevant_changes([".github/workflows/linux.yml"], "windows")

    def test_actions_dir_triggers_all(self) -> None:
        assert has_relevant_changes([".github/actions/qt-install/action.yml"], "linux")
        assert has_relevant_changes([".github/actions/qt-install/action.yml"], "android")

    def test_build_config_triggers_all(self) -> None:
        assert has_relevant_changes([".github/build-config.json"], "ios")

    def test_android_dir_only_triggers_android(self) -> None:
        assert has_relevant_changes(["android/AndroidManifest.xml"], "android")
        assert not has_relevant_changes(["android/AndroidManifest.xml"], "linux")

    def test_docker_linux_patterns(self) -> None:
        assert has_relevant_changes(["deploy/docker/Dockerfile-build-ubuntu"], "docker-linux")
        assert has_relevant_changes(["deploy/linux/AppImage.sh"], "docker-linux")
        assert not has_relevant_changes(["deploy/docker/Dockerfile-build-ubuntu"], "linux")

    def test_docker_android_patterns(self) -> None:
        assert has_relevant_changes(["deploy/docker/Dockerfile-build-android"], "docker-android")
        assert has_relevant_changes(["android/build.gradle"], "docker-android")

    def test_setup_patterns_linux(self) -> None:
        assert has_relevant_changes(["tools/setup/install_dependencies.py"], "linux")
        assert has_relevant_changes(["tools/setup/foo-debian.sh"], "linux")
        assert not has_relevant_changes(["tools/setup/foo-windows.ps1"], "linux")

    def test_setup_patterns_windows(self) -> None:
        assert has_relevant_changes(["tools/setup/foo-windows.ps1"], "windows")
        assert not has_relevant_changes(["tools/setup/foo-debian.sh"], "windows")

    def test_setup_patterns_android_matches_all(self) -> None:
        assert has_relevant_changes(["tools/setup/anything.py"], "android")

    def test_setup_patterns_ios(self) -> None:
        assert has_relevant_changes(["tools/setup/foo-ios.sh"], "ios")
        assert has_relevant_changes(["tools/setup/foo-macos.sh"], "ios")
        assert not has_relevant_changes(["tools/setup/foo-debian.sh"], "ios")

    def test_empty_files_list(self) -> None:
        assert not has_relevant_changes([], "linux")

    def test_empty_strings_ignored(self) -> None:
        assert not has_relevant_changes(["", ""], "linux")

    def test_test_dir_triggers(self) -> None:
        assert has_relevant_changes(["test/UnitTest.cc"], "linux")

    def test_deploy_platform_triggers(self) -> None:
        assert has_relevant_changes(["deploy/macos/Info.plist"], "macos")
        assert not has_relevant_changes(["deploy/macos/Info.plist"], "linux")

    def test_scripts_dir_triggers_all(self) -> None:
        assert has_relevant_changes([".github/scripts/foo.py"], "windows")

    def test_docker_workflow_triggers_docker_platforms(self) -> None:
        assert has_relevant_changes([".github/workflows/docker.yml"], "docker-linux")
        assert has_relevant_changes([".github/workflows/docker.yml"], "docker-android")
        assert not has_relevant_changes([".github/workflows/docker.yml"], "linux")
