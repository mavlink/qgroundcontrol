"""Master pushes publish a signed Android App Bundle alongside the APK."""

from __future__ import annotations

import json
import re

from ..._helpers import get_step, get_steps, load_action, load_workflow

RELEASE_BUNDLES = '--build-dir "${RUNNER_TEMP}/build/android-build/build/outputs/bundle/release"'


def _build_steps() -> list[dict]:
    return get_steps(load_workflow("android.yml"), "build")


def test_app_bundle_is_built_only_for_master_pushes_on_the_release_leg() -> None:
    steps = _build_steps()
    bundle = get_step(steps, "Build Android App Bundle")
    assert bundle["id"] == "aab"
    assert bundle["if"] == (
        "${{ !matrix.emulator && matrix.primary && github.event_name == 'push' && "
        "github.ref == 'refs/heads/master' }}"
    )
    assert "-DQT_ANDROID_SIGN_AAB=ON" in get_step(steps, "Configure")["with"]["extra-args"]


def test_app_bundle_builds_through_cmake_build_beside_the_main_build() -> None:
    steps = _build_steps()
    build = get_step(steps, "build")
    bundle = get_step(steps, "aab")
    assert bundle["uses"] == build["uses"] == "./.github/actions/cmake-build"
    assert bundle["with"] == {"target": "aab", "output-file": "qgc-aab-build.log"}
    # Its own log keeps the main build's for the failure upload; `aab` titles its summary section.
    assert bundle["with"]["output-file"] != build["with"]["output-file"]
    assert bundle["env"] == build["env"] == {"GRADLE_OPTS": "-Dorg.gradle.daemon=false"}
    # Only full and autogen builds save the compiler caches.
    (save,) = [
        step
        for step in get_steps(load_action("cmake-build"))
        if step.get("uses") == "./.github/actions/save-build-cache"
    ]
    assert "(inputs.target == 'all' || inputs.target == 'autogen')" in save["if"]


def test_only_the_release_bundle_is_prepared_after_a_successful_build() -> None:
    steps = _build_steps()
    bundle = get_step(steps, "aab")
    prepare = get_step(steps, "Prepare App Bundle")
    assert steps.index(prepare) == steps.index(bundle) + 1
    assert prepare["id"] == "aab-artifact"
    assert prepare["if"] == "${{ steps.aab.outcome == 'success' }}"
    assert prepare["shell"] == "bash"
    # Gradle's `bundle` task also writes a debug bundle; only the release output is published.
    assert 'tools/ci/packaging/find_artifact.py"' in prepare["run"]
    assert RELEASE_BUNDLES in prepare["run"]
    assert "--pattern '*.aab'" in prepare["run"]
    assert '--copy-to "${RUNNER_TEMP}/build/${PACKAGE}.aab"' in prepare["run"]


def test_app_bundle_is_attested_and_uploaded_separately_from_the_apk() -> None:
    steps = _build_steps()
    upload = get_step(steps, "Attest and Upload App Bundle")
    assert steps.index(upload) > steps.index(get_step(steps, "aab-artifact"))
    assert upload["if"] == "${{ steps.aab-artifact.outcome == 'success' }}"
    assert upload["uses"] == "./.github/actions/attest-and-upload"
    assert upload["with"]["artifact-name"] == "${{ env.PACKAGE }}.aab"
    apk = get_step(steps, "Attest and Upload")
    # Distinct names keep its GitHub artifact, SBOM artifact and sidecars apart from the APK's.
    assert upload["with"]["subject-name"] != apk["with"]["subject-name"]
    assert upload["with"]["package-name"] != apk["with"]["package-name"]
    assert upload["with"]["upload-aws"] == "false"


def test_the_composite_bundle_build_is_referenced_only_by_its_outcome() -> None:
    # cmake-build declares no outputs, so `steps.aab.outputs.*` would always be empty.
    references = set(re.findall(r"steps\.aab\.(\w+)", json.dumps(load_workflow("android.yml"))))
    assert references == {"outcome"}
