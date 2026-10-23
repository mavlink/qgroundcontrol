"""Contracts for workflow conditions written as GitHub expressions instead of shell steps."""

from __future__ import annotations

from ..._helpers import get_steps, load_workflow, workflow_names


def test_bot_pull_requests_start_their_checks() -> None:
    # Pull requests opened with GITHUB_TOKEN start no workflows, so required checks never report.
    # The PX4BuildBot token already opens the Crowdin pull requests.
    creators = {}
    for workflow in workflow_names():
        for job in load_workflow(workflow)["jobs"].values():
            for step in job.get("steps", []):
                if step.get("uses", "").startswith("peter-evans/create-pull-request@"):
                    creators[workflow] = step["with"].get("token")
                    assert job["if"] == "github.repository_owner == 'mavlink'", workflow
    assert creators == dict.fromkeys(
        ["lupdate.yml", "px4-metadata.yml"],
        "${{ secrets.PX4BUILDBOT_ACCESSTOKEN || github.token }}",
    )


def test_play_store_rollout_depends_only_on_ref_type() -> None:
    steps = get_steps(load_workflow("android.yml"), "build")
    (deploy,) = (step for step in steps if step.get("name") == "Deploy to Play Store")
    assert deploy["uses"] == "r0adkll/upload-google-play@v1"
    assert deploy["with"]["track"] == "${{ github.ref_type == 'tag' && 'production' || 'beta' }}"
    assert deploy["with"]["status"] == (
        "${{ github.ref_type == 'tag' && 'inProgress' || 'completed' }}"
    )
    # Tags keep their 20% staged rollout; completed beta releases take no fraction.
    assert deploy["with"]["userFraction"] == "${{ github.ref_type == 'tag' && '0.2' || '' }}"


def test_crowdin_mode_covers_every_trigger() -> None:
    workflow = load_workflow("crowdin.yml")
    triggers = workflow["on"]
    assert set(triggers) == {"push", "schedule", "workflow_dispatch"}
    action = triggers["workflow_dispatch"]["inputs"]["action"]
    assert action["type"] == "choice"
    assert action["required"] is True
    assert action["options"] == ["upload", "download"]

    job = workflow["jobs"]["sync"]
    assert job["env"]["SYNC_MODE"] == (
        "${{ (github.event_name == 'schedule' || inputs.action == 'download') "
        "&& 'download' || 'upload' }}"
    )
    (sync,) = [step for step in job["steps"] if step.get("uses", "").startswith("crowdin/")]
    assert "if" not in sync
    assert sync["name"] == "Sync with Crowdin (${{ env.SYNC_MODE }})"
    # crowdin/github-action only pushes the localization branch and opens its pull request after
    # download_translations, so one step serves both modes.
    assert sync["with"]["upload_sources"] == "${{ env.SYNC_MODE == 'upload' }}"
    assert sync["with"]["download_translations"] == "${{ env.SYNC_MODE == 'download' }}"
    assert sync["with"]["upload_translations"] is False
    assert "download_sources" not in sync["with"]
    assert "download_bundle" not in sync["with"]


def test_codeql_languages_build_without_compilation() -> None:
    job = load_workflow("codeql.yml")["jobs"]["analyze"]
    assert job["strategy"]["matrix"] == {"language": ["actions", "java-kotlin", "python"]}
    (init,) = (step for step in job["steps"] if step.get("name") == "Initialize CodeQL")
    assert init["with"]["build-mode"] == "none"


def test_macos_signing_shares_one_gate() -> None:
    job = load_workflow("macos.yml")["jobs"]["build"]
    # Forks without the signing secrets would otherwise hit SignMacBundle.cmake's FATAL_ERROR.
    assert " ".join(job["env"]["MACOS_SIGN"].split()) == (
        "${{ github.event_name != 'pull_request' && matrix.build_type == 'Release' && "
        "(github.repository_owner == 'mavlink' || secrets.MACOS_SIGNING_IDENTITY != '') }}"
    )
    steps = {step.get("name"): step for step in job["steps"]}
    assert (
        "-DQGC_MACOS_SIGN_WITH_IDENTITY=${{ env.MACOS_SIGN == 'true' && 'ON' || 'OFF' }}"
        in steps["CMake configure"]["with"]["extra-args"]
    )
    assert steps["Import Code Signing Certificate"]["if"] == "env.MACOS_SIGN == 'true'"
    bundle_env = steps["Create app bundle"]["env"]
    assert set(bundle_env) == {
        "QGC_MACOS_SIGNING_IDENTITY",
        "QGC_MACOS_NOTARIZATION_USERNAME",
        "QGC_MACOS_NOTARIZATION_PASSWORD",
        "QGC_MACOS_NOTARIZATION_TEAM_ID",
    }
    for name, value in bundle_env.items():
        secret = name.removeprefix("QGC_")
        assert value == f"${{{{ env.MACOS_SIGN == 'true' && secrets.{secret} || '' }}}}"
    for name, step in steps.items():
        if "secrets.MACOS_" in str(step):
            assert name in {"Import Code Signing Certificate", "Create app bundle"}
