"""Docs workflow contracts."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_step, load_action, load_workflow


def test_docs_build_base_matches_the_deployed_directory() -> None:
    # config.mjs builds the site for /<BRANCH_NAME>/ and deploy-docs publishes it under
    # <github.ref_name>/; without the variable every asset URL misses the branch directory.
    config = (REPO_ROOT / "docs/.vitepress/config.mjs").read_text(encoding="utf-8")
    assert re.search(r"^\s*base:.*\bprocess\.env\.BRANCH_NAME\b", config, re.MULTILINE)
    jobs = load_workflow("docs.yml")["jobs"]
    build = get_step(jobs["build"]["steps"], "Build with VitePress")
    assert build["env"]["BRANCH_NAME"] == "${{ github.ref_name }}"
    deploy = next(step for step in jobs["deploy"]["steps"] if "deploy-docs" in step.get("uses", ""))
    assert "source-branch" not in deploy["with"]
    action = load_action("deploy-docs")
    assert "source-branch" not in action["inputs"]
    deploy_script = get_step(action["runs"]["steps"], "Deploy docs")["run"]
    assert '--branch "${GITHUB_REF_NAME}"' in deploy_script


def test_docs_build_runs_the_docs_npm_project() -> None:
    # package.json, its lock file, and .nvmrc live in docs/, next to the VitePress site.
    steps = load_workflow("docs.yml")["jobs"]["build"]["steps"]
    node = next(step for step in steps if step.get("uses", "").startswith("actions/setup-node@"))
    assert node["with"]["node-version-file"] == "docs/.nvmrc"
    for name in ("Install dependencies", "Build with VitePress"):
        assert get_step(steps, name)["working-directory"] == "docs"
    assert get_step(steps, "Upload artifact")["with"]["path"] == "docs/.vitepress/dist/"
