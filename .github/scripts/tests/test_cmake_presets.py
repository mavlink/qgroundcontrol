"""Contracts for the repository CMake preset graph."""

from __future__ import annotations

import json
import os
import shutil
import subprocess
from typing import TYPE_CHECKING, Any

from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path

ROOT_PRESETS = REPO_ROOT / "CMakePresets.json"
PRESET_CATEGORIES = {
    "configure": "configurePresets",
    "build": "buildPresets",
    "test": "testPresets",
    "package": "packagePresets",
    "workflow": "workflowPresets",
}


def _load_preset_graph() -> dict[str, dict[str, dict[str, Any]]]:
    graph: dict[str, dict[str, dict[str, Any]]] = {category: {} for category in PRESET_CATEGORIES}
    visited: set[Path] = set()

    def visit(path: Path) -> None:
        path = path.resolve()
        if path in visited:
            return
        visited.add(path)

        document = json.loads(path.read_text(encoding="utf-8"))
        for category, document_key in PRESET_CATEGORIES.items():
            for preset in document.get(document_key, []):
                name = preset["name"]
                assert name not in graph[category], (
                    f"duplicate {category} preset {name!r} in {path.relative_to(REPO_ROOT)}"
                )
                graph[category][name] = preset
        for include in document.get("include", []):
            visit(path.parent / include)

    visit(ROOT_PRESETS)
    return graph


def _inherited_names(preset: dict[str, Any]) -> list[str]:
    inherited = preset.get("inherits", [])
    return [inherited] if isinstance(inherited, str) else inherited


def _effective_configure_cache_value(
    configure_presets: dict[str, dict[str, Any]], name: str, key: str
) -> Any:
    def resolve(preset_name: str, ancestors: frozenset[str]) -> Any:
        assert preset_name not in ancestors, f"configure preset inheritance cycle at {preset_name}"
        ancestors |= {preset_name}
        preset = configure_presets[preset_name]
        cache_variables = preset.get("cacheVariables", {})
        if key in cache_variables:
            return cache_variables[key]
        for parent_name in _inherited_names(preset):
            value = resolve(parent_name, ancestors)
            if value is not None:
                return value
        return None

    return resolve(name, frozenset())


def test_every_visible_configure_preset_has_build_and_workflow_presets() -> None:
    graph = _load_preset_graph()
    visible_configure_names = {
        name for name, preset in graph["configure"].items() if not preset.get("hidden", False)
    }
    visible_build_names = {
        name for name, preset in graph["build"].items() if not preset.get("hidden", False)
    }

    assert visible_build_names == visible_configure_names
    assert set(graph["workflow"]) == visible_configure_names
    for name in visible_configure_names:
        assert graph["build"][name]["configurePreset"] == name


def test_preset_metadata_is_owned_by_the_root_and_builds_stay_in_the_checkout() -> None:
    root_document = json.loads(ROOT_PRESETS.read_text(encoding="utf-8"))
    common_document = json.loads(
        (REPO_ROOT / "cmake/presets/common.json").read_text(encoding="utf-8")
    )
    assert root_document["cmakeMinimumRequired"] == {"major": 3, "minor": 25, "patch": 0}
    assert "cmakeMinimumRequired" not in common_document

    for preset in _load_preset_graph()["configure"].values():
        if not preset.get("hidden", False):
            assert preset["binaryDir"].startswith("${sourceDir}/build")


def test_test_presets_match_configurations_that_build_tests() -> None:
    graph = _load_preset_graph()
    testing_configure_names = {
        name
        for name, preset in graph["configure"].items()
        if not preset.get("hidden", False)
        and _effective_configure_cache_value(graph["configure"], name, "QGC_BUILD_TESTING") == "ON"
    }
    visible_test_names = {
        name for name, preset in graph["test"].items() if not preset.get("hidden", False)
    }

    assert visible_test_names == testing_configure_names
    for name in visible_test_names:
        assert graph["test"][name]["configurePreset"] == name


def test_package_presets_only_cover_cpack_backed_artifacts() -> None:
    graph = _load_preset_graph()
    assert set(graph["package"]) == {"Linux-deb", "Linux-rpm", "Windows", "Windows-arm64"}

    for name, generator in (("Linux-deb", "DEB"), ("Linux-rpm", "RPM")):
        preset = graph["package"][name]
        assert preset["configurePreset"] == name
        assert preset["generators"] == [generator]
        assert preset["configurations"] == ["Release"]
        assert (
            _effective_configure_cache_value(graph["configure"], name, "QGC_CPACK_GENERATOR")
            == generator
        )

    for name in ("Windows", "Windows-arm64"):
        assert graph["package"][name]["configurePreset"] == name
        assert graph["package"][name]["configurations"] == ["Release"]


def test_workflow_steps_use_their_matching_configure_preset() -> None:
    graph = _load_preset_graph()
    for workflow_name, workflow in graph["workflow"].items():
        steps = workflow["steps"]
        assert steps[:2] == [
            {"type": "configure", "name": workflow_name},
            {"type": "build", "name": workflow_name},
        ]

        step_types = [step["type"] for step in steps]
        assert ("test" in step_types) == (workflow_name in graph["test"])
        assert ("package" in step_types) == (workflow_name in graph["package"])
        for step in steps[1:]:
            referenced_preset = graph[step["type"]][step["name"]]
            assert referenced_preset["configurePreset"] == workflow_name


def test_platform_bases_define_the_required_qt_paths() -> None:
    configure_presets = _load_preset_graph()["configure"]
    expected_toolchains = {
        "android-base": "$penv{QT_TARGET_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake",
        "ios-base": "$penv{QT_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake",
        "linux-base": "$penv{QT_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake",
        "macos-base": "$penv{QT_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake",
        "windows-base": "$penv{QT_ROOT_DIR}/lib/cmake/Qt6/qt.toolchain.cmake",
    }
    for name, toolchain in expected_toolchains.items():
        preset = configure_presets[name]
        assert preset["hidden"] is True
        assert preset["generator"] == "Ninja"
        assert preset["toolchainFile"] == toolchain

    android_cache = configure_presets["android-base"]["cacheVariables"]
    assert android_cache["ANDROID_NDK"] == "$penv{ANDROID_NDK}"
    assert android_cache["CMAKE_SYSTEM_VERSION"] == "$penv{ANDROID_MIN_SDK}"
    assert "ANDROID_PLATFORM" not in android_cache
    assert android_cache["CMAKE_PREFIX_PATH"] == "$penv{QT_TARGET_ROOT_DIR}"
    assert android_cache["QT_HOST_PATH"] == "$penv{QT_HOST_PATH}"

    ios_cache = configure_presets["ios-base"]["cacheVariables"]
    assert ios_cache["QT_HOST_PATH"] == "$penv{QT_HOST_PATH}"
    assert ios_cache["Qt6LinguistTools_DIR"] == ("$penv{QT_HOST_PATH}/lib/cmake/Qt6LinguistTools")


def test_android_preset_resolves_minimum_sdk_before_loading_toolchain(tmp_path: Path) -> None:
    source_dir = tmp_path / "source"
    preset_dir = source_dir / "cmake/presets"
    toolchain = tmp_path / "qt/lib/cmake/Qt6/qt.toolchain.cmake"
    preset_dir.mkdir(parents=True)
    toolchain.parent.mkdir(parents=True)

    shutil.copyfile(REPO_ROOT / "cmake/presets/common.json", preset_dir / "common.json")
    shutil.copyfile(REPO_ROOT / "cmake/presets/Android.json", preset_dir / "Android.json")
    (source_dir / "CMakePresets.json").write_text(
        json.dumps(
            {
                "version": 6,
                "cmakeMinimumRequired": {"major": 3, "minor": 25, "patch": 0},
                "include": ["cmake/presets/Android.json"],
            }
        ),
        encoding="utf-8",
    )
    (source_dir / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\nproject(AndroidPresetContract NONE)\n",
        encoding="utf-8",
    )
    toolchain.write_text(
        'if(NOT CMAKE_SYSTEM_VERSION STREQUAL "28")\n'
        '  message(FATAL_ERROR "Android API level was ${CMAKE_SYSTEM_VERSION}")\n'
        "endif()\n"
        "set(CMAKE_SYSTEM_NAME Generic)\n",
        encoding="utf-8",
    )

    env = os.environ.copy()
    env.update(
        {
            "ANDROID_MIN_SDK": "28",
            "ANDROID_NDK": str(tmp_path / "ndk"),
            "QT_HOST_PATH": str(tmp_path / "qt-host"),
            "QT_TARGET_ROOT_DIR": str(tmp_path / "qt"),
        }
    )
    subprocess.run(
        ["cmake", "--preset", "Android", "-S", str(source_dir), "-B", str(tmp_path / "build")],
        check=True,
        cwd=source_dir,
        env=env,
        capture_output=True,
        text=True,
    )


def test_repository_build_entrypoints_use_presets() -> None:
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    configure_tool = (REPO_ROOT / "tools/configure.py").read_text(encoding="utf-8")
    coverage_tool = (REPO_ROOT / "tools/coverage.py").read_text(encoding="utf-8")
    docker_entrypoint = (REPO_ROOT / "deploy/docker/entrypoint.sh").read_text(encoding="utf-8")
    multipass_builder = (REPO_ROOT / "deploy/multipass/build-in-vm.sh").read_text(encoding="utf-8")
    vagrant_builder = (REPO_ROOT / "deploy/vagrant/Vagrantfile").read_text(encoding="utf-8")
    vm_workflow = (REPO_ROOT / ".github/workflows/vm-builds.yml").read_text(encoding="utf-8")

    assert "cmake --build --preset {{build_preset}}" in justfile
    assert "ctest --preset default" in justfile
    assert '"--preset",' in configure_tool
    assert "preset = select_preset(config)" in configure_tool
    assert 'preset="Linux-coverage"' in coverage_tool
    assert 'cmake --preset "${CMAKE_PRESET}"' in docker_entrypoint
    docker_lines = docker_entrypoint.splitlines()
    direct_qt_cmake_lines = [
        index
        for index, line in enumerate(docker_lines)
        if "qt-cmake" in line and "--preset" not in line and not line.lstrip().startswith("#")
    ]
    assert direct_qt_cmake_lines
    for index in direct_qt_cmake_lines:
        assert any("preset-exception:" in line for line in docker_lines[max(0, index - 3) : index])
    assert 'qt-cmake" --preset "${CMAKE_PRESET}"' in multipass_builder
    assert "cmake --preset Linux" in vagrant_builder
    assert "- 'CMakePresets.json'" in vm_workflow
    assert "- 'cmake/presets/**'" in vm_workflow
