"""Regression tests for the container analysis installer."""

from __future__ import annotations

import importlib.util
import json
from pathlib import Path
from unittest.mock import Mock

import pytest

ROOT = Path(__file__).resolve().parents[2]
SPEC = importlib.util.spec_from_file_location(
    "install_analysis", ROOT / "deploy/docker/install_analysis.py"
)
assert SPEC is not None and SPEC.loader is not None
install_analysis = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(install_analysis)


def test_image_build_installs_packages_and_clazy_from_config(tmp_path, monkeypatch):
    config = tmp_path / "tools/setup/build-config.json"
    config.parent.mkdir(parents=True)
    config.write_text(
        json.dumps(
            {
                "analysis": {
                    "llvm_version": "19",
                    "clazy_revision": "revision",
                    "clazy_sha256": "checksum",
                }
            }
        )
    )
    run, build = Mock(), Mock()
    monkeypatch.setattr(install_analysis, "IMAGE_ROOT", tmp_path)
    monkeypatch.setattr(install_analysis.subprocess, "run", run)
    monkeypatch.setattr(install_analysis, "build_clazy", build)
    assert install_analysis.main([]) == 0
    assert run.call_args_list[0].args[0] == ["apt-get", "update"]
    packages = run.call_args_list[1].args[0]
    assert "clang-19" in packages
    assert "clang-tidy-19" in packages
    assert "clang-tools-19" in packages
    assert "clangd-19" in packages
    assert "libclang-cpp19-dev" in packages
    assert "clazy" not in packages
    build.assert_called_once_with(
        "19", "revision", "checksum", Path("/opt/clazy"), tmp_path / "clazy-build"
    )


def test_verify_never_installs_or_downloads(monkeypatch):
    monkeypatch.setattr(
        install_analysis, "load_build_config", lambda path: {"analysis": {"llvm_version": "18"}}
    )
    verify, run, download = Mock(), Mock(), Mock()
    monkeypatch.setattr(install_analysis, "verify_toolchain", verify)
    monkeypatch.setattr(install_analysis.subprocess, "run", run)
    monkeypatch.setattr(install_analysis, "download_with_retry", download)
    assert install_analysis.main(["--verify"]) == 0
    verify.assert_called_once_with("18", Path("/opt/clazy"))
    run.assert_not_called()
    download.assert_not_called()


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


def test_devcontainer_adds_analysis_without_retargeting_application_builders():
    root = Path(__file__).resolve().parents[2]
    dockerfile = (root / "deploy/docker/Dockerfile").read_text()
    variants = json.loads((root / "deploy/docker/variants.json").read_text())["variants"]
    assert next(v for v in variants if v["id"] == "ubuntu")["target"] == "linux"
    assert all(v["target"] != "qgc-dev" for v in variants)
    assert "FROM linux AS qgc-dev" in dockerfile
    assert " AS linux-analysis" not in dockerfile
    assert " AS devcontainer" not in dockerfile
    assert "arm64) QT_HOST=linux_arm64; QT_ARCH=linux_gcc_arm64" in dockerfile
    assert "resolve-arch --arch" in dockerfile
    assert "--group dev --no-default-groups" in dockerfile
    assert "UV_NO_SYNC=true" in dockerfile
    assert (
        'ENV PATH="/opt/qgc-venv/bin:/opt/clazy/bin:/opt/llvm/bin:/opt/qt/bin:${PATH}"'
        in dockerfile
    )
    nonroot = dockerfile.split("USER ${DEV_USER}", 1)[1].split("ENTRYPOINT", 1)[0]
    assert "/opt/qgc-bootstrap/deploy/docker/install_analysis.py --verify" in nonroot
    assert 'SHELL ["/bin/bash", "-o", "pipefail", "-c"]' in nonroot
