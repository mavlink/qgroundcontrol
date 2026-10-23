"""Contracts for the repository CMake preset graph."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from typing import TYPE_CHECKING, Any

import pytest

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow

if TYPE_CHECKING:
    from pathlib import Path

ROOT_PRESETS = REPO_ROOT / "CMakePresets.json"
PRESETS_VERSION = 8
PRESETS_SCHEMA = (
    "https://cmake.org/cmake/help/latest/_downloads/3e2d73bff478d88a7de0de736ba5e361/schema.json"
)
# First CMake release reading each presets schema version (cmake-presets(7) "Versions").
PRESETS_VERSION_MIN_CMAKE = {6: (3, 25), 7: (3, 27), 8: (3, 28), 9: (3, 30), 10: (3, 31)}
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
    "ios.yml",
    "linux.yml",
    "macos.yml",
    "windows.yml",
)
BUILD_CONFIG = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
LEAN_DEBUG_CACHE = {
    "CMAKE_C_FLAGS_DEBUG": "-g1",
    "CMAKE_CXX_FLAGS_DEBUG": "-g1",
    "QGC_UNITY_BUILD": "ON",
}
# Cache values the CI validation jobs configure; the presets are their only source.
CI_VALIDATION_PRESET_CACHE = {
    "Linux-coverage": {
        **LEAN_DEBUG_CACHE,
        "CMAKE_BUILD_TYPE": "Debug",
        "QGC_BUILD_TESTING": "ON",
        "QGC_ENABLE_COVERAGE": "ON",
    },
    "Linux-sanitizers": {
        **LEAN_DEBUG_CACHE,
        "CMAKE_BUILD_TYPE": "Debug",
        "QGC_BUILD_TESTING": "ON",
        "QGC_ENABLE_ASAN": "ON",
        "QGC_ENABLE_UBSAN": "ON",
        "QGC_TEST_DETECT_LEAKS": "ON",
    },
    "iOS-simulator": {
        **LEAN_DEBUG_CACHE,
        "CMAKE_BUILD_TYPE": "Debug",
        "QGC_BUILD_TESTING": "OFF",
        "CMAKE_SYSTEM_NAME": "iOS",
        "CMAKE_OSX_SYSROOT": "iphonesimulator",
        "CMAKE_OSX_ARCHITECTURES": "x86_64",
        "CMAKE_OBJC_FLAGS_DEBUG": "-g1",
        "CMAKE_OBJCXX_FLAGS_DEBUG": "-g1",
    },
}


def _preset_documents() -> dict[Path, dict[str, Any]]:
    documents: dict[Path, dict[str, Any]] = {}

    def visit(path: Path) -> None:
        path = path.resolve()
        if path in documents:
            return
        documents[path] = json.loads(path.read_text(encoding="utf-8"))
        for include in documents[path].get("include", []):
            visit(path.parent / include)

    visit(ROOT_PRESETS)
    return documents


def _load_preset_graph() -> dict[str, dict[str, dict[str, Any]]]:
    graph: dict[str, dict[str, dict[str, Any]]] = {category: {} for category in PRESET_CATEGORIES}
    for path, document in _preset_documents().items():
        for category, document_key in PRESET_CATEGORIES.items():
            for preset in document.get(document_key, []):
                name = preset["name"]
                assert name not in graph[category], (
                    f"duplicate {category} preset {name!r} in {path.relative_to(REPO_ROOT)}"
                )
                graph[category][name] = preset
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


def test_every_preset_file_declares_the_schema_supported_by_the_minimum_cmake() -> None:
    minimum = tuple(int(part) for part in BUILD_CONFIG["build"]["cmake_minimum_version"].split("."))
    assert minimum >= PRESETS_VERSION_MIN_CMAKE[PRESETS_VERSION]
    documents = _preset_documents()
    assert len(documents) == 1 + len(list((REPO_ROOT / "cmake/presets").glob("*.json")))
    for path, document in documents.items():
        name = path.relative_to(REPO_ROOT)
        assert document["version"] == PRESETS_VERSION, name
        assert document["$schema"] == PRESETS_SCHEMA, name


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


def test_macos_local_builds_default_to_host_architecture(tmp_path: Path) -> None:
    configure_presets = _load_preset_graph()["configure"]
    source_dir = tmp_path / "source"
    (source_dir / ".github").mkdir(parents=True)
    shutil.copyfile(
        REPO_ROOT / ".github/build-config.json", source_dir / ".github/build-config.json"
    )
    (source_dir / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.25)\n"
        f'list(APPEND CMAKE_MODULE_PATH "{(REPO_ROOT / "cmake").as_posix()}" '
        f'"{(REPO_ROOT / "cmake/modules").as_posix()}")\n'
        "include(CustomOptions)\n"
        "if(QGC_MACOS_UNIVERSAL_BUILD)\n"
        '  message(FATAL_ERROR "Local macOS builds must default to the host architecture")\n'
        "endif()\n"
        "project(MacOSNativeDefault NONE)\n",
        encoding="utf-8",
    )
    subprocess.run(
        ["cmake", "-S", str(source_dir), "-B", str(tmp_path / "build")],
        check=True,
        capture_output=True,
        text=True,
    )

    for name in ("macOS", "macOS-debug"):
        assert (
            _effective_configure_cache_value(configure_presets, name, "QGC_MACOS_UNIVERSAL_BUILD")
            == "OFF"
        )

    for name in (
        "default",
        "default-release",
        "default-relwithdebinfo",
        "default-minsizerel",
        "macOS",
        "macOS-debug",
    ):
        assert (
            _effective_configure_cache_value(configure_presets, name, "CMAKE_OSX_ARCHITECTURES")
            is None
        )


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
                **json.loads(ROOT_PRESETS.read_text(encoding="utf-8")),
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
    vscode_tasks = (REPO_ROOT / ".vscode/tasks.default.json").read_text(encoding="utf-8")
    vscode_settings = (REPO_ROOT / ".vscode/settings.default.json").read_text(encoding="utf-8")

    assert "cmake --build --preset {{ build_preset }}" in justfile
    assert "ctest --preset default" in justfile
    assert "--parallel {{ jobs }} --no-tests=error" in justfile
    assert "msvc2022" not in justfile
    assert '"--preset"' in vscode_tasks
    assert '"python.defaultInterpreterPath": "${workspaceFolder}/tools/.venv"' in vscode_settings
    checkout = get_step(get_steps(load_workflow("ci-scripts.yml"), "test-ci-scripts"), "Checkout")
    assert {"CMakePresets.json", "cmake", "justfile", ".vscode"} <= set(
        checkout["with"]["sparse-checkout"].split()
    )


def test_justfile_runs_tools_from_the_lock_and_resolves_host_paths() -> None:
    justfile = (REPO_ROOT / "justfile").read_text(encoding="utf-8")
    assert 'python := "uv run --frozen --project tools --group scripts python"' in justfile
    assert "host_os := os()" in justfile
    assert 'qt_root_arg := if qt_dir == ""' in justfile
    assert 'app_path := if host_os == "windows"' in justfile

    vagrant = REPO_ROOT / "deploy" / "vagrant" / "Vagrantfile"
    assert "deploy/vagrant/provision.py" in vagrant.read_text(encoding="utf-8")


def test_host_tools_use_host_platform_predicates() -> None:
    python_venv = REPO_ROOT / "cmake" / "modules" / "PythonVenv.cmake"
    toolchain = REPO_ROOT / "cmake" / "Toolchain.cmake"
    helpers = REPO_ROOT / "cmake" / "Helpers.cmake"

    assert "if(CMAKE_HOST_WIN32)" in python_venv.read_text(encoding="utf-8")
    assert "NOT CMAKE_HOST_WIN32" in toolchain.read_text(encoding="utf-8")
    assert "if(CMAKE_HOST_WIN32)" in helpers.read_text(encoding="utf-8")


def test_windows_qt_architectures_use_one_msvc_generation() -> None:
    sources = (
        REPO_ROOT / "tools/ci/planning/android_matrix.py",
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
                if workflow == "macos.yml":
                    extra_args = value.get("with", {}).get("extra-args", "")
                    # The shipped Release build is universal; the Debug test leg builds for the runner.
                    assert (
                        "-DQGC_MACOS_UNIVERSAL_BUILD=${{ matrix.build_type == 'Release' && 'ON' || 'OFF' }}"
                        in extra_args
                    )
            for child in value.values():
                visit(child, workflow)
        elif isinstance(value, list):
            for child in value:
                visit(child, workflow)

    for workflow in CI_PLATFORM_WORKFLOWS:
        visit(load_workflow(workflow), workflow)

    action = load_action("cmake-configure")
    assert action["inputs"]["use-qt-cmake"]["default"] == "true"


@pytest.mark.parametrize("platform", ["windows", "macos"])
def test_portable_tests_share_the_platform_build(platform: str) -> None:
    workflow = load_workflow(f"{platform}.yml")
    steps = workflow["jobs"]["build"]["steps"]
    configure = next(
        step for step in steps if step.get("uses") == "./.github/actions/cmake-configure"
    )
    assert "QGC_BUILD_PORTABLE_TESTS=" in configure["with"]["extra-args"]
    portable = next(step for step in steps if step.get("id") == "portable")
    assert portable["with"]["include-labels"] == "Portable"
    assert "steps.build.outcome == 'success'" in portable["if"]
    if platform == "windows":
        assert "matrix.variant == 'amd64'" in portable["if"]


@pytest.mark.parametrize("name", sorted(CI_VALIDATION_PRESET_CACHE))
def test_ci_validation_presets_carry_the_ci_cache(name: str) -> None:
    configure_presets = _load_preset_graph()["configure"]
    for key, value in CI_VALIDATION_PRESET_CACHE[name].items():
        assert _effective_configure_cache_value(configure_presets, name, key) == value, key


def test_developer_debug_presets_keep_full_debug_info_and_per_file_builds() -> None:
    configure_presets = _load_preset_graph()["configure"]
    for name in (
        "default",
        "Linux-debug",
        "Linux-clang-debug",
        "macOS-debug",
        "Windows-debug",
        "iOS-debug",
        "Android-debug",
    ):
        for key in LEAN_DEBUG_CACHE:
            assert _effective_configure_cache_value(configure_presets, name, key) is None, (
                name,
                key,
            )


def test_ci_validation_jobs_take_cmake_settings_from_presets() -> None:
    def configure_inputs(steps: list[dict[str, Any]]) -> dict[str, str]:
        return next(
            step for step in steps if step.get("uses") == "./.github/actions/cmake-configure"
        )["with"]

    linux = load_workflow("linux.yml")
    steps = linux["jobs"]["debug-validation"]["steps"]
    configure = configure_inputs(steps)
    assert configure["preset"] == (
        "${{ matrix.mode == 'coverage' && 'Linux-coverage' || 'Linux-sanitizers' }}"
    )
    assert "extra-args" not in configure
    tests = next(step for step in steps if step.get("uses") == "./.github/actions/test-phase")
    assert not [name for name in tests.get("env", {}) if name.endswith("SAN_OPTIONS")]
    sanitizer_exclusions = _load_preset_graph()["test"]["Linux-sanitizers"]["filter"]["exclude"]
    assert f"'{sanitizer_exclusions['label']}'" in tests["with"]["exclude-labels"]

    ios = load_workflow("ios.yml")
    job = ios["jobs"]["build"]
    simulator = next(
        leg for leg in job["strategy"]["matrix"]["include"] if leg["target"] == "simulator"
    )
    assert simulator["preset"] == "iOS-simulator"
    extra_args = configure_inputs(job["steps"]).get("extra-args", "")
    for flag in ("CMAKE_OSX_", "_FLAGS_DEBUG", "QGC_UNITY_BUILD"):
        assert flag not in extra_args


def test_sanitizer_preset_supplies_ci_runtime_options_to_every_test(tmp_path: Path) -> None:
    configure_presets = _load_preset_graph()["configure"]
    sanitizer_cache = {
        key: _effective_configure_cache_value(configure_presets, "Linux-sanitizers", key)
        for key in ("QGC_ENABLE_ASAN", "QGC_ENABLE_UBSAN", "QGC_TEST_DETECT_LEAKS")
    }
    source_dir = tmp_path / "source"
    build_dir = tmp_path / "build"
    (source_dir / "registered").mkdir(parents=True)
    (source_dir / "CMakeLists.txt").write_text(
        "cmake_minimum_required(VERSION 3.28)\n"
        "project(SanitizerTestEnvironment NONE)\n"
        "set(QGC_BUILD_TESTING ON)\n"
        "add_executable(${PROJECT_NAME} IMPORTED)\n"
        'set_target_properties(${PROJECT_NAME} PROPERTIES IMPORTED_LOCATION "${CMAKE_COMMAND}")\n'
        "enable_testing()\n"
        f'include("{(REPO_ROOT / "cmake/QGCTest.cmake").as_posix()}")\n'
        "add_subdirectory(registered)\n"
        'add_test(NAME Direct COMMAND "${CMAKE_COMMAND}" -E environment)\n',
        encoding="utf-8",
    )
    (source_dir / "registered/CMakeLists.txt").write_text(
        "add_qgc_test(Registered LABELS Unit)\n", encoding="utf-8"
    )
    subprocess.run(
        [
            "cmake",
            "-S",
            str(source_dir),
            "-B",
            str(build_dir),
            *(f"-D{key}={value}" for key, value in sanitizer_cache.items()),
        ],
        check=True,
        capture_output=True,
        text=True,
    )

    shown = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "--show-only=json-v1"],
        check=True,
        capture_output=True,
        text=True,
    )
    suppressions = build_dir.as_posix()
    expected = [
        "ASAN_OPTIONS=detect_leaks=1:halt_on_error=1:check_initialization_order=1",
        f"LSAN_OPTIONS=suppressions={suppressions}/asan_suppressions.txt",
        f"UBSAN_OPTIONS=print_stacktrace=1:halt_on_error=1:suppressions={suppressions}/ubsan_suppressions.txt",
    ]
    environments = {
        test["name"]: next(
            prop["value"] for prop in test["properties"] if prop["name"] == "ENVIRONMENT"
        )
        for test in json.loads(shown.stdout)["tests"]
    }
    assert set(environments) == {"Direct", "Registered"}
    for environment in environments.values():
        assert environment[-len(expected) :] == expected

    # CTest's options replace the caller's, so local runs match CI.
    env = {name: value for name, value in os.environ.items() if not name.endswith("SAN_OPTIONS")}
    env["ASAN_OPTIONS"] = "detect_leaks=0:halt_on_error=0"
    run = subprocess.run(
        ["ctest", "--test-dir", str(build_dir), "-R", "^Direct$", "-V"],
        check=True,
        env=env,
        capture_output=True,
        text=True,
    )
    assert f": {expected[0]}\n" in run.stdout
    assert env["ASAN_OPTIONS"] not in run.stdout
