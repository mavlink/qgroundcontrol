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
    ("linux", "custom-plugin"),
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
    assert debug["uses"] == "./.github/actions/debug-runner"
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
    # Pull request builds aren't released: one event condition gates every attestation step.
    not_pr = "github.event_name != 'pull_request'"
    assert scan["if"] == provenance["if"] == sbom["if"] == dependency["if"] == not_pr
    assert scan["with"] == {
        "path": "${{ runner.temp }}/build",
        "format": "spdx-json",
        "output-file": "${{ steps.prepare.outputs.sbom-path }}",
        "upload-artifact": True,
        "upload-artifact-retention": 30,
        "artifact-name": "sbom-${{ steps.prepare.outputs.subject-name }}",
    }
    assert provenance["with"] == {"subject-path": "${{ steps.prepare.outputs.path }}"}
    assert sbom["with"] == {
        "subject-path": provenance["with"]["subject-path"],
        "sbom-path": "${{ steps.prepare.outputs.sbom-path }}",
    }
    assert dependency["with"]["subject-path"] == provenance["with"]["subject-path"]
    assert provenance["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"

    job = load_workflow("docker.yml")["jobs"]["attest-images"]
    # plan_platform.py gives every leg the same push target (test_plan_platform.py checks that
    # this matches pushes to the upstream repository).
    assert job["needs"] == ["changes", "build"]
    assert job["if"] == '${{ !contains(needs.changes.outputs.matrix, \'"push_target":""\') }}'
    assert job["strategy"]["matrix"] == {"include": "${{ fromJSON(needs.changes.outputs.matrix) }}"}
    steps = {step["name"]: step for step in job["steps"]}
    assert steps["Login to GHCR"]["if"] == "startsWith(matrix.push_target, 'ghcr.io/')"
    assert (
        steps["Login to Docker Hub"]["if"] == "${{ !startsWith(matrix.push_target, 'ghcr.io/') }}"
    )
    provenance, sbom = [s for s in job["steps"] if s.get("uses") == "actions/attest@v4"]
    assert "sbom-path" not in provenance["with"]
    assert provenance["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"
    assert sbom["with"]["sbom-path"] == "image-sbom.cdx.json"
    for key in ("subject-name", "subject-digest", "push-to-registry"):
        assert provenance["with"][key] == sbom["with"][key]
    assert provenance["with"]["subject-name"] == "${{ matrix.push_target }}"
    assert provenance["with"]["subject-digest"] == "${{ steps.image.outputs.digest }}"
    assert provenance["with"]["push-to-registry"] is True

    build = {
        step.get("name"): step for step in load_workflow("docker.yml")["jobs"]["build"]["steps"]
    }
    assert build["Build with Docker"]["with"]["push-image"] == "${{ matrix.push_target }}"
    for name in ("Save published image digest", "Upload image security evidence"):
        assert build[name]["if"] == "matrix.push_target != ''"


def test_attestation_actions_publish_checksum_and_resolved_sbom_output() -> None:
    ci_workflow = load_workflow("ci-scripts.yml")
    ci_steps = ci_workflow["jobs"]["test-ci-scripts"]["steps"]
    checkout = next(step for step in ci_steps if step["name"] == "Checkout")
    sparse_checkout = set(checkout["with"]["sparse-checkout"].splitlines())
    assert ".github" in sparse_checkout or ".github/actions/attest-and-upload" in sparse_checkout

    upload_action = load_action("attest-and-upload")
    upload_steps = {step["name"]: step for step in upload_action["runs"]["steps"]}
    prepare = upload_steps["Prepare artifact checksum, dependency SBOM and build identity"]
    dependency_attestation = upload_steps["Attest CPM dependency SBOM"]
    save = upload_steps["Save artifact"]
    credentials = upload_steps["Configure AWS credentials"]
    publish = upload_steps["Upload artifact and checksum to AWS"]

    assert 'attest_helper.py" prepare' in prepare["run"]
    assert prepare["env"]["ARTIFACT_NAME"] == "${{ inputs.artifact-name }}"
    assert prepare["env"]["PACKAGE_NAME"] == "${{ inputs.package-name }}"
    assert prepare["env"]["SUBJECT_NAME"] == "${{ inputs.subject-name }}"
    assert prepare["env"]["UPLOAD_AWS"] == "${{ inputs.upload-aws }}"
    # attest_helper.py adds the release policy; only credential presence reaches it.
    assert prepare["env"]["HAS_AWS_CREDENTIALS"] == (
        "${{ inputs.aws-role-arn != '' || "
        "(inputs.aws-key-id != '' && inputs.aws-secret-access-key != '') }}"
    )
    assert "secret" not in prepare["run"].lower()
    assert dependency_attestation["uses"] == "actions/attest@v4"
    assert dependency_attestation["if"] == "github.event_name != 'pull_request'"
    assert dependency_attestation["with"] == {
        "subject-path": "${{ steps.prepare.outputs.path }}",
        "sbom-path": "${{ steps.prepare.outputs.dependency-sbom }}",
    }
    for output in ("path", "checksum", "metadata", "dependency-sbom"):
        assert f"${{{{ steps.prepare.outputs.{output} }}}}" in save["with"]["path"]
    assert save["with"]["retention-days"] == (
        "${{ github.event_name == 'pull_request' && '7' || '30' }}"
    )
    assert save["with"]["compression-level"] == 0

    assert credentials["uses"] == "./.github/actions/aws-credentials"
    assert credentials["if"] == "steps.prepare.outputs.publish-aws == 'true'"
    assert publish["if"] == "steps.aws-credentials.outcome == 'success'"
    assert publish["env"]["CHECKSUM_PATH"] == "${{ steps.prepare.outputs.checksum }}"
    assert '--artifact "${ARTIFACT_PATH}:${ARTIFACT_NAME}"' in publish["run"]
    assert '--artifact "${CHECKSUM_PATH}:${ARTIFACT_NAME}.sha256"' in publish["run"]
    # aws_upload.py reads the ref from the runner's GITHUB_REF_NAME and GITHUB_REF_TYPE.
    assert "--ref-" not in publish["run"]
    assert "secret" not in str(publish).lower()
