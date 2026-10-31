"""Every packaging workflow reports its package against master's artifact of the same name."""

from __future__ import annotations

import re

import pytest

from ..._helpers import get_step, load_action, load_workflow

REPORT = "./.github/actions/artifact-report"
# Workflow, the step that creates the package, the step that uploads it, and the GPL findings mode:
# only the AppImage's known issue (Ubuntu's libavcodec bundles x264 and x265) warns.
PACKAGES = [
    ("windows.yml", "Create Installer", "Attest and Upload", "fail"),
    ("macos.yml", "Create app bundle", "Attest and Upload", "fail"),
    ("linux.yml", "Create AppImage", "Attest and Upload", "warn"),
    ("android.yml", "Prepare Artifact", "Attest and Upload", "fail"),
    ("ios.yml", "Package IPA", "Attest and Upload", "fail"),
]


@pytest.mark.parametrize(("workflow", "creator", "uploader", "gpl_findings"), PACKAGES)
def test_report_follows_the_package_and_compares_the_uploaded_artifact(
    workflow: str, creator: str, uploader: str, gpl_findings: str
) -> None:
    job = load_workflow(workflow)["jobs"]["build"]
    steps = job["steps"]
    (index,) = [index for index, step in enumerate(steps) if step.get("uses") == REPORT]
    report, upload = steps[index], get_step(steps, uploader)
    # Right after the package exists (only package checks in between) and before it is uploaded.
    assert steps.index(get_step(steps, creator)) < index < steps.index(upload)
    assert all(
        "uses" not in step for step in steps[steps.index(get_step(steps, creator)) + 1 : index]
    )

    assert report.get("if") == upload.get("if")
    assert report["with"]["artifact-name"] == upload["with"]["package-name"]
    assert report["with"]["package"].endswith("/" + upload["with"]["artifact-name"])
    assert report["with"].get("gpl-findings", "fail") == gpl_findings
    # The baseline lookup lists other runs' artifacts.
    assert job["permissions"]["actions"] == "read"


def test_action_passes_the_token_and_inputs_the_script_reads() -> None:
    action = load_action("artifact-report")
    (step,) = action["runs"]["steps"]
    assert step["run"] == 'python3 "${GITHUB_WORKSPACE}/tools/ci/packaging/artifact_report.py"'
    inputs = {name.replace("-", "_").upper() for name in action["inputs"]}
    assert set(step["env"]) == {"GH_TOKEN", *(f"INPUT_{name}" for name in inputs)}
    for name, value in step["env"].items():
        if name.startswith("INPUT_"):
            input_name = re.fullmatch(r"\$\{\{ inputs\.([\w-]+) \}\}", value)
            assert input_name is not None
            assert input_name.group(1).replace("-", "_").upper() == name.removeprefix("INPUT_")
    assert step["env"]["GH_TOKEN"] == "${{ github.token }}"
