"""Static contracts for managed RunsOn images and opt-in runner routing."""

from __future__ import annotations

import re
from typing import Any

import pytest
import yaml
from _helpers import REPO_ROOT
from android_matrix import LINUX_EMULATOR_JOB, build_matrix


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def _load_yaml(path: str) -> dict[str, Any]:
    document = yaml.safe_load(_read(path))
    assert isinstance(document, dict)
    if True in document and "on" not in document:
        document["on"] = document.pop(True)
    return document


def test_custom_linux_image_is_declared() -> None:
    config = _load_yaml(".github/runs-on.yml")

    assert config["images"]["qgc-ubuntu24-x64"] == {
        "platform": "linux",
        "arch": "x64",
        "name": "qgc-runs-on-ubuntu24-x64-*",
    }
    assert config["runners"]["linux-x64-builder-prebaked"]["image"] == "qgc-ubuntu24-x64"


def test_runner_pools_have_on_demand_capacity_and_cache() -> None:
    config = _load_yaml(".github/runs-on.yml")
    for pool, runner in config["runners"].items():
        assert runner["family"]
        assert all(isinstance(family, str) and family for family in runner["family"])
        for resource in ("cpu", "ram"):
            assert runner[resource]
            assert all(type(value) is int and value > 0 for value in runner[resource])
        assert runner["spot"] is False
        assert "s3-cache" in runner["extras"]
        assert re.fullmatch(r"[1-9]\d*gb", runner["volume"])
        platform, arch = pool.split("-")[:2]
        image = runner["image"]
        if image in config["images"]:
            assert config["images"][image]["platform"] == platform
            assert config["images"][image]["arch"] == arch
        else:
            assert image.endswith(f"-{arch}")
            assert image.startswith("windows" if platform == "windows" else "ubuntu")


@pytest.mark.parametrize(
    "pool",
    [
        "linux-x64-builder-60gb",
        "linux-x64-builder-prebaked",
        "linux-x64-tester",
        "linux-x64-vm-builder",
        "windows-x64-builder",
    ],
)
def test_memory_heavy_pools_retain_a_16_gib_floor(pool: str) -> None:
    assert min(_load_yaml(".github/runs-on.yml")["runners"][pool]["ram"]) >= 16


@pytest.mark.parametrize("pool", ["linux-x64-emulator", "linux-x64-vm-builder"])
def test_virtualized_pools_require_nested_virtualization(pool: str) -> None:
    runner = _load_yaml(".github/runs-on.yml")["runners"][pool]
    assert runner["nested-virt"] is True
    assert runner["image"].endswith("-x64")


def test_vm_builds_do_not_use_the_smaller_android_emulator_pool() -> None:
    workflow = _load_yaml(".github/workflows/vm-builds.yml")
    assert LINUX_EMULATOR_JOB["runson_runner"] == "linux-x64-emulator"
    for job in workflow["jobs"].values():
        route = job["runs-on"]
        assert "/runner=linux-x64-vm-builder" in route
        assert "linux-x64-emulator" not in route
        assert "github.repository_owner == 'mavlink'" in route
        assert "github.event_name != 'pull_request'" in route
        assert "|| 'ubuntu-latest'" in route
        cache_step = next(step for step in job["steps"] if step.get("uses") == "runs-on/action@v2")
        assert "if" not in cache_step
    for event in ("push", "pull_request"):
        assert ".github/runs-on.yml" in workflow["on"][event]["paths"]


@pytest.mark.parametrize(
    "workflow",
    [
        "linux.yml",
        "custom-build.yml",
        "docker.yml",
        "android.yml",
        "windows.yml",
        "runner-images.yml",
        "vm-builds.yml",
    ],
)
def test_workflows_use_central_runner_definitions(workflow: str) -> None:
    text = _read(f".github/workflows/{workflow}")
    assert "runner=" in text
    assert not re.search(r"/(?:family|cpu|ram|spot|image|volume|extras|nested-virt)=", text)
    configured = _load_yaml(".github/runs-on.yml")["runners"]
    references = re.findall(r"(?:linux|windows)-(?:x64|arm64)-[\w-]+", text)
    for pool in references:
        assert pool in configured


def test_android_matrix_references_defined_runners() -> None:
    configured = _load_yaml(".github/runs-on.yml")["runners"]
    for leg in build_matrix(is_pr=False):
        if pool := leg["runson_runner"]:
            assert pool in configured


def test_runner_image_workflow_uses_current_commit_and_dedicated_role() -> None:
    workflow = _load_yaml(".github/workflows/runner-images.yml")
    workflow_text = _read(".github/workflows/runner-images.yml")

    assert "workflow_dispatch" in workflow["on"]
    assert workflow["on"]["schedule"]
    assert workflow["jobs"]["promote"]["needs"] == ["ubuntu24-x64", "smoke-test"]
    assert workflow["permissions"] == {"contents": "read"}
    assert workflow["jobs"]["ubuntu24-x64"]["permissions"] == {
        "contents": "read",
        "id-token": "write",
    }
    assert "RUNS_ON_AMI_ROLE_ARN" in workflow_text
    assert "secrets.AWS_ROLE_ARN" not in workflow_text
    assert "PKR_VAR_source_ref: ${{ github.sha }}" in workflow_text
    assert "hashicorp/setup-packer@v3" in workflow_text
    assert "version: '1.16.0'" in workflow_text
    assert "github.event.repository.default_branch" in workflow_text


def test_runner_image_workflow_smoke_tests_the_managed_image() -> None:
    workflow = _load_yaml(".github/workflows/runner-images.yml")
    smoke_job = workflow["jobs"]["smoke-test"]
    smoke_steps = str(smoke_job["steps"])

    assert smoke_job["needs"] == "ubuntu24-x64"
    assert "linux-x64-builder-prebaked" in str(smoke_job["runs-on"])
    assert "ami={1}" in smoke_job["runs-on"]
    assert "needs.ubuntu24-x64.outputs.image" in smoke_job["runs-on"]
    assert "qt-cmake" in smoke_steps
    assert ".qgc-modules" in smoke_steps
    assert "/opt/qgc-sdk/cpm/manifest.json" in smoke_steps
    assert "/opt/qgc-sdk/cpm/sources" in smoke_steps
    assert "libgstreamer1.0-dev" in smoke_steps


def test_ci_scripts_validates_the_packer_template() -> None:
    workflow = _load_yaml(".github/workflows/ci-scripts.yml")
    validation_job = workflow["jobs"]["validate-runner-images"]
    validation_steps = str(validation_job["steps"])

    assert validation_job["runs-on"] == "ubuntu-latest"
    assert "hashicorp/setup-packer@v3" in validation_steps
    assert "'1.16.0'" in validation_steps
    assert "packer fmt -check" in validation_steps
    assert "packer init" in validation_steps
    assert "packer validate" in validation_steps


def test_linux_image_uses_repository_setup_helpers_and_module_manifest() -> None:
    provision = _read(".github/runner-images/provision_linux.py")
    packer = _read(".github/runner-images/qgc-ubuntu24-x64.pkr.hcl")

    assert '"tools/setup/install_dependencies"' in provision
    assert '"tools/setup/install_qt.py"' in provision
    assert ".qgc-modules" in provision
    assert '"create-seed"' in provision
    assert "QGC_PREINSTALLED_CPM_DIR" in provision
    assert 'owners      = ["135269210855"]' in packer
    assert 'name                = "runs-on-v2.2-ubuntu24-full-x64-*"' in packer


def test_managed_runner_routes_are_opt_in() -> None:
    for workflow in ("linux.yml", "custom-build.yml", "docker.yml", "android.yml"):
        assert "vars.RUNS_ON_LINUX_BUILDER" in _read(f".github/workflows/{workflow}")

    for workflow in ("windows.yml", "android.yml"):
        assert "vars.RUNS_ON_WINDOWS_POOL" in _read(f".github/workflows/{workflow}")

    warm_pool = _load_yaml(".github/runner-images/windows-warm-pool.example.yml")
    pool = warm_pool["pools"]["qgc-windows-x64-builder"]
    assert pool["runner"] == "windows-x64-builder"
    assert pool["schedule"]
    assert (
        warm_pool["runners"]["windows-x64-builder"]
        == _load_yaml(".github/runs-on.yml")["runners"]["windows-x64-builder"]
    )


def test_cpp_codeql_wraps_native_build():
    workflow = _load_yaml(".github/workflows/linux.yml")
    jobs = [
        job
        for job in workflow["jobs"].values()
        if any(
            step.get("uses", "").startswith("github/codeql-action/init@")
            for step in job.get("steps", [])
        )
    ]
    assert len(jobs) == 1
    steps = jobs[0]["steps"]
    init = next(
        i
        for i, step in enumerate(steps)
        if step.get("uses", "").startswith("github/codeql-action/init@")
    )
    analyze = next(
        i
        for i, step in enumerate(steps)
        if step.get("uses", "").startswith("github/codeql-action/analyze@")
    )
    build = next(
        i for i, step in enumerate(steps) if step.get("uses") == "./.github/actions/cmake-build"
    )
    assert init < build < analyze
    assert steps[init]["with"]["languages"] == "c-cpp"
