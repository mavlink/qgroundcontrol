"""build-setup's steps follow its planner and its summary, in the order the setup actions need."""

from __future__ import annotations

import json
import re

from ci.reporting.setup_summary import STEP_LABELS
from ci.runner.build_setup import SetupInputs, plan_setup
from common.build_config import load_build_config

from ..._helpers import get_steps, load_action
from .._workflows import local_action


def _used_actions() -> list[str]:
    """The action each build-setup step runs, without its version; empty for a run step."""
    return [
        str(step.get("uses", "")).partition("@")[0]
        for step in get_steps(load_action("build-setup"))
    ]


def test_action_reads_only_plan_outputs_the_planner_writes() -> None:
    referenced = set(
        re.findall(r"steps\.plan\.outputs\.(\w+)", json.dumps(load_action("build-setup")))
    )
    assert referenced
    config = load_build_config()
    for inputs in (
        SetupInputs("desktop", "windows", "win64_msvc2022_arm64_cross_compiled"),
        SetupInputs("android", "linux", "linux_gcc_64", "x86_64"),
        SetupInputs("ios", qt_arch="ios_device"),
    ):
        assert referenced <= plan_setup(inputs, config).outputs.keys()


def test_java_is_installed_before_gradle_and_the_android_sdk() -> None:
    # setup-gradle and sdkmanager run `java`; RunsOn's Windows image ships none.
    uses = _used_actions()
    java = uses.index("actions/setup-java")
    assert java < uses.index("gradle/actions/setup-gradle")
    assert java < uses.index("android-actions/setup-android")


def test_gradle_hashes_the_workspace_before_the_cpm_cache_and_qt_are_restored() -> None:
    # The basic provider's key globs **/*.gradle* under the workspace, following symlinks; Windows
    # cannot stat android_openssl's per-ABI `include` symlinks in the restored CPM cache (EPERM).
    uses = _used_actions()
    gradle = uses.index("gradle/actions/setup-gradle")
    restores = [
        index
        for index, action in enumerate(uses)
        if action in {"./.github/actions/qt-install", "./.github/actions/cache"}
    ]
    assert restores
    assert gradle < min(restores)
    # Its cache switch reads the backend that setup-python's probe exports.
    assert uses.index("./.github/actions/setup-python") < gradle


def test_setup_summary_can_name_every_step_that_may_fail() -> None:
    # Only a step with an id has an outcome, and setup_summary.py labels a failed one by its id.
    *setup, summary = get_steps(load_action("build-setup"))
    assert "toJSON(steps)" in json.dumps(summary["env"])
    # The disk watch never fails the job.
    reported = [step for step in setup if local_action(step) != "disk-watch"]
    assert [step.get("id") for step in reported] == list(STEP_LABELS)
    for step in reported:
        assert STEP_LABELS[step["id"]] == step.get("name", STEP_LABELS[step["id"]])
