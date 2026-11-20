"""Contracts for the repository CMake preset graph."""

from __future__ import annotations

import json
import os
import re
import shutil
import subprocess
from typing import TYPE_CHECKING, Any

from ..._helpers import REPO_ROOT, get_steps, load_workflow
from .._workflows import local_action

if TYPE_CHECKING:
    from pathlib import Path

ROOT_PRESETS = REPO_ROOT / "CMakePresets.json"
# First CMake release reading each presets schema version (cmake-presets(7) "Versions").
PRESETS_VERSION_MIN_CMAKE = {6: (3, 25), 7: (3, 27), 8: (3, 28), 9: (3, 30), 10: (3, 31)}
PRESET_CATEGORIES = {
    "configure": "configurePresets",
    "build": "buildPresets",
    "test": "testPresets",
    "package": "packagePresets",
    "workflow": "workflowPresets",
}
BUILD_CONFIG = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
# Settings that speed up the CI validation builds: less debug information and unity batches.
LEAN_DEBUG_CACHE = ("CMAKE_C_FLAGS_DEBUG", "CMAKE_CXX_FLAGS_DEBUG", "QGC_UNITY_BUILD")


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


def test_every_preset_file_shares_a_schema_the_minimum_cmake_reads() -> None:
    documents = _preset_documents()
    assert len(documents) == 1 + len(list((REPO_ROOT / "cmake/presets").glob("*.json")))
    root = documents[ROOT_PRESETS.resolve()]
    minimum = tuple(int(part) for part in BUILD_CONFIG["build"]["cmake_minimum_version"].split("."))
    assert minimum >= PRESETS_VERSION_MIN_CMAKE[root["version"]]
    for path, document in documents.items():
        name = path.relative_to(REPO_ROOT)
        assert document["version"] == root["version"], name
        assert document["$schema"] == root["$schema"], name


def test_package_presets_package_their_own_configuration_with_its_generator() -> None:
    graph = _load_preset_graph()
    assert graph["package"]
    for name, preset in graph["package"].items():
        assert preset["configurePreset"] == name
        if "generators" in preset:
            generator = _effective_configure_cache_value(
                graph["configure"], name, "QGC_CPACK_GENERATOR"
            )
            assert preset["generators"] == [generator], name


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


def test_windows_qt_architectures_use_one_msvc_generation() -> None:
    sources = (
        REPO_ROOT / "tools/ci/planning/android_matrix.py",
        REPO_ROOT / ".github/workflows/windows.yml",
    )
    generations = {
        generation
        for source in sources
        for generation in re.findall(r"win64_msvc([0-9]+)", source.read_text(encoding="utf-8"))
    }
    assert len(generations) == 1, f"Windows Qt MSVC generations differ: {sorted(generations)}"


def test_developer_debug_presets_keep_full_debug_info_and_per_file_builds() -> None:
    # A base preset that gained the CI validation settings would silently strip developers' builds.
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


def test_sanitizer_job_excludes_the_labels_its_test_preset_excludes() -> None:
    # `ctest --preset Linux-sanitizers` locally and CI's sanitizer leg must skip the same tests.
    steps = get_steps(load_workflow("linux.yml"), "debug-validation")
    (tests,) = (step for step in steps if local_action(step) == "test-phase")
    excluded = _load_preset_graph()["test"]["Linux-sanitizers"]["filter"]["exclude"]["label"]
    assert f"'{excluded}'" in tests["with"]["exclude-labels"]


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
