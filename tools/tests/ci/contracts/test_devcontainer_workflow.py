"""Dev Container workflow contracts: what triggers it, when it publishes, and what the editor reuses."""

from __future__ import annotations

import json
import re

from ..._helpers import REPO_ROOT, evaluate_expression, load_workflow
from .._workflows import path_filter_covers

WORKFLOW = load_workflow("devcontainer.yml")
BUILD = WORKFLOW["jobs"]["build"]["steps"]
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
    paths = WORKFLOW["on"]["push"]["paths"]
    sources = _image_sources()
    assert sources
    for source in [
        *sources,
        ".devcontainer/devcontainer.json",
        ".dockerignore",
        ".github/workflows/devcontainer.yml",
    ]:
        assert path_filter_covers(source, paths), (
            f"{source} changes the image but does not trigger devcontainer.yml"
        )


def test_only_upstream_master_and_release_tags_publish() -> None:
    (policy,) = (step for step in BUILD if "PUBLISH" in (step.get("env") or {}))
    publish = policy["env"]["PUBLISH"]

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


def test_devcontainer_reuses_the_published_image_as_a_cache() -> None:
    build = json.loads((REPO_ROOT / ".devcontainer/devcontainer.json").read_text())["build"]
    assert build["cacheFrom"] == f"{WORKFLOW['env']['IMAGE']}:latest"
    targets = {step["with"]["target"] for step in BUILD if "target" in (step.get("with") or {})}
    assert targets == {build["target"]}
