"""Keep versioned analysis-image releases independent of application builds."""

import os
import shutil
import subprocess
from pathlib import Path

import pytest
import yaml

ROOT = Path(__file__).resolve().parents[3]
WORKFLOW = yaml.safe_load((ROOT / ".github/workflows/analysis-image.yml").read_text())
JOB = WORKFLOW["jobs"]["image"]
VERSION_SCRIPT = next(step["run"] for step in JOB["steps"] if step.get("id") == "version")


def test_image_has_its_own_release_tags_and_workflow():
    triggers = WORKFLOW.get("on", WORKFLOW.get(True))
    assert triggers["push"] == {"tags": ["analysis-image-v*"]}
    assert "workflow_dispatch" in triggers
    assert ".github/build-config.json" in triggers["pull_request"]["paths"]
    assert "deploy/docker/**" in triggers["pull_request"]["paths"]
    assert JOB["env"]["PUBLISH_IMAGE"] == (
        "${{ github.repository == 'mavlink/qgroundcontrol' && "
        "github.event_name == 'push' && startsWith(github.ref, 'refs/tags/analysis-image-v') }}"
    )
    login = next(
        step for step in JOB["steps"] if step.get("uses", "").startswith("docker/login-action@")
    )
    assert login["if"] == "env.PUBLISH_IMAGE == 'true'"
    assert login["with"]["registry"] == "ghcr.io"
    assert not any("DOCKERHUB" in str(step) for step in JOB["steps"])


def test_published_identity_supports_version_and_digest_pins():
    metadata = next(step for step in JOB["steps"] if step.get("id") == "meta")
    assert metadata["with"]["images"] == "ghcr.io/mavlink/qgroundcontrol-analysis"
    assert metadata["with"]["flavor"] == "latest=false"
    assert metadata["with"]["tags"].splitlines() == [
        "type=raw,value=${{ steps.version.outputs.version }}",
        "type=sha,prefix=sha-,format=long,enable=${{ env.PUBLISH_IMAGE == 'true' }}",
    ]
    summary = JOB["steps"][-1]
    assert summary["env"]["IMAGE_DIGEST"] == "${{ steps.image.outputs.digest }}"
    assert "ghcr.io/mavlink/qgroundcontrol-analysis@%s" in summary["run"]


def test_publication_builds_only_tooling_and_preserves_cache_policy():
    build = next(step for step in JOB["steps"] if step.get("id") == "image")
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
    assert not any(step.get("uses") == "./.github/actions/docker" for step in JOB["steps"])
    assert "cmake --build" not in "\n".join(step.get("run", "") for step in JOB["steps"])
    assert "install_analysis.py" not in yaml.safe_dump(JOB)


def test_no_workflow_job_consumes_the_analysis_image():
    for path in (ROOT / ".github/workflows").glob("*.yml"):
        workflow = yaml.safe_load(path.read_text())
        for job in workflow.get("jobs", {}).values():
            container = str(job.get("container", ""))
            assert "devcontainer" not in container
            assert "qgroundcontrol-analysis" not in container
    application = (ROOT / ".github/workflows/docker.yml").read_text()
    assert "devcontainer" not in application
    assert "analysis-image" not in application


@pytest.mark.skipif(os.name == "nt" or not shutil.which("bash"), reason="Release step uses Bash")
@pytest.mark.parametrize(
    ("publish", "tag", "merged", "expected"),
    [
        ("true", "analysis-image-v1.2.3", True, "1.2.3"),
        ("true", "v1.2.3", True, None),
        ("true", "analysis-image-vlatest", True, None),
        ("true", "analysis-image-v1.2.3", False, None),
        ("false", "feature-branch", False, "validation"),
    ],
)
def test_release_version_step_requires_a_merged_versioned_tag(
    tmp_path, publish, tag, merged, expected
):
    subprocess.run(["git", "init", "-q", str(tmp_path)], check=True)
    commit = [
        "git",
        "-c",
        "user.name=Test",
        "-c",
        "user.email=test@example.com",
        "-c",
        "commit.gpgsign=false",
        "commit",
        "-q",
        "--allow-empty",
        "-m",
    ]
    subprocess.run([*commit, "base"], cwd=tmp_path, check=True)
    sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=tmp_path, text=True).strip()
    subprocess.run(
        ["git", "update-ref", "refs/remotes/origin/master", sha], cwd=tmp_path, check=True
    )
    if not merged:
        subprocess.run([*commit, "unmerged"], cwd=tmp_path, check=True)
        sha = subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=tmp_path, text=True).strip()
    output = tmp_path / "output"
    result = subprocess.run(
        ["bash", "-e", "-o", "pipefail", "-c", VERSION_SCRIPT],
        cwd=tmp_path,
        env={
            **os.environ,
            "PUBLISH_IMAGE": publish,
            "REF_NAME": tag,
            "GITHUB_SHA": sha,
            "GITHUB_OUTPUT": str(output),
        },
        capture_output=True,
        text=True,
    )
    if expected is None:
        assert result.returncode != 0
        assert not output.exists()
    else:
        assert result.returncode == 0, result.stderr
        assert output.read_text() == f"version={expected}\n"
