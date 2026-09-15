"""Regression tests for the shared container/CI analysis installer."""

from __future__ import annotations

import json
from pathlib import Path
from unittest.mock import Mock

import pytest
from setup import install_analysis


def test_packages_follow_config(tmp_path, capsys):
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps({"analysis": {"llvm_version": "19"}}))
    assert install_analysis.main(["--config", str(config), "--print-packages"]) == 0
    packages = capsys.readouterr().out.split()
    assert "clang-19" in packages
    assert "clang-tidy-19" in packages
    assert "clang-tools-19" in packages
    assert "clangd-19" in packages
    assert "libclang-cpp19-dev" in packages
    assert "clazy" not in packages


def test_checksum_failure_prevents_extraction_and_build(tmp_path, monkeypatch):
    monkeypatch.setattr(install_analysis, "download_with_retry", Mock())
    monkeypatch.setattr(install_analysis, "sha256_file", lambda path: "wrong")
    extract = Mock()
    run = Mock()
    monkeypatch.setattr(install_analysis, "extract_tar_data", extract)
    monkeypatch.setattr(install_analysis.subprocess, "run", run)
    with pytest.raises(RuntimeError, match="SHA256 mismatch"):
        install_analysis.build_clazy("18", "revision", "expected", tmp_path, tmp_path / "work")
    extract.assert_not_called()
    run.assert_not_called()


def test_clazy_build_matches_llvm_and_uses_available_workers(tmp_path, monkeypatch):
    download = Mock()
    extract = Mock()
    run = Mock()
    monkeypatch.setattr(install_analysis, "download_with_retry", download)
    monkeypatch.setattr(install_analysis, "sha256_file", lambda path: "expected")
    monkeypatch.setattr(install_analysis, "extract_tar_data", extract)
    monkeypatch.setattr(install_analysis.subprocess, "run", run)
    monkeypatch.setattr(
        install_analysis.os, "sched_getaffinity", lambda pid: {0, 1, 2, 3}, raising=False
    )
    prefix, work = tmp_path / "install", tmp_path / "work"
    install_analysis.build_clazy("18", "revision", "expected", prefix, work)
    download.assert_called_once_with(
        "https://github.com/KDE/clazy/archive/revision.tar.gz", work / "clazy.tar.gz"
    )
    extract.assert_called_once_with(work / "clazy.tar.gz", work / "source", mode="r:gz")
    configure = run.call_args_list[0].args[0]
    assert f"-DCMAKE_INSTALL_PREFIX={prefix}" in configure
    assert "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-18" in configure
    assert "-DLLVM_CONFIG_EXECUTABLE=/usr/lib/llvm-18/bin/llvm-config" in configure
    assert "-DCLAZY_LINK_CLANG_DYLIB=ON" in configure
    assert "-DCLAZY_MAN_PAGE=OFF" in configure
    assert run.call_args_list[1].args[0][-2:] == ["--parallel", "4"]
    assert run.call_args_list[2].args[0] == ["cmake", "--install", str(work / "build")]
    assert all(call.kwargs["check"] for call in run.call_args_list)


@pytest.mark.parametrize(
    ("version", "libraries", "error"),
    [
        ("clang version 17.0.0\n", "", "configured LLVM 18"),
        ("clang version 18.1.8\n", "libclang-cpp.so.17 => /lib/clang.so\n", "libclang-cpp"),
        ("clang version 18.1.8\n", "libclang-cpp.so.18.1 => not found\n", "libclang-cpp"),
    ],
)
def test_verify_rejects_version_or_abi_drift(monkeypatch, version, libraries, error):
    monkeypatch.setattr(
        install_analysis.subprocess,
        "check_output",
        lambda args, **kwargs: libraries if args[0] == "ldd" else version,
    )
    with pytest.raises(RuntimeError, match=error):
        install_analysis.verify_toolchain("18", Path("/opt/clazy"))


def test_verify_runs_compiler_smoke(monkeypatch):
    monkeypatch.setattr(
        install_analysis.subprocess,
        "check_output",
        lambda args, **kwargs: (
            "libclang-cpp.so.18.1 => /usr/lib/llvm-18/lib/libclang-cpp.so.18.1\n"
            if args[0] == "ldd"
            else "Ubuntu LLVM version 18.1.8\n"
        ),
    )
    run = Mock()
    monkeypatch.setattr(install_analysis.subprocess, "run", run)
    install_analysis.verify_toolchain("18", Path("/opt/clazy"))
    assert run.call_args.args[0] == ["clang++", "-x", "c++", "-fsyntax-only", "-"]
    assert run.call_args.kwargs["check"]


def test_devcontainer_inherits_published_ubuntu_analysis_stage():
    root = Path(__file__).resolve().parents[2]
    dockerfile = (root / "deploy/docker/Dockerfile").read_text()
    variants = json.loads((root / "deploy/docker/variants.json").read_text())["variants"]
    assert next(v for v in variants if v["id"] == "ubuntu")["target"] == "linux-analysis"
    assert all(v["target"] != "linux-analysis" for v in variants if v["id"] != "ubuntu")
    assert "FROM linux-analysis AS devcontainer" in dockerfile
    assert (
        'ENV PATH="/opt/qgc-venv/bin:/opt/clazy/bin:/opt/llvm/bin:/opt/qt/bin:${PATH}"'
        in dockerfile
    )
    nonroot = dockerfile.split("USER ${DEV_USER}", 1)[1].split("ENTRYPOINT", 1)[0]
    assert (
        "--config /opt/qgc-bootstrap/tools/setup/build-config.json --prefix /opt/clazy --verify"
        in nonroot
    )
    assert 'SHELL ["/bin/bash", "-o", "pipefail", "-c"]' in nonroot
