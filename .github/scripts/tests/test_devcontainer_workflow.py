"""Keep development-image publication separate from application builds."""

from pathlib import Path

import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = yaml.safe_load((ROOT / ".github/workflows/docker.yml").read_text())
JOB = WORKFLOW["jobs"]["devcontainer"]


def test_development_image_publishes_only_on_upstream_master_pushes():
    assert JOB["if"] == "github.event_name != 'push' || github.ref == 'refs/heads/master'"
    assert JOB["env"]["PUBLISH_IMAGE"] == (
        "${{ github.repository == 'mavlink/qgroundcontrol' && "
        "github.event_name == 'push' && github.ref == 'refs/heads/master' }}"
    )
    login = next(step for step in JOB["steps"] if step["uses"].startswith("docker/login-action@"))
    assert login["if"] == "env.PUBLISH_IMAGE == 'true'"
    assert login["with"]["registry"] == "ghcr.io"
    assert not any("DOCKERHUB" in str(step) for step in JOB["steps"])


def test_published_identity_is_distinct_from_application_builder_tags():
    metadata = next(
        step for step in JOB["steps"] if step["uses"].startswith("docker/metadata-action@")
    )
    assert metadata["with"]["images"] == "ghcr.io/mavlink/qgroundcontrol"
    assert metadata["with"]["flavor"] == "latest=false"
    assert metadata["with"]["tags"].splitlines() == [
        "type=raw,value=devcontainer",
        "type=sha,prefix=devcontainer-,format=long,enable=${{ env.PUBLISH_IMAGE == 'true' }}",
    ]


def test_publication_builds_only_the_devcontainer_and_preserves_cache_policy():
    build = next(
        step for step in JOB["steps"] if step["uses"].startswith("docker/build-push-action@")
    )
    inputs = build["with"]
    assert inputs["target"] == "devcontainer"
    assert inputs["platforms"] == "linux/amd64"
    assert inputs["push"] == "${{ env.PUBLISH_IMAGE == 'true' }}"
    assert inputs["load"] is True
    assert inputs["cache-from"] == "type=gha,version=2,scope=qgc-docker-devcontainer-devcontainer"
    assert inputs["cache-to"] == (
        "${{ github.event_name != 'pull_request' && "
        "'type=gha,version=2,scope=qgc-docker-devcontainer-devcontainer,mode=max' || '' }}"
    )
    assert not any("run" in step for step in JOB["steps"])
    assert not any(step["uses"] == "./.github/actions/docker" for step in JOB["steps"])


def test_no_workflow_job_consumes_the_development_image():
    for path in (ROOT / ".github/workflows").glob("*.yml"):
        workflow = yaml.safe_load(path.read_text())
        for job in workflow.get("jobs", {}).values():
            assert "devcontainer" not in str(job.get("container", ""))
    for name, job in WORKFLOW["jobs"].items():
        if name != "devcontainer":
            assert "devcontainer" not in yaml.safe_dump(job)
