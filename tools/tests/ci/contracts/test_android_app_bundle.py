"""Master pushes publish a signed Android App Bundle alongside the APK."""

from __future__ import annotations

from ..._helpers import get_steps, load_workflow


def _build_steps() -> dict[str, dict]:
    steps = get_steps(load_workflow("android.yml"), "build")
    return {step["name"]: step for step in steps if "name" in step}


def test_app_bundle_is_built_only_for_master_pushes_on_the_release_leg() -> None:
    steps = _build_steps()
    bundle = steps["Build Android App Bundle"]
    assert bundle["id"] == "aab"
    assert bundle["if"] == (
        "${{ !matrix.emulator && matrix.primary && github.event_name == 'push' && "
        "github.ref == 'refs/heads/master' }}"
    )
    assert "--target aab" in bundle["run"]
    # Gradle's `bundle` task also writes a debug bundle; only the release output is published.
    assert (
        '--build-dir "${RUNNER_TEMP}/build/android-build/build/outputs/bundle/release"'
        in bundle["run"]
    )
    assert '--copy-to "${RUNNER_TEMP}/build/${PACKAGE}.aab"' in bundle["run"]
    assert "-DQT_ANDROID_SIGN_AAB=ON" in steps["Configure"]["with"]["extra-args"]


def test_app_bundle_is_attested_and_uploaded_separately_from_the_apk() -> None:
    steps = _build_steps()
    upload = steps["Attest and Upload App Bundle"]
    assert upload["if"] == "${{ steps.aab.outcome == 'success' }}"
    assert upload["uses"] == "$/.github/actions/attest-and-upload"
    assert upload["with"]["artifact-name"] == "${{ env.PACKAGE }}.aab"
    # Distinct names keep its GitHub artifact, SBOM artifact and sidecars apart from the APK's.
    assert upload["with"]["subject-name"] != steps["Attest and Upload"]["with"]["subject-name"]
    assert upload["with"]["package-name"] != steps["Attest and Upload"]["with"]["package-name"]
    assert upload["with"]["upload-aws"] == "false"
