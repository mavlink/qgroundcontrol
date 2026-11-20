"""CI builds take CMake and Ninja from build-config's pins, and Renovate tracks every pin there."""

from __future__ import annotations

import fnmatch
import json
import re
from typing import Any

import pytest
from ci.runner.build_setup import SetupInputs, plan_setup
from common.build_config import lookup_dotted

from ..._helpers import REPO_ROOT, evaluate_expression, get_steps, load_action
from .._workflows import local_action, step_groups, uses_action, uses_references

CONFIG = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
RENOVATE = json.loads((REPO_ROOT / ".github/renovate.json").read_text(encoding="utf-8"))
PINS = ("cmake_version", "ninja_version")


def _version(value: str) -> tuple[int, ...]:
    return tuple(int(part) for part in value.split("."))


def _get_cmake() -> tuple[int, dict[str, Any]]:
    steps = get_steps(load_action("build-setup"))
    (index,) = [i for i, step in enumerate(steps) if uses_action(step, "lukka/get-cmake")]
    return index, steps[index]


def test_get_cmake_installs_the_build_config_pins() -> None:
    _, step = _get_cmake()
    assert step["with"]["cmakeVersion"] == "${{ steps.plan.outputs.cmake_version }}"
    assert step["with"]["ninjaVersion"] == "${{ steps.plan.outputs.ninja_version }}"

    outputs = plan_setup(SetupInputs("desktop", "linux", "linux_gcc_64"), CONFIG).outputs
    for pin in PINS:
        assert outputs[pin] == CONFIG["build"][pin]


def test_get_cmake_precedes_the_python_environment_on_path() -> None:
    """GITHUB_PATH prepends, so the later step wins over the build and dev groups' CMake."""
    index, _ = _get_cmake()
    steps = get_steps(load_action("build-setup"))
    (python,) = [i for i, step in enumerate(steps) if local_action(step) == "setup-python"]
    assert python < index


def test_pinned_cmake_is_in_the_get_cmake_catalog_and_meets_the_minimum() -> None:
    """get-cmake resolves versions from the catalog bundled in its release; tag vX bundles CMake X."""
    # The action is pinned to a commit, so its version is the comment after the SHA.
    (version,) = {
        comment
        for _, _, references in uses_references()
        for uses, comment in references
        if uses.startswith("lukka/get-cmake@")
    }
    tag = re.fullmatch(r"v(\d+\.\d+\.\d+)", version)
    assert tag is not None
    pinned = _version(CONFIG["build"]["cmake_version"])
    assert _version(CONFIG["build"]["cmake_minimum_version"]) <= pinned <= _version(tag[1])


def _tracked_pins() -> dict[str, str]:
    """Each dependency Renovate's custom managers extract from build-config.json, and its value."""
    config_text = (REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8")
    tracked = {}
    for manager in RENOVATE["customManagers"]:
        if "/^\\.github/build-config\\.json$/" in manager["managerFilePatterns"]:
            for pattern in manager["matchStrings"]:
                for match in re.finditer(pattern.replace("(?<", "(?P<"), config_text):
                    name = manager.get("depNameTemplate") or match["depName"]
                    tracked[name] = match["currentValue"]
    return tracked


def _rules_naming(name: str) -> list[dict[str, Any]]:
    return [
        rule
        for rule in RENOVATE["packageRules"]
        if name in [*rule.get("matchDepNames", []), *rule.get("matchPackageNames", [])]
    ]


def test_schema_requires_the_pins_and_renovate_tracks_them() -> None:
    schema = json.loads((REPO_ROOT / ".github/build-config.schema.json").read_text())
    assert set(PINS) <= set(schema["properties"]["build"]["required"])

    released_tools = {
        pin["repository"]: pin["version"] for pin in CONFIG["tools"].values() if "repository" in pin
    }
    appimage_tools = {pin["repository"]: pin["tag"] for pin in CONFIG["appimage"].values()}
    assert _tracked_pins() == {
        "lukka/get-cmake": CONFIG["build"]["cmake_version"],
        "ninja-build/ninja": CONFIG["build"]["ninja_version"],
        "openssl/openssl": CONFIG["android"]["openssl_version"],
        **released_tools,
        **appimage_tools,
    }

    # A release rebuilt in place under one tag has no version to follow.
    rolling = {name for name, tag in appimage_tools.items() if tag == "continuous"}
    disabled = {
        name
        for name in appimage_tools
        if any(rule.get("enabled") is False for rule in _rules_naming(name))
    }
    assert disabled == rolling

    # build-config.json also pins these releases' SHA-256 digests, which a bump must update by
    # hand, so none may join the weekly group PR or open a PR before it is approved.
    rules = RENOVATE["packageRules"]
    weekly = next(i for i, rule in enumerate(rules) if rule.get("groupName") == "all dependencies")
    held = {
        name
        for rule in rules[weekly + 1 :]
        if rule.get("dependencyDashboardApproval") is True and rule.get("groupName", "") is None
        for name in rule.get("matchDepNames", [])
    }
    assert {*released_tools, *appimage_tools, "openssl/openssl"} - rolling <= held


def test_workflows_read_only_build_config_keys_that_exist() -> None:
    read = 0
    for label, _, steps in step_groups():
        for step in steps:
            for keys in re.findall(
                r"read_config\.py --(?:github-output|get) ([\w. ]+)", step.get("run", "")
            ):
                for key in keys.split():
                    assert lookup_dotted(CONFIG, key, None) is not None, (label, key)
                    read += 1
    assert read


def test_renovate_version_rules_accept_the_pinned_values() -> None:
    """A pin outside its rule's allowedVersions or versioning scheme would silently stop updating."""
    checked = 0
    for name, value in _tracked_pins().items():
        for rule in _rules_naming(name):
            if (allowed := rule.get("allowedVersions", "")).startswith("/"):
                assert re.search(allowed.strip("/"), value), name
                checked += 1
            if (versioning := rule.get("versioning", "")).startswith("regex:"):
                scheme = versioning.removeprefix("regex:").replace("(?<", "(?P<")
                assert re.fullmatch(scheme, value), name
                checked += 1
    assert checked


def test_renovate_holds_the_qt_coupled_android_toolchain_for_approval() -> None:
    """Qt's Android template pins these; an unreviewed bump can outrun the compileSdk Qt supports."""
    (rule,) = [
        rule
        for rule in RENOVATE["packageRules"]
        if "com.android.tools.build:gradle" in rule.get("matchDepNames", [])
    ]
    assert rule["dependencyDashboardApproval"] is True
    assert rule["groupName"] == "android toolchain"
    assert set(rule["matchManagers"]) == {"gradle", "gradle-wrapper"}
    # The gradle-wrapper manager names the wrapper's Gradle "gradle".
    assert "gradle" in rule["matchDepNames"]
    held = [re.compile(fnmatch.translate(name)) for name in rule["matchDepNames"]]
    build_gradle = (REPO_ROOT / "android/build.gradle").read_text(encoding="utf-8")
    coupled = [
        dependency
        for dependency in re.findall(r"""['"]([\w.-]+:[\w.-]+):[^'"]+['"]""", build_gradle)
        if dependency.startswith(("androidx.", "com.android.", "org.jetbrains.kotlin:"))
    ]
    assert "androidx.core:core" in coupled
    for dependency in coupled:
        assert any(name.match(dependency) for name in held), dependency


@pytest.mark.parametrize(
    ("event", "cache_hit", "saves"),
    [
        ("push", "", True),
        ("schedule", "false", True),
        ("push", "true", False),
        # Copilot sessions cannot write the cache; a save only logs "cache write denied".
        ("dynamic", "", False),
    ],
)
def test_apt_downloads_are_saved_by_runs_that_may_write_the_cache(
    event: str, cache_hit: str, saves: bool
) -> None:
    (save,) = (
        step
        for step in get_steps(load_action("install-dependencies"))
        if uses_action(step, "actions/cache/save")
    )
    context = {
        "runner": {"os": "Linux"},
        "github": {"event_name": event},
        "steps": {"apt-cache": {"outputs": {"cache-hit": cache_hit}}},
    }
    assert bool(evaluate_expression(save["if"], context)) is saves


def test_jobs_installing_dependencies_get_the_pinned_tools_first() -> None:
    """The helper skips the distro CMake, Ninja, ccache and mold; build-setup and its cache install them."""
    checked = 0
    for label, _, steps in step_groups(actions=False):
        # Windows installs MSVC before build-setup.
        if label.startswith("windows.yml:"):
            continue
        used = [local_action(step) for step in steps]
        if "install-dependencies" not in used:
            continue
        assert "build-setup" in used[: used.index("install-dependencies")], label
        checked += 1
    assert checked
