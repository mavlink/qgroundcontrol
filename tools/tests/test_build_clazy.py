"""Tests for setup/build_clazy.py."""

from __future__ import annotations

import io
import json
import tarfile
from typing import TYPE_CHECKING

import pytest
from setup import build_clazy

if TYPE_CHECKING:
    from pathlib import Path

CONFIG = json.loads(
    (build_clazy.DEFAULT_PREFIX.parents[1] / ".github/build-config.json").read_text()
)["analysis"]
PIN = build_clazy.ClazyPin(llvm_version="18", revision="abc123", sha256="0" * 64)


@pytest.fixture
def toolchain(tmp_path: Path, monkeypatch: pytest.MonkeyPatch) -> dict[str, list]:
    """Fake LLVM install, archive download and subprocess calls."""
    llvm = tmp_path / "llvm"
    (llvm / "bin").mkdir(parents=True)
    (llvm / "bin/llvm-config").write_text("")
    monkeypatch.setattr(build_clazy, "llvm_root", lambda version: llvm)
    calls: dict[str, list] = {"downloads": [], "commands": []}

    def download(url: str, dest: Path, *, expected_sha256: str) -> None:
        calls["downloads"].append((url, expected_sha256))
        with tarfile.open(dest, "w:gz") as archive:
            info = tarfile.TarInfo(f"clazy-{PIN.revision}/CMakeLists.txt")
            archive.addfile(info, io.BytesIO(b""))

    monkeypatch.setattr(build_clazy, "download_file", download)
    monkeypatch.setattr(
        build_clazy.subprocess, "run", lambda command, check: calls["commands"].append(command)
    )
    return calls


def test_pin_comes_from_the_analysis_build_config() -> None:
    pin = build_clazy.read_pin()
    assert pin == build_clazy.ClazyPin(
        llvm_version=str(CONFIG["llvm_version"]),
        revision=CONFIG["clazy_revision"],
        sha256=CONFIG["clazy_sha256"],
    )


def test_builds_the_verified_revision_with_the_analysis_llvm(tmp_path: Path, toolchain) -> None:
    prefix = tmp_path / "prefix"
    work = tmp_path / "work"
    assert build_clazy.build_clazy(PIN, prefix, work, jobs=3) == prefix / "bin/clazy-standalone"
    assert toolchain["downloads"] == [
        ("https://github.com/KDE/clazy/archive/abc123.tar.gz", PIN.sha256)
    ]
    configure, build, install = toolchain["commands"]
    source = work / "src" / f"clazy-{PIN.revision}"
    assert configure[:7] == ["cmake", "-S", str(source), "-B", str(work / "build"), "-G", "Ninja"]
    assert {
        "-DCMAKE_C_COMPILER=/usr/bin/clang-18",
        "-DCMAKE_CXX_COMPILER=/usr/bin/clang++-18",
        "-DCLAZY_LINK_CLANG_DYLIB=ON",
        f"-DCMAKE_INSTALL_PREFIX={prefix}",
    } <= set(configure)
    assert build == [
        *("cmake", "--build", str(work / "build")),
        *("--target", "clazy-standalone", "--parallel", "3"),
    ]
    assert install == ["cmake", "--install", str(work / "build")]


def test_missing_llvm_development_packages_are_named(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr(build_clazy, "llvm_root", lambda version: tmp_path / "missing")
    monkeypatch.setattr(build_clazy, "read_pin", lambda: PIN)
    assert build_clazy.main(["--prefix", str(tmp_path / "prefix")]) == 1
    assert "libclang-cpp18-dev" in capsys.readouterr().err


def test_main_installs_into_the_requested_prefix(tmp_path: Path, toolchain, monkeypatch) -> None:
    monkeypatch.setattr(build_clazy, "read_pin", lambda: PIN)
    monkeypatch.chdir(tmp_path)
    assert build_clazy.main(["--prefix", ".cache/clazy", "--work-dir", "work", "--jobs", "2"]) == 0
    configure = toolchain["commands"][0]
    assert f"-DCMAKE_INSTALL_PREFIX={tmp_path / '.cache/clazy'}" in configure
    assert str(tmp_path / "work/build") in configure
