"""Static contracts for managed RunsOn images and opt-in runner routing."""

from __future__ import annotations

import os
import re
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest
from ci.planning.android_matrix import LINUX_EMULATOR_JOB, LINUX_JOB, build_matrix

from ..._helpers import REPO_ROOT, load_script, load_yaml

if TYPE_CHECKING:
    from collections.abc import Iterator

RUNSON_WORKFLOWS = [
    "analysis.yml",
    "android.yml",
    "custom-build.yml",
    "docker.yml",
    "linux.yml",
    "vm-builds.yml",
    "windows.yml",
]
# Until RUNS_ON_SPOT_POOLS is enabled, PR labels request the `-spot` twin's settings directly, inline
# or (linux.yml, whose legs use different pools) from the matrix entry's spot_family.
PR_SPOT_SUFFIX = re.compile(
    r"github\.event_name == 'pull_request' &&(?: matrix\.host == 'linux' &&)? "
    r"\(vars\.RUNS_ON_SPOT_POOLS == 'true' && '-spot' \|\| "
    r"(?:'/spot=cop/family=(?P<family>[\w.+-]+)'"
    r"|format\('/spot=cop/family=\{0\}', matrix\.spot_family\))\) \|\| ''"
)
SPOT_SUFFIX = "-spot"


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
        assert runner["spot"] == "cop" if pool.endswith(SPOT_SUFFIX) else runner["spot"] is False
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
    runners = load_yaml(".github/runs-on.yml")["runners"]
    for name in (pool, pool + SPOT_SUFFIX):
        if name in runners:
            assert min(runners[name]["ram"]) >= 16


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


def test_spot_pools_mirror_their_on_demand_pools() -> None:
    # Pull requests use the twin, so resizing a pool without its twin must fail here, not drift.
    runners = load_yaml(".github/runs-on.yml")["runners"]
    for pool, runner in runners.items():
        if pool.endswith(SPOT_SUFFIX):
            assert pool.removesuffix(SPOT_SUFFIX) in runners
            continue
        twin = runners.get(pool + SPOT_SUFFIX)
        if runner.get("nested-virt"):
            assert twin is None, f"{pool} must stay on-demand"
            continue
        assert twin, f"{pool} needs a {SPOT_SUFFIX} twin"
        assert {k: v for k, v in twin.items() if k not in ("family", "spot")} == {
            k: v for k, v in runner.items() if k not in ("family", "spot")
        }
        # Intel 8i and Graviton types keep the pool's order; 7th-generation types follow.
        preferred = [t for t in runner["family"] if re.fullmatch(r"[cmr]\d+[ig]\.\w+", t)]
        assert twin["family"][: len(preferred)] == preferred
        assert len(twin["family"]) > 2
        assert all(re.fullmatch(r"[cmr]\d+[ig]\.\d*xlarge", t) for t in twin["family"])
        assert not any(t.startswith("r7i.") for t in twin["family"])


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
    configured = load_yaml(".github/runs-on.yml")["runners"]
    # PR-only Spot labels are validated separately; any other shape label still fails below.
    text = PR_SPOT_SUFFIX.sub("''", _normalized(text))
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
def test_only_pull_requests_request_spot_from_intel_or_graviton_types(workflow: str) -> None:
    # Push, merge-queue, tag, and dispatch builds keep the pools' on-demand capacity. PR labels drop
    # AMD types, whose us-west-2 Spot interruption rates are well above the Intel and Graviton rates.
    text = _normalized(_read(f".github/workflows/{workflow}"))
    suffixes = list(PR_SPOT_SUFFIX.finditer(text))
    assert len(suffixes) == text.count("/spot=") == text.count("'-spot'")
    runners = load_yaml(".github/runs-on.yml")["runners"]
    families = [match["family"] for match in suffixes if match["family"]]
    for job in load_yaml(f".github/workflows/{workflow}")["jobs"].values():
        if "matrix.spot_family" not in str(job.get("runs-on", "")):
            continue
        # Each leg must request its own pool's twin, not merely some twin.
        for entry in job["strategy"]["matrix"]["include"]:
            twin = runners[entry["runson_runner"] + SPOT_SUFFIX]
            assert entry["spot_family"] == "+".join(twin["family"])
            families.append(entry["spot_family"])
    assert len(families) >= len(suffixes)
    for family in families:
        twins = [
            pool
            for pool, runner in runners.items()
            if pool.endswith(SPOT_SUFFIX) and "+".join(runner["family"]) == family
        ]
        assert twins, f"{workflow}: {family} must match a {SPOT_SUFFIX} pool"
        # Android names its pools in android_matrix.py rather than in the workflow.
        referenced = f"{text} {LINUX_JOB['runson_runner']}" if workflow == "android.yml" else text
        bases = [re.escape(twin.removesuffix(SPOT_SUFFIX)) for twin in twins]
        assert re.search(rf"(?<![\w-])(?:{'|'.join(bases)})(?![\w-])", referenced)
    if workflow == "vm-builds.yml":
        assert not families
    if workflow == "android.yml":
        assert "github.event_name == 'pull_request' && matrix.host == 'linux' && (" in text
        assert not runners[LINUX_JOB["runson_runner"]].get("nested-virt")
        assert LINUX_EMULATOR_JOB["host"] != "linux"


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
    assert "version: '1.16.0'" in workflow_text
    assert "github.event.repository.default_branch" in workflow_text


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
    assert ".github/actions/build-config" in build_steps["Checkout"]["with"]["sparse-checkout"]
    assert build_steps["Resolve build configuration"]["uses"] == "./.github/actions/build-config"
    packer_env = build_steps["Build runner image"]["env"]
    assert packer_env["PKR_VAR_qt_version"] == "${{ steps.build-config.outputs.qt_version }}"
    assert packer_env["PKR_VAR_qt_modules"] == "${{ steps.build-config.outputs.qt_modules }}"
    outputs = load_yaml(".github/actions/build-config/action.yml")["outputs"]
    assert {"qt_version", "qt_modules"} <= outputs.keys()


def test_ci_scripts_validates_the_packer_template() -> None:
    workflow = load_yaml(".github/workflows/ci-scripts.yml")
    validation_job = workflow["jobs"]["validate-runner-images"]
    validation_steps = str(validation_job["steps"])

    assert validation_job["runs-on"] == "ubuntu-latest"
    assert "hashicorp/setup-packer@v3" in validation_steps
    assert "'1.16.0'" in validation_steps
    assert "packer fmt -check" in validation_steps
    assert "packer init" in validation_steps
    assert "packer validate" in validation_steps


def test_ci_scripts_lints_runson_config_with_a_verified_cli() -> None:
    job = load_yaml(".github/workflows/ci-scripts.yml")["jobs"]["validate-runson-config"]
    install = next(step for step in job["steps"] if step.get("name") == "Install RunsOn CLI")
    lint = next(step for step in job["steps"] if step.get("name") == "Lint RunsOn config")

    # GitHub-hosted and secret-free, so fork pull requests can run it.
    assert job["runs-on"] == "ubuntu-latest"
    assert "secrets." not in str(job)
    assert re.fullmatch(r"v\d+\.\d+\.\d+", install["env"]["ROC_VERSION"])
    assert re.fullmatch(r"[0-9a-f]{64}", install["env"]["ROC_SHA256"])
    script = install["run"]
    assert script.index("sha256sum --check --strict") < script.index("chmod +x")
    assert '/roc" lint .github/runs-on.yml' in lint["run"]
    assert '/roc" lint .github/runner-images/windows-warm-pool.example.yml' in lint["run"]


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
    for workflow in ("linux.yml", "custom-build.yml", "docker.yml", "android.yml"):
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
