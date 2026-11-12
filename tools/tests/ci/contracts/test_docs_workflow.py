"""Docs workflow contracts."""

from __future__ import annotations

import re

from ..._helpers import REPO_ROOT, get_steps, load_action, load_workflow


def test_docs_build_base_matches_the_deployed_directory() -> None:
    # config.mjs builds the site for /<BRANCH_NAME>/ and deploy-docs publishes it under
    # <github.ref_name>/; without the variable every asset URL misses the branch directory.
    config = (REPO_ROOT / "docs/.vitepress/config.mjs").read_text(encoding="utf-8")
    assert re.search(r"^\s*base:.*\bprocess\.env\.BRANCH_NAME\b", config, re.MULTILINE)
    build = get_steps(load_workflow("docs.yml"), "build")
    assert any(
        (step.get("env") or {}).get("BRANCH_NAME") == "${{ github.ref_name }}" for step in build
    )
    deploy = [step.get("run", "") for step in get_steps(load_action("deploy-docs"))]
    assert any('--branch "${GITHUB_REF_NAME}"' in script for script in deploy)


def test_doxygen_pull_requests_may_skip_sources_only_while_warnings_stay_non_fatal() -> None:
    # With WARN_AS_ERROR a source change could fail the build, which pull requests that change
    # only sources never run.
    paths = load_workflow("doxygen.yml")["on"]["pull_request"]["paths"]
    doxyfile = (REPO_ROOT / "tools/configs/Doxyfile").read_text(encoding="utf-8")
    fatal = re.search(r"^\s*WARN_AS_ERROR\s*=\s*(YES|FAIL_ON_WARNINGS)", doxyfile, re.MULTILINE)
    assert not fatal or any(path.startswith("src/") for path in paths)
