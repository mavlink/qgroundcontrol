"""CI builds take CMake and Ninja from build-config's pins; install-dependencies adds no others."""

from __future__ import annotations

import json
import re
from typing import Any

from common.build_config import export_build_config_values, github_output_values

from ..._helpers import REPO_ROOT, get_step, get_steps, load_action, load_workflow, workflow_names

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
    assert step["with"]["cmakeVersion"] == "${{ steps.config.outputs.build_cmake_version }}"
    assert step["with"]["ninjaVersion"] == "${{ steps.config.outputs.build_ninja_version }}"

    action_outputs = load_action("build-config")["outputs"]
    read_outputs = github_output_values(export_build_config_values(CONFIG))
    for pin in PINS:
        name = f"build_{pin}"
        assert action_outputs[name]["value"] == f"${{{{ steps.read.outputs.{name} }}}}"
        assert read_outputs[name] == CONFIG["build"][pin]


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
        "Restore apt download cache (Linux)",
        "Install apt packages (Linux)",
    ]
    cache = get_step(steps, "Restore apt download cache (Linux)")
    assert cache["uses"].startswith("actions/cache@")
    assert cache["with"]["key"] == "${{ steps.apt-plan.outputs.key }}"
    assert cache["with"]["restore-keys"] == "${{ steps.apt-plan.outputs.restore-keys }}"
    commands = " ".join(step.get("run", "") for step in steps)
    assert "tools/setup/install_dependencies" not in commands
    assert commands.count("tools/ci/runner/install_dependencies_helper.py") == 2


def test_macos_installs_through_the_ci_helper() -> None:
    (step,) = [
        step for step in get_steps(load_action("install-dependencies")) if "macOS" in step["if"]
    ]
    assert step["with"]["command"] == "python3 tools/ci/runner/install_dependencies_helper.py macos"


def test_jobs_installing_dependencies_get_the_pinned_tools_first() -> None:
    """The helper skips the distro CMake, Ninja, ccache and mold; build-setup and its cache install them."""
    # Windows installs MSVC before build-setup; build-gstreamer's category lists none of these tools.
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
