"""Tests for setup/install_analysis_tools.py."""

from __future__ import annotations

import re
from pathlib import Path

import pytest
from common.paths import REPO_ROOT
from setup import install_analysis_tools as tools

DOCKERFILE = REPO_ROOT / "deploy/docker/Dockerfile"


@pytest.fixture
def installed(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> dict[str, list]:
    """Record the installer's commands, wheel install and Clazy build without running them."""
    calls: dict[str, list] = {"run": [], "wheel": [], "clazy": []}
    monkeypatch.setattr(tools, "_run", lambda command, **kwargs: calls["run"].append(command))
    monkeypatch.setattr(
        tools,
        "install_clang_tidy",
        lambda version, sha256, work_dir: calls["wheel"].append((version, sha256, work_dir)),
    )
    monkeypatch.setattr(
        tools.build_clazy,
        "build_clazy",
        lambda pin, prefix, work_dir, jobs: calls["clazy"].append((pin, prefix, jobs)),
    )
    return calls


def test_installs_the_machine_s_pinned_wheel_and_clazy_with_llvm_on_path(
    tmp_path: Path, installed: dict[str, list], monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr(tools.platform, "machine", lambda: "aarch64")
    analysis = tools.load_build_config()["analysis"]
    llvm = analysis["llvm_version"]

    tools.install(tmp_path, skip_apt=False, jobs=3)

    assert installed["run"][1][-len(tools.llvm_packages(llvm)) :] == tools.llvm_packages(llvm)
    assert installed["wheel"] == [
        (analysis["clang_tidy_version"], analysis["clang_tidy_sha256"]["aarch64"], tmp_path)
    ]
    assert installed["clazy"] == [(tools.build_clazy.read_pin(), tmp_path / "clazy", 3)]
    assert (tmp_path / "llvm").readlink() == Path(f"/usr/lib/llvm-{llvm}")
    path = ":".join(str(directory) for directory in tools.bin_dirs(tmp_path))
    assert (tmp_path / "env.sh").read_text() == f'export PATH="{path}:$PATH"\n'
    assert [Path(command[0]).name for command in installed["run"][2:]] == [
        "clang-tidy",
        "clazy-standalone",
        "clang-scan-deps",
    ]


def test_unpinned_machine_fails_before_installing(
    tmp_path: Path, installed: dict[str, list], monkeypatch: pytest.MonkeyPatch, capsys
) -> None:
    monkeypatch.setattr(tools.platform, "machine", lambda: "riscv64")
    assert tools.main(["--prefix", str(tmp_path)]) == 1
    assert "No clang-tidy wheel pinned for riscv64" in capsys.readouterr().err
    assert installed == {"run": [], "wheel": [], "clazy": []}


def test_devcontainer_path_matches_the_installed_layout() -> None:
    """Non-login shells get the env.sh PATH from the image's ENV."""
    dockerfile = DOCKERFILE.read_text(encoding="utf-8")
    assert "install_analysis_tools.py --prefix /opt/qgc-analysis" in dockerfile
    (env_path,) = re.findall(r'^ENV PATH="(/opt/qgc-analysis/[^"]*):\$\{PATH\}"$', dockerfile, re.M)
    assert env_path.split(":") == [str(d) for d in tools.bin_dirs(Path("/opt/qgc-analysis"))]


@pytest.mark.parametrize(("cpus", "gib", "jobs"), [(24, 8, 4), (4, 16, 4), (8, 1, 1)])
def test_default_jobs_leave_each_compile_two_gib(monkeypatch, cpus: int, gib: int, jobs: int):
    monkeypatch.setattr(tools.os, "cpu_count", lambda: cpus)
    pages = {"SC_PHYS_PAGES": gib << 18, "SC_PAGE_SIZE": 4096}
    monkeypatch.setattr(tools.os, "sysconf", pages.__getitem__)
    assert tools.default_jobs() == jobs
