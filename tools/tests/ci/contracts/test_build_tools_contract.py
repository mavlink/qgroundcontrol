"""CI builds take CMake and Ninja from build-config's pins; install-dependencies adds no others."""

from __future__ import annotations

import json
import re
from typing import Any

import pytest
from ci.runner.build_setup import SetupInputs, plan_setup

from ..._helpers import (
    REPO_ROOT,
    evaluate_expression,
    get_step,
    get_steps,
    load_action,
    load_workflow,
    workflow_names,
)

CONFIG = json.loads((REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8"))
PINS = ("cmake_version", "ninja_version")


def _version(value: str) -> tuple[int, ...]:
    return tuple(int(part) for part in value.split("."))


def _get_cmake() -> tuple[int, dict[str, Any]]:
    steps = get_steps(load_action("build-setup"))
    (index,) = [i for i, step in enumerate(steps) if "lukka/get-cmake@" in step.get("uses", "")]
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
    (python,) = [
        i for i, step in enumerate(steps) if step.get("uses") == "./.github/actions/setup-python"
    ]
    assert python < index


def test_pinned_cmake_is_in_the_get_cmake_catalog_and_meets_the_minimum() -> None:
    """get-cmake resolves versions from the catalog bundled in its release; tag vX bundles CMake X."""
    _, step = _get_cmake()
    tag = re.fullmatch(r"lukka/get-cmake@v(\d+\.\d+\.\d+)", step["uses"])
    assert tag is not None
    pinned = _version(CONFIG["build"]["cmake_version"])
    assert _version(CONFIG["build"]["cmake_minimum_version"]) <= pinned <= _version(tag[1])


def test_schema_requires_the_pins_and_renovate_tracks_them() -> None:
    schema = json.loads((REPO_ROOT / ".github/build-config.schema.json").read_text())
    assert set(PINS) <= set(schema["properties"]["build"]["required"])

    config_text = (REPO_ROOT / ".github/build-config.json").read_text(encoding="utf-8")
    renovate = json.loads((REPO_ROOT / ".github/renovate.json").read_text(encoding="utf-8"))
    tracked = {}
    for manager in renovate["customManagers"]:
        if "/^\\.github/build-config\\.json$/" in manager["managerFilePatterns"]:
            for pattern in manager["matchStrings"]:
                for match in re.finditer(pattern.replace("(?<", "(?P<"), config_text):
                    name = manager.get("depNameTemplate") or match["depName"]
                    tracked[name] = match["currentValue"]
    released_tools = {
        pin["repository"]: pin["version"] for pin in CONFIG["tools"].values() if "repository" in pin
    }
    assert tracked == {
        "lukka/get-cmake": CONFIG["build"]["cmake_version"],
        "ninja-build/ninja": CONFIG["build"]["ninja_version"],
        **released_tools,
    }
    assert set(released_tools) == {
        "ccache/ccache",
        "rui314/mold",
        "mozilla/sccache",
        "anchore/grype",
        "hashicorp/packer",
        "runs-on/cli",
        "aras-p/ClangBuildAnalyzer",
    }
    # A version bump needs new digests, so it must not join the weekly group PR unapproved.
    rules = renovate["packageRules"]
    (rule,) = [rule for rule in rules if set(rule.get("matchDepNames", [])) == set(released_tools)]
    assert rule["dependencyDashboardApproval"] is True
    assert rule["groupName"] is None
    weekly = next(i for i, rule in enumerate(rules) if rule.get("groupName") == "all dependencies")
    assert rules.index(rule) > weekly


def test_linux_install_plans_caches_and_installs_without_python_build_tools() -> None:
    steps = [
        step for step in get_steps(load_action("install-dependencies")) if "Linux" in step["if"]
    ]
    assert [step.get("id") or step["name"] for step in steps] == [
        "apt-plan",
        "apt-cache",
        "Install apt packages (Linux)",
        "Save apt download cache (Linux)",
    ]
    cache = get_step(steps, "Restore apt download cache (Linux)")
    assert cache["uses"].startswith("actions/cache/restore@")
    assert cache["with"]["key"] == "${{ steps.apt-plan.outputs.key }}"
    assert cache["with"]["restore-keys"] == "${{ steps.apt-plan.outputs.restore-keys }}"
    save = get_step(steps, "Save apt download cache (Linux)")
    assert save["uses"].startswith("actions/cache/save@")
    assert save["with"] == {"path": cache["with"]["path"], "key": cache["with"]["key"]}
    assert save["continue-on-error"] is True
    commands = " ".join(step.get("run", "") for step in steps)
    assert "tools/setup/install_dependencies" not in commands
    assert commands.count("tools/ci/runner/install_dependencies_helper.py") == 2


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
    save = get_step(
        get_steps(load_action("install-dependencies")), "Save apt download cache (Linux)"
    )
    context = {
        "runner": {"os": "Linux"},
        "github": {"event_name": event},
        "steps": {"apt-cache": {"outputs": {"cache-hit": cache_hit}}},
    }
    assert bool(evaluate_expression(save["if"], context)) is saves


@pytest.mark.parametrize(("event", "cloud"), [("push", True), ("dynamic", False)])
def test_get_cmake_skips_the_cloud_cache_in_copilot_sessions(event: str, cloud: bool) -> None:
    _, step = _get_cmake()
    context = {"github": {"event_name": event}}
    assert evaluate_expression(step["with"]["useCloudCache"], context) is cloud
    assert step["with"]["useLocalCache"] is False


@pytest.mark.parametrize("runner_os", ["macOS", "Windows"])
def test_macos_and_windows_install_through_the_retrying_ci_helper(runner_os: str) -> None:
    """install_dependencies_helper.py retries and times out each attempt itself."""
    (step,) = [
        step
        for step in get_steps(load_action("install-dependencies"))
        if step["if"] == f"runner.os == '{runner_os}'" and "run" in step
    ]
    helper = '"${GITHUB_WORKSPACE}/tools/ci/runner/install_dependencies_helper.py"'
    assert step["run"] == f"python3 {helper} {runner_os.lower()}"
    assert step["shell"] == "bash"
    assert "uses" not in step


def test_jobs_installing_dependencies_get_the_pinned_tools_first() -> None:
    """The helper skips the distro CMake, Ninja, ccache and mold; build-setup and its cache install them."""
    # Windows installs MSVC before build-setup. build-gstreamer installs before build-setup, and its
    # Android leg skips it and installs the pins itself (test_build_gstreamer_workflow.py).
    exempt = {"windows.yml", "build-gstreamer.yml"}
    checked = 0
    for workflow in workflow_names():
        if workflow in exempt:
            continue
        for job in (load_workflow(workflow).get("jobs") or {}).values():
            steps = job.get("steps") or []
            uses = [step.get("uses") for step in steps]
            if "./.github/actions/install-dependencies" not in uses:
                continue
            index = uses.index("./.github/actions/install-dependencies")
            setups = [
                step
                for step in steps[:index]
                if step.get("uses") == "./.github/actions/build-setup"
            ]
            assert setups, workflow
            assert all(
                (step.get("with") or {}).get("build-cache", "true") == "true" for step in setups
            )
            checked += 1
    assert checked >= 8
