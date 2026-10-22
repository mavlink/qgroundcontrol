"""Keep optional diagnostics bounded and preserve normal build behavior."""

import pytest

from ..._helpers import get_steps, load_action, load_workflow

BUILD_JOBS = [
    ("linux", "build"),
    ("linux", "debug-validation"),
    ("windows", "build"),
    ("macos", "build"),
    ("android", "build"),
    ("ios", "build"),
    ("docker", "build"),
    ("custom-build", "build"),
]


@pytest.mark.parametrize(("platform", "job_name"), BUILD_JOBS)
def test_build_diagnostics_are_informational_and_debugging_is_opt_in(platform, job_name):
    workflow = load_workflow(f"{platform}.yml")
    option = workflow["on"]["workflow_dispatch"]["inputs"]["debug_runner"]
    assert option["type"] == "boolean"
    assert option["default"] is False
    job = workflow["jobs"][job_name]
    assert job["permissions"]["actions"] == "read"
    steps = job["steps"]
    timeline = next(s for s in steps if s.get("uses") == "Kesin11/actions-timeline@v3")
    checkout = next(s for s in steps if s.get("uses", "").startswith("actions/checkout@"))
    assert steps.index(timeline) < steps.index(checkout)
    assert timeline["continue-on-error"] is True
    assert timeline["with"] == {"show-waiting-runner": True}
    debug = steps[-1]
    assert debug["uses"] == "$/.github/actions/debug-runner"
    assert debug["timeout-minutes"] == 15
    condition = debug["if"].removeprefix("${{ ").removesuffix(" }}")
    assert set(condition.split(" && ")) == {
        "failure()",
        "!cancelled()",
        "inputs.debug_runner",
        "github.ref_type == 'branch'",
    }


def test_debug_session_is_actor_only_and_does_not_wait_in_post_cleanup():
    steps = get_steps(load_action("debug-runner"))
    setup, session = steps
    assert setup["if"] == "runner.os == 'Windows'"
    assert setup["with"]["install"] == "tmate"
    assert setup["with"]["cache"] is False
    assert session["uses"] == "mxschmitt/action-tmate@v3"
    assert session["with"]["limit-access-to-actor"] is True
    assert session["with"]["detached"] is False
    assert session["with"]["install-dependencies"] == "${{ runner.os != 'Windows' }}"
    assert session["with"]["msys2-location"] == "${{ steps.msys2.outputs.msys2-location }}"


def test_unified_attestation_preserves_separate_provenance_and_sbom_subjects():
    steps = get_steps(load_action("attest-and-upload"))
    scan = next(s for s in steps if s.get("uses") == "anchore/sbom-action@v0")
    provenance, sbom, dependency = [s for s in steps if s.get("uses") == "actions/attest@v4"]
    skip = "steps.prepare.outputs.skip != 'true'"
    assert scan["if"] == provenance["if"] == sbom["if"] == skip
    assert scan["with"] == {
        "path": "${{ steps.prepare.outputs.scan-path }}",
        "format": "spdx-json",
        "output-file": "${{ steps.prepare.outputs.sbom-path }}",
        "upload-artifact": True,
        "upload-artifact-retention": 30,
        "artifact-name": "sbom-${{ inputs.subject-name || inputs.package-name }}",
    }
    assert provenance["with"] == {"subject-path": "${{ steps.prepare.outputs.path }}"}
    assert sbom["with"] == {
        "subject-path": provenance["with"]["subject-path"],
        "sbom-path": "${{ steps.prepare.outputs.sbom-path }}",
    }
    assert dependency["with"]["subject-path"] == provenance["with"]["subject-path"]
    assert provenance["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"

    job = load_workflow("docker.yml")["jobs"]["attest-images"]
    assert (
        job["if"] == "github.repository == 'mavlink/qgroundcontrol' && github.event_name == 'push'"
    )
    provenance, sbom = [s for s in job["steps"] if s.get("uses") == "actions/attest@v4"]
    assert "sbom-path" not in provenance["with"]
    assert provenance["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"
    assert sbom["with"]["sbom-path"] == "image-sbom.cdx.json"
    for key in ("subject-name", "subject-digest", "push-to-registry"):
        assert provenance["with"][key] == sbom["with"][key]
    assert provenance["with"]["subject-digest"] == "${{ steps.image.outputs.digest }}"
    assert provenance["with"]["push-to-registry"] is True
