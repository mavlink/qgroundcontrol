"""Contracts for the repository CMake preset graph."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from typing import TYPE_CHECKING, Any

import pytest
import yaml
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
CI_PLATFORM_WORKFLOWS = (
    "analysis.yml",
    "android.yml",
    "build-profile.yml",
    "custom-build.yml",
    "ios.yml",
    "linux.yml",
    "macos.yml",
    "windows.yml",
)
CMAKE_CONFIGURE_ACTION = REPO_ROOT / ".github" / "actions" / "cmake-configure" / "action.yml"
BUILD_CONFIG = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))


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
    major, minor = (int(part) for part in BUILD_CONFIG["build"]["cmake_minimum_version"].split("."))
    assert root_document["cmakeMinimumRequired"] == {"major": major, "minor": minor, "patch": 0}
    assert "cmakeMinimumRequired" not in common_document

    cmake_lists = (REPO_ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
    match = re.search(r"cmake_minimum_required\(VERSION ([0-9]+\.[0-9]+)\)", cmake_lists)
    assert match is not None
    assert match.group(1) == BUILD_CONFIG["build"]["cmake_minimum_version"]

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
    assert ios_cache["Qt6LinguistTools_DIR"] == "$penv{QT_HOST_PATH}/lib/cmake/Qt6LinguistTools"
    assert ios_cache["CMAKE_SYSTEM_NAME"] == "iOS"
    assert "CMAKE_OSX_DEPLOYMENT_TARGET" not in ios_cache


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
        }
    )
    subprocess.run(
        [
            "cmake",
            "--preset",
            "Android",
            "-S",
            str(source_dir),
            "-B",
            str(tmp_path / "build"),
            f"-DCMAKE_TOOLCHAIN_FILE={toolchain}",
            f"-DCMAKE_PREFIX_PATH={tmp_path / 'qt'}",
        ],
        check=True,
        cwd=source_dir,
        env=env,
        capture_output=True,
        text=True,
    )


def test_developer_build_entrypoints_use_presets() -> None:
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    configure_tool = (REPO_ROOT / "tools/configure.py").read_text(encoding="utf-8")
    vscode_tasks = (REPO_ROOT / ".vscode/tasks.default.json").read_text(encoding="utf-8")
    vscode_settings = (REPO_ROOT / ".vscode/settings.default.json").read_text(encoding="utf-8")
    ci_scripts = (REPO_ROOT / ".github/workflows/ci-scripts.yml").read_text(encoding="utf-8")

    assert "cmake --build --preset {{ build_preset }}" in justfile
    assert "ctest --preset default" in justfile
    assert "--parallel {{ jobs }} --no-tests=error" in justfile
    assert "host_os := os()" in justfile
    assert 'python := if host_os == "windows"' in justfile
    assert "msvc2022" not in justfile
    assert 'qt_root_arg := if qt_dir == ""' in justfile
    assert 'app_path := if host_os == "windows"' in justfile
    assert '"--preset",' in configure_tool
    assert "preset = select_preset(config)" in configure_tool
    assert "qt-cmake not found; pass --no-qt-cmake" in configure_tool
    assert '"--preset"' in vscode_tasks
    assert '"python.defaultInterpreterPath": "${workspaceFolder}/.venv"' in vscode_settings
    for path in ("CMakePresets.json", "cmake/presets", "justfile", ".vscode"):
        assert path in ci_scripts

    multipass = REPO_ROOT / "deploy" / "multipass" / "build-in-vm.sh"
    if multipass.exists():
        assert '"${QT_ROOT}/bin/qt-cmake"' in multipass.read_text(encoding="utf-8")


def test_host_tools_use_host_platform_predicates() -> None:
    python_venv = REPO_ROOT / "cmake" / "modules" / "PythonVenv.cmake"
    toolchain = REPO_ROOT / "cmake" / "Toolchain.cmake"
    helpers = REPO_ROOT / "cmake" / "Helpers.cmake"
    if not all(path.exists() for path in (python_venv, toolchain, helpers)):
        pytest.skip("host-platform CMake modules not in checkout")

    assert "if(CMAKE_HOST_WIN32)" in python_venv.read_text(encoding="utf-8")
    assert "NOT CMAKE_HOST_WIN32" in toolchain.read_text(encoding="utf-8")
    assert "if(CMAKE_HOST_WIN32)" in helpers.read_text(encoding="utf-8")


def test_windows_qt_architectures_use_one_msvc_generation() -> None:
    sources = (
        REPO_ROOT / ".github/scripts/android_matrix.py",
        REPO_ROOT / ".github/workflows/build-gstreamer.yml",
        REPO_ROOT / ".github/workflows/windows.yml",
    )
    generations = {
        generation
        for source in sources
        for generation in re.findall(r"win64_msvc([0-9]+)", source.read_text(encoding="utf-8"))
    }
    assert len(generations) == 1, f"Windows Qt MSVC generations differ: {sorted(generations)}"


def test_ci_configure_steps_select_platform_presets() -> None:
    def visit(value: Any, workflow: str) -> None:
        if isinstance(value, dict):
            if value.get("uses") == "./.github/actions/cmake-configure":
                assert value.get("with", {}).get("preset"), f"{workflow} omits configure preset"
                use_qt_cmake = value.get("with", {}).get("use-qt-cmake", "true")
                if workflow == "android.yml":
                    assert use_qt_cmake == "false"
                    extra_args = value.get("with", {}).get("extra-args", "")
                    assert "-DCMAKE_TOOLCHAIN_FILE=" in extra_args
                    assert "-DCMAKE_PREFIX_PATH=" in extra_args
                else:
                    assert use_qt_cmake != "false", f"{workflow} disables qt-cmake"
            for child in value.values():
                visit(child, workflow)
        elif isinstance(value, list):
            for child in value:
                visit(child, workflow)

    for workflow in CI_PLATFORM_WORKFLOWS:
        document = yaml.safe_load(
            (REPO_ROOT / ".github" / "workflows" / workflow).read_text(encoding="utf-8")
        )
        visit(document, workflow)

    action = yaml.safe_load(CMAKE_CONFIGURE_ACTION.read_text(encoding="utf-8"))
    assert action["inputs"]["use-qt-cmake"]["default"] == "true"
