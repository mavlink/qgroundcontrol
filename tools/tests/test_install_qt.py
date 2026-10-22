"""Tests for tools/setup/install_qt.py."""

from __future__ import annotations

import hashlib
import json
import subprocess
from typing import TYPE_CHECKING

import pytest
from setup import install_qt
from setup.install_qt import (
    _run_aqt_with_retries,
    compute_cache_digest,
    plan_install,
    resolve_arch_dir,
    resolve_preinstalled_qt,
    resolve_qt_root,
    resolve_windows_host_arch,
)

if TYPE_CHECKING:
    from pathlib import Path

pytestmark = pytest.mark.windows


def test_configured_container_install_reuses_qt_policy(monkeypatch, tmp_path):
    config = tmp_path / "build-config.json"
    config.write_text(json.dumps({"qt": {"version": "6.8.3", "modules": "qtcharts qtlocation"}}))
    monkeypatch.setenv("CONFIG_FILE", str(config))
    monkeypatch.setattr(install_qt, "tool_command", lambda *args, **kwargs: ["locked-aqt"])
    commands = []
    monkeypatch.setattr(install_qt, "_run_aqt_with_retries", commands.append)
    root = tmp_path / "Qt/6.8.3/android_arm64_v8a"
    root.mkdir(parents=True)
    assert (
        install_qt.main(
            [
                "install",
                "--from-config",
                "--host",
                "all_os",
                "--target",
                "android",
                "--arch",
                "android_arm64_v8a",
                "--outdir",
                str(tmp_path / "Qt"),
                "--autodesktop",
            ]
        )
        == 0
    )
    assert commands == [
        [
            "locked-aqt",
            "install-qt",
            "all_os",
            "android",
            "6.8.3",
            "android_arm64_v8a",
            "--outputdir",
            str(tmp_path / "Qt"),
            "--modules",
            "qtcharts",
            "qtlocation",
            "--autodesktop",
        ]
    ]


class TestResolveArchDir:
    def test_linux_gcc_64(self) -> None:
        assert resolve_arch_dir("linux_gcc_64") == "gcc_64"

    def test_linux_arm64(self) -> None:
        assert resolve_arch_dir("linux_arm64") == "arm64"

    def test_win64_msvc2022_64(self) -> None:
        assert resolve_arch_dir("win64_msvc2022_64") == "msvc2022_64"

    def test_win64_msvc2022_arm64_cross_compiled(self) -> None:
        assert resolve_arch_dir("win64_msvc2022_arm64_cross_compiled") == "msvc2022_arm64"

    def test_future_windows_cross_compile_arch(self) -> None:
        arch = "win64_msvc2025_arm64_cross_compiled"
        assert resolve_arch_dir(arch) == "msvc2025_arm64"
        assert resolve_windows_host_arch(arch) == "win64_msvc2025_64"

    def test_clang_64_maps_to_macos(self) -> None:
        assert resolve_arch_dir("clang_64") == "macos"

    def test_android_arm64_v8a_unchanged(self) -> None:
        assert resolve_arch_dir("android_arm64_v8a") == "android_arm64_v8a"

    @pytest.mark.parametrize(
        "arch", ["ios", "ios_device", "ios_simulator_arm64", "ios_simulator_x86_64"]
    )
    def test_ios_unchanged(self, arch: str) -> None:
        assert resolve_arch_dir(arch) == arch


class TestComputeCacheDigest:
    def test_digest_formula_keeps_existing_cache_keys(self) -> None:
        expected = hashlib.sha256(b"qtgraphs qtlocation\nqtbase icu").hexdigest()
        assert compute_cache_digest("qtgraphs qtlocation", "qtbase icu") == expected

    def test_deterministic(self) -> None:
        a = compute_cache_digest("qtgraphs qtlocation", "")
        b = compute_cache_digest("qtgraphs qtlocation", "")
        assert a == b

    def test_different_modules_differ(self) -> None:
        a = compute_cache_digest("qtgraphs", "")
        b = compute_cache_digest("qtlocation", "")
        assert a != b

    def test_archives_affect_digest(self) -> None:
        a = compute_cache_digest("qtgraphs", "")
        b = compute_cache_digest("qtgraphs", "icu")
        assert a != b

    def test_returns_hex_string(self) -> None:
        d = compute_cache_digest("", "")
        assert len(d) == 64
        assert all(c in "0123456789abcdef" for c in d)


class TestResolveQtRoot:
    def test_valid_path(self, tmp_path: Path) -> None:
        qt_root = tmp_path / "6.8.3" / "gcc_64"
        qt_root.mkdir(parents=True)
        result = resolve_qt_root(tmp_path, "6.8.3", "gcc_64")
        assert result == qt_root

    def test_missing_path_exits(self, tmp_path: Path) -> None:
        with pytest.raises(SystemExit):
            resolve_qt_root(tmp_path, "6.8.3", "gcc_64")


class TestResolvePreinstalledQt:
    @staticmethod
    def _create_sdk(tmp_path: Path, modules: str = "qtgraphs qtlocation") -> Path:
        qt_root = tmp_path / "Qt" / "6.8.3" / "gcc_64"
        (qt_root / "bin").mkdir(parents=True)
        (qt_root / ".qgc-modules").write_text(modules, encoding="utf-8")
        return qt_root

    def test_compatible_sdk_is_reused(self, tmp_path: Path) -> None:
        qt_root = self._create_sdk(tmp_path)

        assert resolve_preinstalled_qt(tmp_path, "6.8.3", "gcc_64", "qtlocation") == qt_root

    def test_missing_requested_module_is_rejected(self, tmp_path: Path) -> None:
        self._create_sdk(tmp_path, "qtgraphs")

        assert resolve_preinstalled_qt(tmp_path, "6.8.3", "gcc_64", "qtlocation") is None

    def test_archive_subset_is_rejected(self, tmp_path: Path) -> None:
        self._create_sdk(tmp_path)

        assert resolve_preinstalled_qt(tmp_path, "6.8.3", "gcc_64", archives="qtbase") is None

    def test_unmanaged_sdk_is_rejected(self, tmp_path: Path) -> None:
        qt_root = tmp_path / "Qt" / "6.8.3" / "gcc_64"
        (qt_root / "bin").mkdir(parents=True)

        assert resolve_preinstalled_qt(tmp_path, "6.8.3", "gcc_64") is None


def _github_files(monkeypatch: pytest.MonkeyPatch, tmp_path: Path) -> dict[str, Path]:
    files = {
        name: tmp_path / f"{name.lower()}.txt"
        for name in ("GITHUB_OUTPUT", "GITHUB_ENV", "GITHUB_PATH")
    }
    for name, path in files.items():
        monkeypatch.setenv(name, str(path))
        path.write_text("")
    return files


class TestPlan:
    def test_outputs_unchanged_cache_inputs(self, monkeypatch, tmp_path) -> None:
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
        files = _github_files(monkeypatch, tmp_path)
        args = ["--version", "6.8.3", "--arch", "win64_msvc2022_64", "--modules", "qtgraphs"]
        outdir = tmp_path / ".qt" / "Qt"
        assert install_qt.main(["plan", "--outdir", str(outdir), *args]) == 0

        # Matches the former `${cache_dir}/Qt/${version}/${arch_dir}` path and digest exactly.
        assert files["GITHUB_OUTPUT"].read_text().splitlines() == [
            f"digest={compute_cache_digest('qtgraphs', '')}",
            "cache_path=.qt/Qt/6.8.3/msvc2022_64",
            "preinstalled_root=",
        ]

    def test_reports_compatible_preinstalled_sdk(self, tmp_path) -> None:
        prefix = tmp_path / "sdk"
        qt_root = TestResolvePreinstalledQt._create_sdk(prefix)

        plan = plan_install(
            tmp_path / ".qt/Qt", "6.8.3", "linux_gcc_64", "qtlocation", "", str(prefix)
        )

        assert plan["preinstalled_root"] == str(qt_root)
        assert plan_install(tmp_path / ".qt/Qt", "6.8.3", "linux_gcc_64")["preinstalled_root"] == ""


class TestEnsure:
    @staticmethod
    def _ensure(tmp_path: Path, *extra: str) -> int:
        return install_qt.main(
            [
                "ensure",
                "--outdir",
                str(tmp_path / "Qt"),
                "--version",
                "6.8.3",
                "--host",
                "linux",
                "--arch",
                "linux_gcc_64",
                *extra,
            ]
        )

    @pytest.fixture
    def aqt_calls(self, monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
        calls: list[list[str]] = []
        monkeypatch.setattr(install_qt, "tool_command", lambda *args, **kwargs: ["aqt"])
        monkeypatch.setattr(install_qt, "_run_aqt_with_retries", calls.append)
        return calls

    def test_preinstalled_sdk_is_exported_without_install(
        self, monkeypatch, tmp_path, aqt_calls
    ) -> None:
        files = _github_files(monkeypatch, tmp_path)
        qt_root = tmp_path / "sdk/Qt/6.8.3/gcc_64"

        assert self._ensure(tmp_path, "--preinstalled-root", str(qt_root), "--cache-hit", "") == 0

        assert aqt_calls == []
        assert files["GITHUB_OUTPUT"].read_text() == f"qt_root_dir={qt_root}\n"
        assert files["GITHUB_ENV"].read_text() == f"QT_ROOT_DIR={qt_root}\n"
        assert files["GITHUB_PATH"].read_text() == f"{qt_root / 'bin'}\n"

    def test_cache_hit_reuses_restored_tree(self, monkeypatch, tmp_path, aqt_calls) -> None:
        files = _github_files(monkeypatch, tmp_path)
        qt_root = tmp_path / "Qt/6.8.3/gcc_64"
        qt_root.mkdir(parents=True)

        assert self._ensure(tmp_path, "--cache-hit", "true") == 0

        assert aqt_calls == []
        assert files["GITHUB_OUTPUT"].read_text() == f"qt_root_dir={qt_root}\n"

    def test_cache_hit_without_tree_fails(self, monkeypatch, tmp_path, aqt_calls) -> None:
        _github_files(monkeypatch, tmp_path)

        with pytest.raises(SystemExit):
            self._ensure(tmp_path, "--cache-hit", "true")
        assert aqt_calls == []

    def test_cache_miss_installs_without_exporting(self, monkeypatch, tmp_path, aqt_calls) -> None:
        files = _github_files(monkeypatch, tmp_path)
        qt_root = tmp_path / "Qt/6.8.3/gcc_64"
        qt_root.mkdir(parents=True)

        assert self._ensure(tmp_path, "--cache-hit", "false", "--export-env", "false") == 0

        assert aqt_calls == [
            [
                "aqt",
                "install-qt",
                "linux",
                "desktop",
                "6.8.3",
                "linux_gcc_64",
                "--outputdir",
                str(tmp_path / "Qt"),
            ]
        ]
        assert files["GITHUB_OUTPUT"].read_text() == f"qt_root_dir={qt_root}\n"
        assert files["GITHUB_ENV"].read_text() == ""
        assert files["GITHUB_PATH"].read_text() == ""


class TestRunAqtWithRetries:
    @staticmethod
    def _fake_run(returncodes: list[int], calls: list[list[str]]):
        seq = iter(returncodes)

        def _run(args: list[str], check: bool = False, **_kwargs) -> subprocess.CompletedProcess:
            calls.append(args)
            code = next(seq)
            if check and code:
                raise subprocess.CalledProcessError(code, args)
            return subprocess.CompletedProcess(args, code)

        return _run

    def test_succeeds_first_try(self, monkeypatch: pytest.MonkeyPatch) -> None:
        calls: list[list[str]] = []
        monkeypatch.setattr("common.proc.subprocess.run", self._fake_run([0], calls))
        _run_aqt_with_retries(["aqt", "install-qt"])
        assert len(calls) == 1

    def test_retries_then_succeeds(self, monkeypatch: pytest.MonkeyPatch) -> None:
        calls: list[list[str]] = []
        monkeypatch.setattr("common.proc.subprocess.run", self._fake_run([254, 0], calls))
        monkeypatch.setattr("common.proc.time.sleep", lambda _s: None)
        _run_aqt_with_retries(["aqt", "install-qt"])
        assert len(calls) == 2

    def test_raises_after_exhausting_attempts(self, monkeypatch: pytest.MonkeyPatch) -> None:
        calls: list[list[str]] = []
        monkeypatch.setattr("common.proc.subprocess.run", self._fake_run([254] * 3, calls))
        monkeypatch.setattr("common.proc.time.sleep", lambda _s: None)
        with pytest.raises(subprocess.CalledProcessError):
            _run_aqt_with_retries(["aqt", "install-qt"])
        assert len(calls) == install_qt._AQT_MAX_ATTEMPTS
