"""Static contracts for managed RunsOn images and opt-in runner routing."""

from __future__ import annotations

import os
import re
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest
from ci.planning.android_matrix import LINUX_EMULATOR_JOB, build_matrix
from ci.runner import validate_runner_configs
from common.build_config import load_build_config, lookup_dotted

from ..._helpers import REPO_ROOT, load_script, load_yaml

if TYPE_CHECKING:
    from collections.abc import Iterator

RUNSON_WORKFLOWS = [
    "analysis.yml",
    "android.yml",
    "docker.yml",
    "linux.yml",
    "vm-builds.yml",
    "windows.yml",
]


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def _normalized(text: str) -> str:
    return " ".join(text.split())


def _instance_shape(instance_type: str) -> tuple[int, int]:
    match = re.fullmatch(r"([cmr])\d+[aig]\.(\d*)xlarge", instance_type)
    assert match, f"{instance_type} must be an exact c/m/r xlarge-class instance type"
    vcpus = 4 * int(match[2] or 1)
    return vcpus, vcpus * {"c": 2, "m": 4, "r": 8}[match[1]]


def _runson_routes(workflow: str) -> Iterator[tuple[str, str]]:
    for job_id, job in load_yaml(f".github/workflows/{workflow}")["jobs"].items():
        # Matrix values cannot read strategy.job-index, so labels belong in the job's runs-on.
        assert "runs-on=" not in str(job.get("strategy", {})), f"{workflow}:{job_id}"
        if "runs-on=" in str(job.get("runs-on", "")):
            yield job_id, job["runs-on"]


def test_custom_linux_image_is_declared() -> None:
    config = load_yaml(".github/runs-on.yml")

    assert config["images"]["qgc-ubuntu24-x64"] == {
        "platform": "linux",
        "arch": "x64",
        "name": "qgc-runs-on-ubuntu24-x64-*",
    }
    assert config["runners"]["linux-x64-builder-prebaked"]["image"] == "qgc-ubuntu24-x64"


def test_runner_pools_have_on_demand_capacity_and_cache() -> None:
    config = load_yaml(".github/runs-on.yml")
    for pool, runner in config["runners"].items():
        assert runner["family"]
        assert all(isinstance(family, str) and family for family in runner["family"])
        for resource in ("cpu", "ram"):
            assert runner[resource]
            assert all(type(value) is int and value > 0 for value in runner[resource])
        # On-demand only: a reclaimed Spot instance discards a whole build.
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
    assert min(load_yaml(".github/runs-on.yml")["runners"][pool]["ram"]) >= 16


def test_analysis_pool_provides_16_cpus_and_matching_memory() -> None:
    runner = load_yaml(".github/runs-on.yml")["runners"]["linux-x64-analyzer"]
    assert runner["cpu"] == [16]
    assert runner["family"][0] == "m8i.4xlarge"
    # Analysis peaks near 11 GiB, so the 32 GiB c8i fallback keeps ample headroom.
    assert min(runner["ram"]) >= 32


def test_runner_pool_families_are_prioritized_exact_types_matching_their_shape() -> None:
    # Exact types make on-demand launches follow list order, and RunsOn then ignores cpu/ram.
    for pool, runner in load_yaml(".github/runs-on.yml")["runners"].items():
        families = runner["family"]
        assert len(families) >= 2, f"{pool} needs an on-demand fallback type"
        assert len(set(families)) == len(families)
        for instance_type in families:
            vcpus, memory = _instance_shape(instance_type)
            assert vcpus in runner["cpu"], f"{pool}: {instance_type}"
            assert min(runner["ram"]) <= memory <= max(runner["ram"]), f"{pool}: {instance_type}"
            is_graviton = instance_type.split(".")[0].endswith("g")
            assert is_graviton == pool.startswith("linux-arm64-"), f"{pool}: {instance_type}"


@pytest.mark.parametrize("pool", ["linux-x64-emulator", "linux-x64-vm-builder"])
def test_virtualized_pools_require_nested_virtualization(pool: str) -> None:
    runner = load_yaml(".github/runs-on.yml")["runners"][pool]
    assert runner["nested-virt"] is True
    assert runner["image"].endswith("-x64")
    # EC2 exposes nested virtualization only on Intel 7i/8i families, not AMD or Graviton.
    assert all(re.fullmatch(r"[cmr][78]i\.\w+", family) for family in runner["family"])


def test_vm_builds_do_not_use_the_smaller_android_emulator_pool() -> None:
    workflow = load_yaml(".github/workflows/vm-builds.yml")
    assert LINUX_EMULATOR_JOB["runson_runner"] == "linux-x64-emulator"
    for job in workflow["jobs"].values():
        route = job["runs-on"]
        assert "/runner=linux-x64-vm-builder" in route
        assert "linux-x64-emulator" not in route
        assert "github.repository_owner == 'mavlink'" in route
        assert "github.event" not in route
        assert "/spot=" not in route
        assert "|| 'ubuntu-latest'" in route
        cache_step = next(step for step in job["steps"] if step.get("uses") == "runs-on/action@v2")
        assert "if" not in cache_step
    assert ".github/runs-on.yml" in workflow["on"]["push"]["paths"]
    assert ".github/runs-on.yml" not in workflow["on"]["pull_request"]["paths"]


@pytest.mark.parametrize(
    "workflow",
    [
        "linux.yml",
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
    configured = load_yaml(".github/runs-on.yml")["runners"]
    text = _normalized(text)
    if workflow == "windows.yml":
        # RunsOn reads runs-on.yml from the default branch only, so the Windows builder repeats its
        # volume as a label to apply in PRs; any other value or shape label still fails below.
        text = text.replace(f"/volume={configured['windows-x64-builder']['volume']}", "")
    assert not re.search(r"/(?:family|cpu|ram|spot|image|volume|extras|nested-virt)=", text)
    references = re.findall(r"(?:linux|windows)-(?:x64|arm64)-[\w-]+", text)
    for pool in references:
        assert pool in configured


@pytest.mark.parametrize("workflow", RUNSON_WORKFLOWS)
def test_runson_labels_are_unique_per_job_attempt_and_matrix_entry(workflow: str) -> None:
    # GitHub may hand a runner to any queued job with the same label, so matrix siblings sharing one
    # stall while RunsOn replaces stolen runners. github.job is null outside steps, so labels spell
    # out the job id, and strategy.job-index separates matrix entries.
    routes = list(_runson_routes(workflow))
    assert routes
    for job_id, route in routes:
        label = _normalized(route)
        assert "github.job" not in label
        assert f"format('runs-on={{0}}-{job_id}-{{1}}-{{2}}/" in label
        assert "github.run_id, github.run_attempt, strategy.job-index" in label


@pytest.mark.parametrize("workflow", RUNSON_WORKFLOWS)
def test_runson_jobs_never_request_spot(workflow: str) -> None:
    # Every event, pull requests included, runs on the pools' on-demand capacity.
    text = _normalized(_read(f".github/workflows/{workflow}"))
    for token in ("-spot", "/spot=", "RUNS_ON_SPOT_POOLS"):
        assert token not in text, f"{workflow}: {token}"
    for job_id, route in _runson_routes(workflow):
        assert "github.event_name" not in route, f"{workflow}:{job_id}"


def test_android_matrix_references_defined_runners() -> None:
    configured = load_yaml(".github/runs-on.yml")["runners"]
    for leg in build_matrix(is_pr=False):
        if pool := leg["runson_runner"]:
            assert pool in configured


def test_runner_image_workflow_uses_current_commit_and_dedicated_role() -> None:
    workflow = load_yaml(".github/workflows/runner-images.yml")
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
    # One region for credentials, Packer, and runner_image.py (which defaults to $AWS_REGION).
    assert workflow["env"] == {"AWS_REGION": "${{ vars.AWS_REGION || 'us-west-2' }}"}
    assert workflow_text.count("vars.AWS_REGION") == 1
    assert "--region" not in workflow_text

    build_steps = {step["name"]: step for step in workflow["jobs"]["ubuntu24-x64"]["steps"]}
    names = list(build_steps)
    # runner_image.py preflight checks the branch and settings before AWS credentials are requested.
    preflight = build_steps["Preflight"]
    assert preflight["run"] == "python3 tools/ci/runner/runner_image.py preflight"
    assert preflight["env"]["DEFAULT_BRANCH"] == "${{ github.event.repository.default_branch }}"
    assert preflight["env"]["ROLE_ARN"] == "${{ secrets.RUNS_ON_AMI_ROLE_ARN }}"
    assert preflight["env"]["SUBNET_ID"] == "${{ vars.RUNS_ON_AMI_SUBNET_ID }}"
    assert names.index("Preflight") < names.index("Configure AWS Credentials")
    for job in ("ubuntu24-x64", "promote"):
        steps = {step.get("name"): step for step in workflow["jobs"][job]["steps"]}
        assert steps["Configure AWS Credentials"]["with"]["aws-region"] == "${{ env.AWS_REGION }}"


def test_runner_image_workflow_smoke_tests_the_managed_image() -> None:
    workflow = load_yaml(".github/workflows/runner-images.yml")
    smoke_job = workflow["jobs"]["smoke-test"]
    smoke_steps = str(smoke_job["steps"])

    assert smoke_job["needs"] == "ubuntu24-x64"
    assert "linux-x64-builder-prebaked" in str(smoke_job["runs-on"])
    assert "ami={1}" in smoke_job["runs-on"]
    assert "needs.ubuntu24-x64.outputs.image" in smoke_job["runs-on"]
    assert "python3 tools/ci/runner/runner_image.py verify" in smoke_steps

    build_steps = {step["name"]: step for step in workflow["jobs"]["ubuntu24-x64"]["steps"]}
    sparse = build_steps["Checkout"]["with"]["sparse-checkout"].split()
    assert {".github/build-config.json", "tools/setup/read_config.py"} <= set(sparse)
    resolve = build_steps["Resolve build configuration"]
    assert resolve["id"] == "build-config"
    keys = resolve["run"].split("--github-output ")[1].split()
    assert keys == ["qt.version", "qt.modules", "tools.packer.version"]
    config = load_build_config()
    assert all(lookup_dotted(config, key, None) for key in keys)
    packer_env = build_steps["Build runner image"]["env"]
    assert packer_env["PKR_VAR_qt_version"] == "${{ steps.build-config.outputs.qt_version }}"
    assert packer_env["PKR_VAR_qt_modules"] == "${{ steps.build-config.outputs.qt_modules }}"
    # Builds and the packer-validate hook use build-config.json's Packer release.
    packer = build_steps["Setup Packer"]["with"]["version"]
    assert packer == "${{ steps.build-config.outputs.tools_packer_version }}"
    names = list(build_steps)
    assert names.index("Resolve build configuration") < names.index("Setup Packer")
    candidate = build_steps["Resolve candidate AMI"]
    assert candidate["run"].startswith("python3 tools/ci/runner/runner_image.py manifest ")


def _precommit_hook(hook_id: str) -> dict[str, Any]:
    config = load_yaml(".pre-commit-config.yaml")
    return next(hook for repo in config["repos"] for hook in repo["hooks"] if hook["id"] == hook_id)


@pytest.mark.parametrize(
    ("hook_id", "check", "inputs"),
    [
        (
            "packer-validate",
            "packer",
            [f".github/runner-images/{validate_runner_configs.PACKER_TEMPLATE}"],
        ),
        ("runs-on-lint", "runs-on", list(validate_runner_configs.RUNS_ON_CONFIGS)),
    ],
)
def test_runner_config_hooks_validate_their_inputs(
    hook_id: str, check: str, inputs: list[str]
) -> None:
    # Manual-stage hooks that pre_commit.py --ci runs, secret-free so fork pull requests run them.
    hook = _precommit_hook(hook_id)
    assert hook["stages"] == ["manual"]
    assert hook["pass_filenames"] is False
    assert hook["entry"].endswith(f"tools/ci/runner/validate_runner_configs.py {check}")
    for path in [
        *inputs,
        ".github/build-config.json",
        "tools/ci/runner/validate_runner_configs.py",
    ]:
        assert (REPO_ROOT / path).is_file(), path
        assert re.search(hook["files"], path), path


def test_linux_image_uses_repository_setup_helpers_and_module_manifest(
    monkeypatch: pytest.MonkeyPatch,
) -> None:
    provision = load_script(".github/runner-images/provision_linux.py")
    calls: list[tuple[list[str], dict[str, Any]]] = []
    written: dict[Path, str] = {}
    monkeypatch.setattr(
        provision.subprocess, "run", lambda command, **kwargs: calls.append((command, kwargs))
    )
    monkeypatch.setattr(Path, "write_text", lambda self, text: written.setdefault(self, text))
    monkeypatch.setattr(Path, "iterdir", lambda self: iter(()))
    monkeypatch.setenv("PATH", os.environ.get("PATH", ""))
    monkeypatch.setenv("QGC_QT_VERSION", "6.12.0")
    monkeypatch.setenv("QGC_QT_MODULES", "qtcharts qtlocation")

    provision.provision()
    scripts = {
        Path(command[1]).relative_to(REPO_ROOT).as_posix(): command
        for command, _ in calls
        if len(command) > 1 and Path(command[1]).is_relative_to(REPO_ROOT)
    }
    qt_root = Path("/opt/qgc-sdk/Qt/6.12.0/gcc_64")
    assert scripts["tools/setup/install_dependencies"][2:] == ["--platform", "debian"]
    install_qt = scripts["tools/setup/install_qt.py"]
    assert install_qt[install_qt.index("--modules") + 1] == "qtcharts qtlocation"
    assert written == {qt_root / ".qgc-modules": "qtcharts qtlocation\n"}
    seed = scripts["tools/ci/cache/cpm_helper.py"]
    assert seed[2] == "create-seed"
    assert seed[seed.index("--qt-root") + 1] == str(qt_root)
    profile = next(kwargs["input"] for command, kwargs in calls if "/etc/environment" in command)
    assert "QGC_PREINSTALLED_CPM_DIR=/opt/qgc-sdk/cpm" in profile

    packer = _read(".github/runner-images/qgc-ubuntu24-x64.pkr.hcl")
    assert 'owners      = ["135269210855"]' in packer
    assert 'name                = "runs-on-v2.2-ubuntu24-full-x64-*"' in packer


def test_managed_runner_routes_are_opt_in() -> None:
    for workflow in ("linux.yml", "docker.yml", "android.yml"):
        assert "vars.RUNS_ON_LINUX_BUILDER" in _read(f".github/workflows/{workflow}")

    for workflow in ("windows.yml", "android.yml"):
        assert "vars.RUNS_ON_WINDOWS_POOL" in _read(f".github/workflows/{workflow}")

    warm_pool = load_yaml(".github/runner-images/windows-warm-pool.example.yml")
    pool = warm_pool["pools"]["qgc-windows-x64-builder"]
    assert pool["runner"] == "windows-x64-builder"
    assert pool["schedule"]
    assert (
        warm_pool["runners"]["windows-x64-builder"]
        == load_yaml(".github/runs-on.yml")["runners"]["windows-x64-builder"]
    )
