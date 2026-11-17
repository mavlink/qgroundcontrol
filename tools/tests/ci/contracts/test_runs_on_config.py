"""Contracts for the RunsOn pools, the workflow labels that select them, and the managed image."""

from __future__ import annotations

import os
import re
from pathlib import Path
from typing import TYPE_CHECKING, Any

import pytest
from ci.planning.android_matrix import build_matrix
from ci.runner import validate_runner_configs

from ..._helpers import REPO_ROOT, load_script, load_yaml, workflow_names
from .._workflows import step_groups

if TYPE_CHECKING:
    from collections.abc import Iterator

# One job per run, on the image under test: its label needs no job word, attempt or matrix index.
SINGLE_JOB_ROUTES = {"runner-images.yml:smoke-test"}


def _read(path: str) -> str:
    return (REPO_ROOT / path).read_text(encoding="utf-8")


def _normalized(text: str) -> str:
    return " ".join(text.split())


def _pools() -> dict[str, dict[str, Any]]:
    return load_yaml(".github/runs-on.yml")["runners"]


def _instance_shape(instance_type: str) -> tuple[int, int]:
    match = re.fullmatch(r"([cmr])\d+[aig]\.(\d*)xlarge", instance_type)
    assert match, f"{instance_type} must be an exact c/m/r xlarge-class instance type"
    vcpus = 4 * int(match[2] or 1)
    return vcpus, vcpus * {"c": 2, "m": 4, "r": 8}[match[1]]


def _routes() -> Iterator[tuple[str, str]]:
    """Yield (workflow:job, runs-on expression) for every job that can run on RunsOn."""
    for label, job, _ in step_groups(actions=False):
        # Matrix values cannot read strategy.job-index, so labels belong in the job's runs-on.
        assert "runs-on=" not in str(job.get("strategy", {})), label
        if "runs-on=" in str(job.get("runs-on", "")):
            yield label, _normalized(job["runs-on"])


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


def test_runner_pool_families_are_prioritized_exact_types_matching_their_shape() -> None:
    # Exact types make on-demand launches follow list order, and RunsOn then ignores cpu/ram.
    for pool, runner in _pools().items():
        families = runner["family"]
        assert len(families) >= 2, f"{pool} needs an on-demand fallback type"
        assert len(set(families)) == len(families)
        for instance_type in families:
            vcpus, memory = _instance_shape(instance_type)
            assert vcpus in runner["cpu"], f"{pool}: {instance_type}"
            assert min(runner["ram"]) <= memory <= max(runner["ram"]), f"{pool}: {instance_type}"
            is_graviton = instance_type.split(".")[0].endswith("g")
            assert is_graviton == pool.startswith("linux-arm64-"), f"{pool}: {instance_type}"


def test_nested_virtualization_pools_use_intel_families() -> None:
    # EC2 exposes nested virtualization only on Intel 7i/8i families, not AMD or Graviton.
    nested = {pool: runner for pool, runner in _pools().items() if runner.get("nested-virt")}
    assert nested
    for pool, runner in nested.items():
        assert all(re.fullmatch(r"[cmr][78]i\.\w+", family) for family in runner["family"]), pool


def test_workflows_select_configured_pools_and_leave_their_shape_to_runs_on_yml() -> None:
    configured = _pools()
    selecting = 0
    for workflow in workflow_names():
        text = _normalized(_read(f".github/workflows/{workflow}"))
        if "runner=" not in text:
            continue
        selecting += 1
        # RunsOn reads runs-on.yml from the default branch only, so the Windows builder repeats its
        # volume as a label to apply in PRs; any other value or shape label still fails below.
        text = text.replace(f"/volume={configured['windows-x64-builder']['volume']}", "")
        assert not re.search(r"/(?:family|cpu|ram|spot|image|volume|extras|nested-virt)=", text), (
            workflow
        )
        # Pool names in labels and in the matrix values that labels format in.
        for pool in re.findall(r"(?:linux|windows)-(?:x64|arm64)-[\w-]+", text):
            assert pool in configured, f"{workflow}: {pool}"
    assert selecting


def test_runson_labels_are_unique_per_job_attempt_and_matrix_entry() -> None:
    # GitHub may hand a runner to any queued job with the same label, so matrix siblings sharing one
    # stall while RunsOn replaces stolen runners. github.job is null outside steps, so labels spell
    # out a word for the job, and strategy.job-index separates matrix entries.
    words: dict[str, list[str]] = {}
    for label, route in _routes():
        if label in SINGLE_JOB_ROUTES:
            continue
        assert "github.job" not in route, label
        word = re.search(r"format\('runs-on=\{0\}-([\w-]+)-\{1\}-\{2\}/", route)
        assert word, label
        assert "github.run_id, github.run_attempt, strategy.job-index" in route, label
        words.setdefault(label.split(":")[0], []).append(word[1])
    assert words
    for workflow, used in words.items():
        assert len(set(used)) == len(used), workflow


def test_runson_routes_never_request_spot_or_depend_on_the_event_or_actor() -> None:
    # Every event, pull requests included, runs on the pools' on-demand capacity.
    for workflow in workflow_names():
        text = _normalized(_read(f".github/workflows/{workflow}"))
        for token in ("-spot", "/spot=", "RUNS_ON_SPOT_POOLS"):
            assert token not in text, f"{workflow}: {token}"
    for label, route in _routes():
        assert "github.event" not in route, label
        assert "github.actor" not in route, label


def test_android_matrix_references_defined_runners() -> None:
    configured = _pools()
    for leg in build_matrix(is_pr=False):
        if pool := leg["runson_runner"]:
            assert pool in configured


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
    hook = _precommit_hook(hook_id)
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


def test_windows_warm_pool_example_copies_the_configured_runner() -> None:
    warm_pool = load_yaml(".github/runner-images/windows-warm-pool.example.yml")
    assert warm_pool["pools"]
    for pool in warm_pool["pools"].values():
        assert warm_pool["runners"][pool["runner"]] == _pools()[pool["runner"]]
