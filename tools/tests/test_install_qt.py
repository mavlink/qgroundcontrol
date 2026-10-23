"""Tests for tools/setup/install_qt.py."""

from __future__ import annotations

import hashlib
import json
import shutil
import subprocess
import sys
from dataclasses import replace
from pathlib import Path

import pytest
from common.build_config import derive_ios_qt_modules, load_build_config
from setup import install_qt
from setup.install_qt import (
    QtPlan,
    _run_aqt_with_retries,
    compute_cache_digest,
    plan_install,
    resolve_arch_dir,
    resolve_installer,
    resolve_preinstalled_qt,
    resolve_qt_root,
)
from setup.qt_installer.repo import QtRequest, resolve_windows_host_arch

from ._helpers import TOOLS_DIR, read_github_file, write_files

pytestmark = pytest.mark.windows


@pytest.fixture(autouse=True)
def _no_installer_override(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("QGC_QT_INSTALLER", raising=False)


@pytest.fixture
def qgc_installs(monkeypatch: pytest.MonkeyPatch) -> list[tuple[str, QtRequest]]:
    """Record QGC installer calls instead of downloading; each creates the arch directory."""
    calls: list[tuple[str, QtRequest]] = []

    def fake_install(request: QtRequest, outdir: Path) -> Path:
        calls.append(("install", request))
        root = outdir / request.version / resolve_arch_dir(request.arch)
        root.mkdir(parents=True, exist_ok=True)
        return root

    monkeypatch.setattr(install_qt, "install", fake_install)
    monkeypatch.setattr(
        install_qt,
        "ensure_host_desktop",
        lambda request, _outdir: calls.append(("desktop", request)),
    )
    return calls


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
                "--installer",
                "aqt",
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


def test_qgc_installer_installs_the_desktop_host_qt_first(tmp_path, qgc_installs) -> None:
    argv = ["install", "--version", "6.12.0", "--host", "all_os", "--target", "android"]
    argv += ["--arch", "android_armv7", "--modules", "qtlocation", "--archives", ""]
    assert (
        install_qt.main([*argv, "--outdir", str(tmp_path), "--autodesktop", "--installer", "qgc"])
        == 0
    )

    request = QtRequest("all_os", "android", "6.12.0", "android_armv7", ("qtlocation",))
    assert qgc_installs == [("desktop", request), ("install", request)]


def test_qgc_installer_gets_an_absolute_output_directory(
    monkeypatch, tmp_path, qgc_installs
) -> None:
    monkeypatch.chdir(tmp_path)
    root = install_qt.install_qt(
        "linux", "desktop", "6.12.0", "linux_gcc_64", Path("Qt"), archives="qtbase icu"
    )
    assert root == tmp_path / "Qt/6.12.0/gcc_64"
    assert qgc_installs == [
        ("install", QtRequest("linux", "desktop", "6.12.0", "linux_gcc_64", (), ("qtbase", "icu")))
    ]


class TestInstallerSelection:
    def test_defaults_to_qgc(self, monkeypatch) -> None:
        monkeypatch.delenv("QGC_QT_INSTALLER", raising=False)
        assert resolve_installer("") == "qgc"

    def test_option_overrides_environment_overrides_default(self, monkeypatch) -> None:
        assert resolve_installer("") == "qgc"
        monkeypatch.setenv("QGC_QT_INSTALLER", "aqt")
        assert resolve_installer("") == "aqt"
        assert resolve_installer("qgc") == "qgc"

    def test_unknown_environment_value_fails(self, monkeypatch, tmp_path, capsys) -> None:
        monkeypatch.setenv("QGC_QT_INSTALLER", "7z")
        argv = ["plan", "--outdir", str(tmp_path), "--version", "6.12.0", "--arch", "linux_gcc_64"]
        assert install_qt.main(argv) == 1
        assert "Unknown Qt installer '7z'" in capsys.readouterr().out


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

    def test_installer_separates_caches(self) -> None:
        assert compute_cache_digest("qtgraphs", "", "aqt") == compute_cache_digest("qtgraphs", "")
        expected = hashlib.sha256(b"qtgraphs\n\ninstaller=qgc").hexdigest()
        assert compute_cache_digest("qtgraphs", "", "qgc") == expected


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


class TestPlan:
    def test_outputs_unchanged_cache_inputs(self, monkeypatch, tmp_path, gh_output) -> None:
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
        args = ["--version", "6.8.3", "--host", "windows", "--arch", "win64_msvc2022_64"]
        outdir = tmp_path / ".qt" / "Qt"
        assert (
            install_qt.main(
                [
                    "plan",
                    "--outdir",
                    str(outdir),
                    *args,
                    "--modules",
                    "qtgraphs",
                    "--installer",
                    "aqt",
                ]
            )
            == 0
        )

        # aqt keeps the former `${cache_dir}/Qt/${version}/${arch_dir}` path and digest exactly.
        plan = QtPlan("windows", "desktop", "6.8.3", "win64_msvc2022_64", str(outdir), "qtgraphs")
        digest = hashlib.sha256(b"qtgraphs\n").hexdigest()
        assert gh_output.read_text().splitlines() == [
            "version=6.8.3",
            "installer=aqt",
            f"key=qt-windows-desktop-win64_msvc2022_64-6.8.3-{digest}",
            "cache_path=.qt/Qt/6.8.3/msvc2022_64",
            "preinstalled_root=",
            "save=false",
            f"plan={plan.to_json()}",
        ]

    def test_qgc_installs_get_their_own_digest(self, monkeypatch, tmp_path, gh_output) -> None:
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
        argv = ["plan", "--outdir", str(tmp_path / ".qt/Qt"), "--version", "6.12.0"]
        argv += ["--host", "linux", "--target", "android", "--arch", "android_arm64_v8a"]
        prefix = "qt-linux-android-android_arm64_v8a-6.12.0-"

        assert install_qt.main([*argv, "--archives", "qtbase"]) == 0
        outputs = read_github_file(gh_output)
        assert outputs["installer"] == "qgc"
        assert outputs["key"] == prefix + compute_cache_digest("", "qtbase", "qgc")

        gh_output.write_text("")
        assert install_qt.main([*argv, "--archives", "qtbase", "--installer", "aqt"]) == 0
        assert read_github_file(gh_output)["key"] == prefix + compute_cache_digest("", "qtbase")

    def test_version_defaults_to_build_config(self, monkeypatch, tmp_path, gh_output) -> None:
        config = tmp_path / "build-config.json"
        config.write_text(json.dumps({"qt": {"version": "6.9.1", "modules": "qtgraphs"}}))
        monkeypatch.setenv("CONFIG_FILE", str(config))
        monkeypatch.setenv("GITHUB_WORKSPACE", str(tmp_path))
        outdir = tmp_path / ".qt" / "Qt"

        assert install_qt.main(["plan", "--outdir", str(outdir), "--arch", "linux_gcc_64"]) == 0

        outputs = gh_output.read_text().splitlines()
        assert outputs[0] == "version=6.9.1"
        assert "cache_path=.qt/Qt/6.9.1/gcc_64" in outputs
        # An empty --version also selects the configured version.
        gh_output.write_text("")
        argv = ["plan", "--outdir", str(outdir), "--arch", "linux_gcc_64", "--version", ""]
        assert install_qt.main(argv) == 0
        assert gh_output.read_text().splitlines()[0] == "version=6.9.1"

    def test_reports_compatible_preinstalled_sdk_and_saves_only_fresh_installs(
        self, tmp_path
    ) -> None:
        prefix = tmp_path / "sdk"
        qt_root = TestResolvePreinstalledQt._create_sdk(prefix)
        request = QtPlan("linux", "desktop", "6.8.3", "linux_gcc_64", str(tmp_path / ".qt/Qt"))

        plan = plan_install(replace(request, modules="qtlocation"), str(prefix), save_allowed=True)

        assert plan["preinstalled_root"] == str(qt_root)
        assert plan["save"] == "false"
        assert QtPlan.from_json(plan["plan"]).preinstalled_root == str(qt_root)
        fresh = plan_install(request, save_allowed=True)
        assert (fresh["preinstalled_root"], fresh["save"]) == ("", "true")
        assert plan_install(request)["save"] == "false"

    @pytest.mark.parametrize("value", ["true", "false", ""])
    def test_save_follows_the_workflow_permission(self, tmp_path, gh_output, value) -> None:
        argv = ["plan", "--outdir", str(tmp_path), "--version", "6.12.0", "--arch", "clang_64"]
        assert install_qt.main([*argv, "--host", "mac", "--save-allowed", value]) == 0
        assert read_github_file(gh_output)["save"] == (value or "false")


def _former_cache_key(host: str, target: str, arch: str, version: str, digest: str) -> str:
    """Render the key template qt-install's restore step used before `plan` computed the key."""
    template = (
        "qt-${{ inputs.host }}-${{ inputs.target }}-${{ inputs.arch }}-"
        "${{ steps.plan.outputs.version }}-${{ steps.plan.outputs.digest }}"
    )
    values = {
        "inputs.host": host,
        "inputs.target": target,
        "inputs.arch": arch,
        "steps.plan.outputs.version": version,
        "steps.plan.outputs.digest": digest,
    }
    for name, value in values.items():
        template = template.replace(f"${{{{ {name} }}}}", value)
    return template


def _former_digest(modules: str, archives: str, installer: str) -> str:
    content = f"{modules}\n{archives}" + ("" if installer == "aqt" else f"\ninstaller={installer}")
    return hashlib.sha256(content.encode()).hexdigest()


_CONFIG_QT = load_build_config()["qt"]
_DESKTOP_MODULES = _CONFIG_QT["modules"]
_IOS_MODULES = derive_ios_qt_modules(_DESKTOP_MODULES)
# Every request qt-install receives: build-setup's desktop, Windows ARM64 host, iOS and Android Qt
# (android.yml builds on Linux, macOS and Windows hosts), and pre-commit's and lupdate's subsets.
_QT_REQUESTS = [
    *(
        (host, "desktop", arch, _DESKTOP_MODULES, "")
        for host, arch in (
            ("linux", "linux_gcc_64"),
            ("linux_arm64", "linux_gcc_arm64"),
            ("mac", "clang_64"),
            ("windows", "win64_msvc2022_64"),
            ("windows_arm64", "win64_msvc2022_arm64"),
            ("windows", "win64_msvc2022_arm64_cross_compiled"),
        )
    ),
    *(("mac", "ios", arch, _IOS_MODULES, "") for arch in ("ios_device", "ios_simulator_x86_64")),
    *(
        (host, "android", arch, _DESKTOP_MODULES, "")
        for host in ("linux", "mac", "windows")
        for arch in ("android_arm64_v8a", "android_armv7", "android_x86_64")
    ),
    ("linux", "desktop", "linux_gcc_64", "", "qtbase qtdeclarative icu"),
    ("linux", "desktop", "linux_gcc_64", "", "qtbase qttools qtdeclarative icu"),
]


@pytest.mark.parametrize("installer", ["qgc", "aqt"])
@pytest.mark.parametrize(("host", "target", "arch", "modules", "archives"), _QT_REQUESTS)
def test_cache_key_is_byte_identical_to_the_former_action_key(
    monkeypatch, tmp_path, gh_output, installer, host, target, arch, modules, archives
) -> None:
    monkeypatch.setenv("QGC_QT_INSTALLER", installer)
    argv = ["plan", "--outdir", str(tmp_path), "--host", host, "--target", target]
    argv += ["--arch", arch, "--modules", modules, "--archives", archives]

    assert install_qt.main(argv) == 0

    version = _CONFIG_QT["version"]
    expected = _former_cache_key(
        host, target, arch, version, _former_digest(modules, archives, installer)
    )
    assert read_github_file(gh_output)["key"] == expected


class TestEnsure:
    @staticmethod
    def _plan(tmp_path: Path, **changes: str) -> QtPlan:
        plan = QtPlan("linux", "desktop", "6.8.3", "linux_gcc_64", str(tmp_path / "Qt"))
        return replace(plan, **changes)

    @staticmethod
    def _ensure(plan: QtPlan, *extra: str) -> int:
        return install_qt.main(["ensure", "--plan", plan.to_json(), *extra])

    @pytest.fixture
    def aqt_calls(self, monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
        calls: list[list[str]] = []
        monkeypatch.setattr(install_qt, "tool_command", lambda *args, **kwargs: ["aqt"])
        monkeypatch.setattr(install_qt, "_run_aqt_with_retries", calls.append)
        return calls

    def test_preinstalled_sdk_is_exported_without_install(
        self, tmp_path, aqt_calls, gh_output, gh_env, gh_path
    ) -> None:
        qt_root = tmp_path / "sdk/Qt/6.8.3/gcc_64"
        plan = self._plan(tmp_path, preinstalled_root=str(qt_root))

        assert self._ensure(plan, "--cache-hit", "") == 0

        assert aqt_calls == []
        assert gh_output.read_text() == f"qt_root_dir={qt_root}\nsource=preinstalled\n"
        assert gh_env.read_text() == f"QT_ROOT_DIR={qt_root}\n"
        assert gh_path.read_text() == f"{qt_root / 'bin'}\n"

    def test_cache_hit_reuses_restored_tree(self, tmp_path, aqt_calls, gh_output) -> None:
        qt_root = tmp_path / "Qt/6.8.3/gcc_64"
        qt_root.mkdir(parents=True)

        assert self._ensure(self._plan(tmp_path), "--cache-hit", "true") == 0

        assert aqt_calls == []
        assert gh_output.read_text() == f"qt_root_dir={qt_root}\nsource=cache\n"

    @pytest.mark.usefixtures("gh_output", "gh_env", "gh_path")
    def test_cache_hit_without_tree_fails(self, tmp_path, aqt_calls) -> None:
        with pytest.raises(SystemExit):
            self._ensure(self._plan(tmp_path), "--cache-hit", "true")
        assert aqt_calls == []

    def test_cache_miss_installs_without_exporting(
        self, tmp_path, aqt_calls, gh_output, gh_env, gh_path
    ) -> None:
        qt_root = tmp_path / "Qt/6.8.3/gcc_64"
        qt_root.mkdir(parents=True)

        argv = ["--cache-hit", "false", "--export-env", "false"]
        assert self._ensure(self._plan(tmp_path, installer="aqt"), *argv) == 0

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
        assert gh_output.read_text() == f"qt_root_dir={qt_root}\nsource=installed\n"
        assert gh_env.read_text() == ""
        assert gh_path.read_text() == ""

    def test_cache_miss_uses_the_planned_request(
        self, monkeypatch, tmp_path, gh_output, qgc_installs
    ) -> None:
        # The installer override applies once, when planning: the cache digest covers it.
        monkeypatch.setenv("QGC_QT_INSTALLER", "qgc")
        argv = ["plan", "--outdir", str(tmp_path / "Qt"), "--version", "6.8.3", "--host", "mac"]
        argv += ["--target", "ios", "--arch", "ios_device", "--modules", "qtgraphs"]
        argv += ["--archives", "qtbase"]
        assert install_qt.main(argv) == 0
        plan = read_github_file(gh_output)["plan"]
        monkeypatch.setenv("QGC_QT_INSTALLER", "aqt")
        gh_output.write_text("")

        assert install_qt.main(["ensure", "--plan", plan, "--export-env", "false"]) == 0

        request = QtRequest("mac", "ios", "6.8.3", "ios_device", ("qtgraphs",), ("qtbase",))
        assert qgc_installs == [("install", request)]
        assert read_github_file(gh_output) == {
            "qt_root_dir": str(tmp_path / "Qt/6.8.3/ios_device"),
            "source": "installed",
        }


class TestCompare:
    def test_reports_parity_in_the_job_summary(self, tmp_path, gh_summary, capsys) -> None:
        for name in ("aqt", "qgc"):
            write_files(tmp_path / name / "6.12.0/msvc2022_arm64", {"bin/qmake.exe": "x"})
        argv = ["compare", "--version", "6.12.0", "--arch", "win64_msvc2022_arm64_cross_compiled"]
        argv += ["--reference", str(tmp_path / "aqt"), "--candidate", str(tmp_path / "qgc")]

        assert install_qt.main(argv) == 0
        summary = gh_summary.read_text()
        assert summary.startswith(
            "### Qt installer parity: Qt 6.12.0 win64_msvc2022_arm64_cross_compiled\n"
        )
        assert "| Result | identical apart from known differences |" in summary
        assert summary in capsys.readouterr().out

        write_files(tmp_path / "qgc/6.12.0/msvc2022_arm64", {"bin/qmake.exe": "y"})
        assert install_qt.main(argv) == 1


def test_docker_image_ships_the_qt_installer(tmp_path) -> None:
    dockerfile = (TOOLS_DIR.parent / "deploy/docker/Dockerfile").read_text(encoding="utf-8")
    assert "COPY tools/setup/qt_installer/ /tmp/tools/setup/qt_installer/\n" in dockerfile
    tools = tmp_path / "tools"
    for package in ("common", "qgc_tools", "setup/qt_installer"):
        shutil.copytree(TOOLS_DIR / package, tools / package)
    shutil.copy2(TOOLS_DIR / "_bootstrap.py", tools)
    shutil.copy2(TOOLS_DIR / "setup/install_qt.py", tools / "setup")

    result = subprocess.run(
        [sys.executable, "-I", "-S", str(tools / "setup/install_qt.py"), "--help"],
        capture_output=True,
        text=True,
        check=False,
    )
    assert result.returncode == 0, result.stderr


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
