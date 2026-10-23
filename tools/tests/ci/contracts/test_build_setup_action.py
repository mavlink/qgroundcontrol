"""build-setup plans its mode-dependent values in one step and prepares the Android toolchain in
the order its setup actions need."""

from __future__ import annotations

import re

from ci.reporting.setup_summary import STEP_LABELS
from ci.runner.build_setup import SetupInputs, plan_setup
from common.build_config import load_build_config

from ..._helpers import REPO_ROOT, get_steps, load_action


def test_plan_step_runs_first_and_supplies_every_plan_output_the_action_reads() -> None:
    action = load_action("build-setup")
    # Only the disk watch (Node, so no Python) starts earlier, to see the whole job.
    watch, plan = get_steps(action)[:2]
    assert watch["uses"] == "./.github/actions/disk-watch"
    assert plan["id"] == "plan"
    assert plan["run"] == 'python3 "${GITHUB_WORKSPACE}/tools/ci/runner/build_setup.py"'
    assert plan["env"] == {
        "INPUT_MODE": "${{ inputs.mode }}",
        "INPUT_QT_HOST": "${{ inputs.qt-host }}",
        "INPUT_QT_ARCH": "${{ inputs.qt-arch }}",
        "INPUT_ABIS": "${{ inputs.abis }}",
    }
    text = (REPO_ROOT / ".github/actions/build-setup/action.yml").read_text(encoding="utf-8")
    referenced = set(re.findall(r"steps\.plan\.outputs\.(\w+)", text))
    config = load_build_config()
    for inputs in (
        SetupInputs("desktop", "windows", "win64_msvc2022_arm64_cross_compiled"),
        SetupInputs("android", "linux", "linux_gcc_64", "x86_64"),
        SetupInputs("ios", qt_arch="ios_device"),
    ):
        assert referenced <= plan_setup(inputs, config).outputs.keys()
    assert {"qt_host", "cache_target", "host_qt_arch", "msvc_arch", "android_x86_64"} <= referenced


def test_java_is_installed_before_gradle_and_the_android_sdk() -> None:
    # setup-gradle and sdkmanager run `java`; RunsOn's Windows image ships none.
    steps = get_steps(load_action("build-setup"))
    android = [step for step in steps if step.get("if", "").startswith("inputs.mode == 'android'")]
    uses = [step.get("uses", "").split("@")[0] for step in android]
    java = uses.index("actions/setup-java")
    assert android[java]["if"] == "inputs.mode == 'android'"
    assert java < uses.index("gradle/actions/setup-gradle")
    assert java < uses.index("android-actions/setup-android")


def test_gradle_hashes_the_workspace_before_the_cpm_cache_and_qt_are_restored() -> None:
    # The basic provider's key globs **/*.gradle* under the workspace, following symlinks; Windows
    # cannot stat android_openssl's per-ABI `include` symlinks in the restored CPM cache (EPERM).
    steps = get_steps(load_action("build-setup"))
    names = [step.get("name") or step.get("uses", "") for step in steps]
    gradle = names.index("Setup Gradle (Android)")
    assert names.index("./.github/actions/setup-python") < gradle
    assert gradle < names.index("Setup caching")
    for index, step in enumerate(steps):
        if step.get("uses") in {"./.github/actions/qt-install", "./.github/actions/cache"}:
            assert gradle < index, names[index]


def test_setup_summary_runs_after_a_failed_step_and_names_it() -> None:
    # Without a status function a failed Qt or SDK install would also skip the Setup section, the
    # only place the job summary shows the runner's image and instance.
    *setup, summary = get_steps(load_action("build-setup"))
    assert summary["name"] == "Summarize setup"
    assert summary["if"] == "${{ !cancelled() }}"
    assert summary["env"]["SETUP_STEPS"] == "${{ toJSON(steps) }}"
    # Only steps with an id have an outcome; the disk watch never fails the job.
    watch, *rest = setup
    assert watch["uses"] == "./.github/actions/disk-watch"
    assert "id" not in watch
    assert [step.get("id") for step in rest] == list(STEP_LABELS)
    for step in rest:
        assert STEP_LABELS[step["id"]] == step.get("name", STEP_LABELS[step["id"]])
