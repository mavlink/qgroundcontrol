#!/usr/bin/env python3
"""Behavioral contracts for the Qt installer helper."""

from __future__ import annotations

import subprocess
from typing import TYPE_CHECKING

import pytest
from setup import install_qt
from setup.install_qt import (
    _run_aqt_with_retries,
    compute_cache_digest,
    resolve_android_qt_root,
    resolve_arch_dir,
    resolve_qt_root,
    validate_aqt_source,
)

if TYPE_CHECKING:
    from pathlib import Path


def test_arch_directory_resolution() -> None:
    expected = {
        "linux_gcc_64": "gcc_64",
        "linux_arm64": "arm64",
        "win64_msvc2022_64": "msvc2022_64",
        "win64_msvc2022_arm64_cross_compiled": "msvc2022_arm64",
        "clang_64": "macos",
        "android_arm64_v8a": "android_arm64_v8a",
        "ios": "ios",
    }
    for arch, directory in expected.items():
        assert resolve_arch_dir(arch) == directory


def test_cache_digest_is_stable_and_input_sensitive() -> None:
    digest = compute_cache_digest("qtgraphs qtlocation", "")
    assert digest == compute_cache_digest("qtgraphs qtlocation", "")
    assert len(digest) == 64
    assert set(digest) <= set("0123456789abcdef")
    assert digest != compute_cache_digest("qtlocation", "")
    assert digest != compute_cache_digest("qtgraphs qtlocation", "icu")


def test_qt_root_must_exist(tmp_path: Path) -> None:
    qt_root = tmp_path / "6.8.3" / "gcc_64"
    qt_root.mkdir(parents=True)
    assert resolve_qt_root(tmp_path, "6.8.3", "gcc_64") == qt_root
    with pytest.raises(SystemExit):
        resolve_qt_root(tmp_path, "6.8.3", "arm64")


def test_android_root_uses_first_available_requested_abi() -> None:
    roots = {
        "arm64": "/qt/arm64",
        "armv7": "/qt/armv7",
        "x86_64": "/qt/x86_64",
        "x86": "/qt/x86",
    }
    cases = {
        "arm64-v8a;armeabi-v7a": "/qt/arm64",
        "armeabi-v7a;x86_64": "/qt/armv7",
        "x86_64": "/qt/x86_64",
        "x86": "/qt/x86",
    }
    for abis, expected in cases.items():
        assert resolve_android_qt_root(abis, roots) == expected

    assert resolve_android_qt_root("arm64-v8a;x86_64", roots | {"arm64": ""}) == "/qt/x86_64"
    with pytest.raises(SystemExit):
        resolve_android_qt_root("mips", roots)


def test_aqt_source_allowlist() -> None:
    accepted = [
        "",
        "aqtinstall",
        "aqtinstall==3.3.0",
        "git+https://github.com/miurahr/aqtinstall@" + "a" * 40,
        "git+https://github.com/miurahr/aqtinstall.git@" + "f" * 40,
    ]
    rejected = [
        "--extra-index-url https://evil aqtinstall",
        "git+https://attacker.example.com/evil@main",
        "git+https://github.com/miurahr/aqtinstall@main",
        "evil-package",
    ]
    for spec in accepted:
        assert validate_aqt_source(spec) == spec
    for spec in rejected:
        with pytest.raises(SystemExit):
            validate_aqt_source(spec)


def test_aqt_retries_until_success_or_limit(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setattr(install_qt.time, "sleep", lambda _seconds: None)

    def run_scenario(returncodes: list[int]) -> int:
        calls = 0
        results = iter(returncodes)

        def fake_run(args: list[str], check: bool = False) -> subprocess.CompletedProcess:
            nonlocal calls
            calls += 1
            return subprocess.CompletedProcess(args, next(results))

        monkeypatch.setattr(install_qt.subprocess, "run", fake_run)
        _run_aqt_with_retries(["aqt", "install-qt"])
        return calls

    assert run_scenario([0]) == 1
    assert run_scenario([254, 0]) == 2

    calls = 0

    def always_fail(args: list[str], check: bool = False) -> subprocess.CompletedProcess:
        nonlocal calls
        calls += 1
        return subprocess.CompletedProcess(args, 254)

    monkeypatch.setattr(install_qt.subprocess, "run", always_fail)
    with pytest.raises(subprocess.CalledProcessError):
        _run_aqt_with_retries(["aqt", "install-qt"])
    assert calls == install_qt._AQT_MAX_ATTEMPTS
