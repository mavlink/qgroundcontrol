"""build-setup prepares the Android toolchain in the order its setup actions need."""

from __future__ import annotations

from ..._helpers import get_steps, load_action


def test_java_is_installed_before_gradle_and_the_android_sdk() -> None:
    # setup-gradle and sdkmanager run `java`; RunsOn's Windows image ships none.
    steps = get_steps(load_action("build-setup"))
    android = [step for step in steps if step.get("if", "").startswith("inputs.mode == 'android'")]
    uses = [step.get("uses", "").split("@")[0] for step in android]
    java = uses.index("actions/setup-java")
    assert android[java]["if"] == "inputs.mode == 'android'"
    assert java < uses.index("gradle/actions/setup-gradle")
    assert java < uses.index("android-actions/setup-android")
