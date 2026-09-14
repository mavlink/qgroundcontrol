"""Keep optional diagnostics bounded and preserve normal build behavior."""

import pytest
import yaml
from _helpers import REPO_ROOT

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


def _load(path):
    return yaml.safe_load((REPO_ROOT / path).read_text(encoding="utf-8"))


@pytest.mark.parametrize(("platform", "job_name"), BUILD_JOBS)
def test_build_diagnostics_are_informational_and_debugging_is_opt_in(platform, job_name):
    workflow = _load(f".github/workflows/{platform}.yml")
    option = workflow[True]["workflow_dispatch"]["inputs"]["debug_runner"]
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
    assert debug["uses"] == "./.github/actions/debug-runner"
    assert debug["timeout-minutes"] == 15
    condition = debug["if"].removeprefix("${{ ").removesuffix(" }}")
    assert set(condition.split(" && ")) == {
        "failure()",
        "!cancelled()",
        "github.event_name == 'workflow_dispatch'",
        "inputs.debug_runner",
        "github.ref_type == 'branch'",
    }


def test_debug_session_is_actor_only_and_does_not_wait_in_post_cleanup():
    steps = _load(".github/actions/debug-runner/action.yml")["runs"]["steps"]
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
    steps = _load(".github/actions/attest-sbom/action.yml")["runs"]["steps"]
    provenance, sbom = [s for s in steps if s.get("uses") == "actions/attest@v4"]
    assert provenance["if"] == sbom["if"] == "steps.check.outputs.skip != 'true'"
    assert provenance["with"] == {"subject-path": "${{ inputs.subject-path }}"}
    assert sbom["with"]["subject-path"] == provenance["with"]["subject-path"]
    assert "sbom-path" in sbom["with"]
    assert provenance["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"

    job = _load(".github/workflows/docker.yml")["jobs"]["attest-images"]
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
