"""Test result discovery must also work outside GITHUB_WORKSPACE."""

from __future__ import annotations

import os
import subprocess
from typing import TYPE_CHECKING

import pytest
import yaml
from _helpers import REPO_ROOT

if TYPE_CHECKING:
    from pathlib import Path


@pytest.mark.parametrize("exists", [False, True])
def test_codecov_results_outside_workspace(tmp_path: Path, exists: bool) -> None:
    action = yaml.safe_load(
        (REPO_ROOT / ".github/actions/test-report/action.yml").read_text(encoding="utf-8")
    )
    steps = action["runs"]["steps"]
    probe = next(step for step in steps if step.get("id") == "codecov-results")
    upload = next(step for step in steps if step.get("uses", "").startswith("codecov/"))
    assert "inputs.upload-codecov == 'true'" in probe["if"]
    assert "steps.codecov-results.outputs.available == 'true'" in upload["if"]
    assert "hashFiles" not in upload["if"]
    workspace = tmp_path / "workspace"
    workspace.mkdir()
    junit = tmp_path / "runner temp" / "build" / "junit-results.xml"
    junit.parent.mkdir(parents=True)
    if exists:
        junit.write_text("<testsuites/>", encoding="utf-8")
    output = tmp_path / "outputs"
    subprocess.run(
        ["bash", "-e", "-c", probe["run"]],
        cwd=workspace,
        env={
            **os.environ,
            "GITHUB_WORKSPACE": str(workspace),
            "GITHUB_OUTPUT": str(output),
            "JUNIT_PATH": str(junit),
        },
        check=True,
    )
    assert output.read_text(encoding="utf-8") == f"available={str(exists).lower()}\n"
