"""Contracts for workflow conditions written as GitHub expressions instead of shell steps."""

from __future__ import annotations

from ..._helpers import get_steps, load_workflow


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
    conditions = {step["name"]: step.get("if") for step in job["steps"]}
    assert conditions["Upload sources to Crowdin"] == "env.SYNC_MODE == 'upload'"
    assert conditions["Download translations from Crowdin"] == "env.SYNC_MODE == 'download'"


def test_codeql_languages_build_without_compilation() -> None:
    job = load_workflow("codeql.yml")["jobs"]["analyze"]
    assert job["strategy"]["matrix"] == {"language": ["actions", "java-kotlin", "python"]}
    (init,) = (step for step in job["steps"] if step.get("name") == "Initialize CodeQL")
    assert init["with"]["build-mode"] == "none"
