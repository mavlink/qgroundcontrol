"""Dev Container workflow contracts: what triggers it, and when it publishes."""

from __future__ import annotations

import json
import re

from ..._helpers import REPO_ROOT, evaluate_expression, get_step, load_workflow

WORKFLOW = load_workflow("devcontainer.yml")
BUILD = WORKFLOW["jobs"]["build"]["steps"]
PUBLISH = WORKFLOW["jobs"]["publish"]


DOCKERFILE = (REPO_ROOT / "deploy/docker/Dockerfile").read_text(encoding="utf-8")


def _image_sources() -> list[str]:
    """Repository paths the Dockerfile copies into the stages the devcontainer target builds on."""
    sources = []
    for stage in re.split(r"^FROM ", DOCKERFILE, flags=re.MULTILINE):
        name = stage.split("\n", 1)[0].split(" AS ")[-1].strip()
        if name not in {"base", "linux"} and not name.startswith("devcontainer"):
            continue
        for line in re.findall(r"^COPY (?!--from=)(.+)$", stage, re.MULTILINE):
            sources += [word for word in line.split() if not word.startswith("--")][:-1]
    return sources


def test_image_inputs_trigger_builds() -> None:
    on = WORKFLOW["on"]
    paths = on["push"]["paths"]
    assert on["pull_request"]["paths"] == paths
    sources = _image_sources()
    assert {"tools/setup/install_analysis_tools.py", "deploy/docker/install_android_sdk.sh"} <= set(
        sources
    )
    for source in [
        *sources,
        ".devcontainer/devcontainer.json",
        ".dockerignore",
        ".github/workflows/devcontainer.yml",
    ]:
        assert any(
            source == path or (path.endswith("/**") and source.startswith(path[:-2]))
            for path in paths
        ), f"{source} changes the image but does not trigger devcontainer.yml"


def test_only_upstream_master_and_release_tags_publish() -> None:
    publish = get_step(BUILD, "Decide publishing")["env"]["PUBLISH"]

    def decides(event: str, ref: str, repository: str = "mavlink/qgroundcontrol") -> bool:
        context = {"github": {"event_name": event, "ref": ref, "repository": repository}}
        # startsWith is the only function call; resolve it for the evaluator.
        expression = publish.replace(
            "startsWith(github.ref, 'refs/tags/v')", str(ref.startswith("refs/tags/v")).lower()
        )
        return evaluate_expression(expression, context) is True

    assert decides("push", "refs/heads/master")
    assert decides("push", "refs/tags/v5.1.0")
    assert not decides("pull_request", "refs/pull/1/merge")
    assert not decides("workflow_dispatch", "refs/heads/master")
    assert not decides("push", "refs/heads/Stable_V5.0")
    assert not decides("push", "refs/heads/master", repository="fork/qgroundcontrol")
    for name in (
        "Login to GHCR",
        "Push image by digest",
        "Upload image digest",
        "Upload image SBOM",
    ):
        assert get_step(BUILD, name)["if"] == "steps.policy.outputs.publish == 'true'"
    assert PUBLISH["if"] == "needs.build.outputs.publish == 'true'"
    assert WORKFLOW["jobs"]["attest-sboms"]["if"] == PUBLISH["if"]
    # Checking devcontainer.json must never push, even on the runs that publish.
    assert get_step(BUILD, "Test devcontainer.json")["with"]["push"] == "never"


def test_registry_attestations_raise_node_s_header_limit() -> None:
    """GHCR's responses overflow Node's default header limit when attestations are pushed."""
    attests = [
        step
        for job in (PUBLISH, WORKFLOW["jobs"]["attest-sboms"])
        for step in job["steps"]
        if step.get("uses") == "actions/attest@v4"
    ]
    assert len(attests) == 2
    for step in attests:
        assert step["with"]["push-to-registry"] is True
        assert step["env"]["NODE_OPTIONS"] == "--max-http-header-size=32768"


def test_master_publishes_latest_and_tags_their_version() -> None:
    tags = get_step(PUBLISH["steps"], "Image tags")["with"]["tags"].splitlines()
    assert "type=raw,value=latest,enable=${{ github.ref == 'refs/heads/master' }}" in tags
    assert "type=ref,event=tag" in tags
    assert get_step(PUBLISH["steps"], "Image tags")["with"]["flavor"] == "latest=false"


def test_devcontainer_reuses_the_published_image_as_a_cache() -> None:
    devcontainer = json.loads((REPO_ROOT / ".devcontainer/devcontainer.json").read_text())
    assert devcontainer["build"]["target"] == "devcontainer"
    assert devcontainer["build"]["cacheFrom"] == f"{WORKFLOW['env']['IMAGE']}:latest"
    push = get_step(BUILD, "Push image by digest")["with"]
    assert push["cache-to"] == "type=inline"
    assert push["target"] == get_step(BUILD, "Build image")["with"]["target"] == "devcontainer"


def test_each_image_runs_the_smoke_test_from_this_checkout() -> None:
    smoke = get_step(BUILD, "Smoke test image")["run"]
    assert '-v "${GITHUB_WORKSPACE}:/workspaces/qgroundcontrol:ro" qgc-dev:ci' in smoke
    assert smoke.endswith("bash -lc 'python3 deploy/docker/smoke_devcontainer.py'")
    assert (REPO_ROOT / "deploy/docker/smoke_devcontainer.py").is_file()
    # Publishing pushes the same build only after the smoke test passes.
    names = [step["name"] for step in BUILD]
    assert names.index("Smoke test image") < names.index("Push image by digest")
