"""Tests for tools/setup/build_gstreamer.py and its setup/gstreamer package."""

from __future__ import annotations

import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest
from common.build_config import load_build_config
from setup.gstreamer import ci_plan, cli
from setup.gstreamer.cerbero import CerberoBuilder
from setup.gstreamer.config import BuildConfig
from setup.gstreamer.host import detect_host_arch, detect_jobs
from setup.gstreamer.meson import MesonBuilder


def test_detect_jobs_with_override() -> None:
    assert detect_jobs(override=8) == 8


def test_detect_jobs_without_override() -> None:
    result = detect_jobs()
    assert result >= 1


def test_detect_jobs_fallback_when_cpu_count_none() -> None:
    with patch("os.cpu_count", return_value=None):
        assert detect_jobs() == 4


@pytest.mark.parametrize(
    "machine, expected",
    [
        ("x86_64", "x86_64"),
        ("amd64", "x86_64"),
        ("aarch64", "arm64"),
        ("arm64", "arm64"),
        ("armv7l", "armv7"),
    ],
)
def test_detect_host_arch(machine: str, expected: str) -> None:
    with patch("platform.machine", return_value=machine):
        assert detect_host_arch() == expected


def test_build_config_defaults(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    assert cfg.prefix == tmp_path / "gst-linux-x86_64"
    assert cfg.source_dir == tmp_path / "gstreamer"
    assert cfg.build_dir == tmp_path / "gstreamer" / "builddir-x86_64"
    assert cfg.build_type == "release"


def test_build_config_custom_prefix(tmp_path: Path) -> None:
    custom = tmp_path / "custom"
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", prefix=custom)
    assert cfg.prefix == custom


@pytest.mark.parametrize("work_dir_args", [[], ["--work-dir", ""]])
def test_main_without_work_dir_uses_system_temp(work_dir_args: list[str]) -> None:
    argv = ["build_gstreamer.py", "--platform", "linux", "--version", "1.24.0", *work_dir_args]
    with patch("sys.argv", argv), patch.object(cli, "MesonBuilder") as builder:
        assert cli.main() == 0
    builder.return_value.run.assert_called_once_with()
    assert builder.call_args.args[0].work_dir == Path(tempfile.gettempdir())


def test_main_reports_missing_build_tools(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, capsys: pytest.CaptureFixture[str]
) -> None:
    monkeypatch.setattr("common.proc.shutil.which", lambda name: None if name != "git" else name)
    argv = ["build_gstreamer.py", "--platform", "linux", "--version", "1.24.0"]
    with patch("sys.argv", [*argv, "--work-dir", str(tmp_path)]):
        assert cli.main() == 1
    assert "python3, pkg-config not found on PATH" in capsys.readouterr().err


@pytest.mark.parametrize(
    ("plat", "arch", "expected"),
    [
        ("windows", "x86_64", "x64"),
        ("windows", "x64", "x64"),
        ("windows", "arm64", "arm64"),
        ("linux", "x86_64", "x86_64"),
        ("linux", "aarch64", "aarch64"),
        ("macos", "", "universal"),
    ],
)
def test_resolve_arch_uses_the_windows_toolchain_name(plat: str, arch: str, expected: str) -> None:
    assert cli.resolve_arch(plat, arch) == expected


def test_resolve_arch_renames_the_default_windows_host_arch() -> None:
    with patch("platform.machine", return_value="AMD64"):
        assert cli.resolve_arch("windows", "") == "x64"


def test_ci_prefix_is_unset_off_github_actions() -> None:
    assert cli.ci_prefix("linux", "x86_64") is None
    assert cli.ci_prefix("windows", "x64") is None


def _main_config(monkeypatch: pytest.MonkeyPatch, *args: str) -> BuildConfig:
    monkeypatch.setattr("sys.argv", ["build_gstreamer.py", "--version", "1.24.0", *args])
    with (
        patch.object(cli, "MesonBuilder") as meson,
        patch.object(cli, "CerberoBuilder") as cerbero,
    ):
        assert cli.main() == 0
    return (meson if meson.called else cerbero).call_args.args[0]


@pytest.mark.parametrize(
    ("args", "arch", "windows_prefix"),
    [
        (["--platform", "linux", "--arch", "aarch64"], "aarch64", None),
        (["--platform", "macos", "--arch", "universal"], "universal", None),
        (["--platform", "android", "--arch", "universal"], "universal", None),
        (["--platform", "windows", "--arch", "x86_64"], "x64", "C:\\gstreamer-x64"),
        (["--platform", "windows", "--arch", "arm64"], "arm64", "C:\\gstreamer-arm64"),
    ],
)
def test_main_on_github_actions_outputs_the_resolved_arch_and_prefix(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    gh_output: Path,
    args: list[str],
    arch: str,
    windows_prefix: str | None,
) -> None:
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    prefix = windows_prefix or str(tmp_path / "gstreamer")

    config = _main_config(monkeypatch, *args)

    assert config.arch == arch
    assert str(config.prefix) == prefix
    assert gh_output.read_text(encoding="utf-8") == f"arch={arch}\nprefix={prefix}\n"


def test_main_explicit_prefix_overrides_the_ci_default(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, gh_output: Path
) -> None:
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    custom = tmp_path / "custom"

    config = _main_config(
        monkeypatch, "--platform", "windows", "--arch", "x86_64", "--prefix", str(custom)
    )

    assert config.prefix == custom
    assert gh_output.read_text(encoding="utf-8") == f"arch=x64\nprefix={custom}\n"


def test_main_off_github_actions_defaults_the_prefix_under_the_work_dir(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    config = _main_config(
        monkeypatch, "--platform", "windows", "--arch", "x86_64", "--work-dir", str(tmp_path)
    )

    assert config.prefix == tmp_path / "gst-windows-x64"


@pytest.mark.parametrize(
    ("plat", "windows_work_dir", "qt_prefix"),
    [
        ("linux", False, False),
        ("macos", False, True),
        ("windows", True, True),
        ("ios", False, False),
    ],
)
def test_main_on_github_actions_defaults_the_work_dir_and_qt_prefix(
    monkeypatch: pytest.MonkeyPatch,
    tmp_path: Path,
    gh_output: Path,
    plat: str,
    windows_work_dir: bool,
    qt_prefix: bool,
) -> None:
    """The build-gstreamer.yml settings: Windows works in RUNNER_TEMP; macOS and Windows link Qt."""
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.setenv("QT_ROOT_DIR", str(tmp_path / "Qt"))

    config = _main_config(monkeypatch, "--platform", plat, "--arch", "arm64")

    assert config.work_dir == (tmp_path if windows_work_dir else Path(tempfile.gettempdir()))
    assert config.qt_prefix == (tmp_path / "Qt" if qt_prefix else None)


def test_main_explicit_work_dir_and_qt_prefix_override_the_ci_defaults(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path, gh_output: Path
) -> None:
    monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
    monkeypatch.setenv("QT_ROOT_DIR", str(tmp_path / "Qt"))
    args = ("--work-dir", str(tmp_path / "work"), "--qt-prefix", str(tmp_path / "other-qt"))

    config = _main_config(monkeypatch, "--platform", "windows", "--arch", "x64", *args)

    assert config.work_dir == tmp_path / "work"
    assert config.qt_prefix == tmp_path / "other-qt"


def test_qt_root_dir_is_ignored_off_github_actions(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.setenv("QT_ROOT_DIR", "/opt/Qt")
    assert cli.ci_qt_prefix("macos") is None
    assert cli.ci_work_dir("windows") is None


@pytest.mark.parametrize(
    ("plat", "arch", "simulator", "outputs"),
    [
        ("linux", "x86_64", False, ("linux", "linux_gcc_64", "gstreamer_build", "")),
        ("linux", "aarch64", False, ("linux_arm64", "linux_gcc_arm64", "gstreamer_build", "")),
        ("macos", "universal", False, ("mac", "clang_64", "", "")),
        ("macos", "x86_64", False, ("mac", "clang_64", "", "")),
        ("windows", "x86_64", False, ("windows", "win64_msvc2022_64", "", "")),
        ("windows", "x64", False, ("windows", "win64_msvc2022_64", "", "")),
        ("windows", "arm64", False, ("windows_arm64", "win64_msvc2022_arm64", "", "")),
        ("android", "armv7", False, ("", "", "gstreamer_build_android", "")),
        ("android", "universal", True, ("", "", "gstreamer_build_android", "")),
        ("ios", "arm64", False, ("", "", "", "")),
        ("ios", "x86_64", True, ("", "", "", "true")),
        # Only iOS has simulator builds; elsewhere the input is ignored, as before.
        ("linux", "x86_64", True, ("linux", "linux_gcc_64", "gstreamer_build", "")),
    ],
)
def test_ci_plan_resolves_the_runner_settings(
    gh_output: Path, plat: str, arch: str, simulator: bool, outputs: tuple[str, str, str, str]
) -> None:
    argv = ["--platform", plat, "--arch", arch, "--simulator", str(simulator).lower()]

    assert ci_plan.main(argv) == 0

    qt_host, qt_arch, apt_category, sim = outputs
    pins = load_build_config()["build"] if plat == "android" else {}
    assert gh_output.read_text(encoding="utf-8") == (
        f"qt-host={qt_host}\nqt-arch={qt_arch}\napt-category={apt_category}\nsimulator={sim}\n"
        f"cmake-version={pins.get('cmake_version', '')}\nninja-version={pins.get('ninja_version', '')}\n"
    )


@pytest.mark.parametrize(
    ("plat", "arch", "simulator"),
    [
        # runs-on gives these an x64 runner, which would build and mislabel an x86_64 SDK.
        ("linux", "arm64", False),
        ("linux", "universal", False),
        ("windows", "aarch64", False),
        ("windows", "x86", False),
        ("macos", "armv7", False),
        ("android", "aarch64", False),
        ("ios", "x86_64", False),
        ("ios", "armv7", True),
        ("freebsd", "x86_64", False),
    ],
)
def test_ci_plan_rejects_archs_the_platform_cannot_build(
    gh_output: Path, capsys: pytest.CaptureFixture[str], plat: str, arch: str, simulator: bool
) -> None:
    argv = ["--platform", plat, "--arch", arch, "--simulator", str(simulator).lower()]

    assert ci_plan.main(argv) == 1

    assert "::error title=Unsupported GStreamer build::" in capsys.readouterr().out
    assert gh_output.read_text(encoding="utf-8") == ""


def test_get_meson_args_contains_prefix(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert f"--prefix={cfg.prefix}" in args
    assert "--buildtype=release" in args
    assert "--wrap-mode=forcefallback" in args


def test_get_meson_args_macos_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="macos", arch="arm64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    for lang in ("c", "cpp", "objc", "objcpp"):
        assert f"-D{lang}_args=['-arch', 'arm64']" in args
        assert f"-D{lang}_link_args=['-arch', 'arm64']" in args


def test_get_meson_args_macos_universal_no_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="macos", arch="universal", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert not any("-Dcpp_args" in a for a in args)
    assert not any("-Dc_args" in a for a in args)


def test_get_meson_args_linux_no_arch_flags(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert not any("-Dcpp_args" in a for a in args)


def test_get_meson_args_includes_plugins(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert "-Dgst-plugins-base:gl=enabled" in args
    assert "-Dgst-plugins-good:qt6=enabled" in args
    assert "-Dgst-plugins-bad:videoparsers=enabled" in args
    assert "-Dgst-plugins-ugly:x264=enabled" in args


@pytest.mark.parametrize(
    ("plat", "arch", "simulator", "expected"),
    [
        ("android", "arm64", False, "cross-android-arm64"),
        ("android", "armv7", False, "cross-android-armv7"),
        ("android", "x86", False, "cross-android-x86"),
        ("android", "x86_64", False, "cross-android-x86-64"),
        ("android", "universal", False, "cross-android-universal"),
        ("android", "universal", True, "cross-android-universal"),
        ("ios", "arm64", False, "cross-ios-arm64"),
        ("ios", "universal", False, "cross-ios-universal"),
        ("ios", "arm64", True, "cross-ios-sim-arm64"),
        ("ios", "x86_64", True, "cross-ios-sim-x86-64"),
        ("ios", "universal", True, "cross-ios-sim-universal"),
    ],
)
def test_cerbero_config_name_matches_the_pinned_cerbero_configs(
    tmp_path: Path, plat: str, arch: str, simulator: bool, expected: str
) -> None:
    cfg = BuildConfig(plat, arch, "1.28.4", work_dir=tmp_path, simulator=simulator)
    assert CerberoBuilder(cfg).get_config_name() == expected


@pytest.mark.parametrize(
    ("plat", "arch", "simulator"),
    [("ios", "x86_64", False), ("ios", "armv7", True), ("android", "aarch64", False)],
)
def test_cerbero_rejects_archs_without_a_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, plat: str, arch: str, simulator: bool
) -> None:
    commands: list[list[str]] = []
    monkeypatch.setattr("setup.gstreamer.cerbero.run_cmd", lambda cmd, **_: commands.append(cmd))
    cfg = BuildConfig(plat, arch, "1.28.4", work_dir=tmp_path, simulator=simulator)
    with pytest.raises(ValueError, match=f"Unsupported arch {arch}"):
        CerberoBuilder(cfg).run()
    assert commands == []


@pytest.mark.parametrize(("build_type", "variants"), [("release", []), ("debug", ["-v", "debug"])])
def test_cerbero_universal_bootstraps_and_packages_once_with_one_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, build_type: str, variants: list[str]
) -> None:
    commands: list[list[str]] = []
    monkeypatch.setattr("setup.gstreamer.cerbero.run_cmd", lambda cmd, **_: commands.append(cmd))
    monkeypatch.setattr(CerberoBuilder, "check_dependencies", lambda self: None)
    cfg = BuildConfig("android", "universal", "1.28.4", build_type=build_type, work_dir=tmp_path)

    CerberoBuilder(cfg).run()

    cerbero = str(tmp_path / "cerbero" / "cerbero-uninstalled")
    assert commands[1:] == [
        [cerbero, "-c", "cross-android-universal", "bootstrap"],
        [
            cerbero,
            "-c",
            "cross-android-universal",
            *variants,
            "package",
            "gstreamer-1.0",
            "-o",
            str(cfg.prefix),
        ],
    ]


class _FakeMeson:
    """Stands in for git and meson: creates the files the macOS universal build reads."""

    def __init__(self) -> None:
        self.commands: list[list[str]] = []
        self.envs: list[dict] = []

    def __call__(self, cmd: list, cwd: Path | None = None, env: dict | None = None, **_) -> None:
        cmd = [str(c) for c in cmd]
        self.commands.append(cmd)
        self.envs.append(env or {})
        if cmd[:2] == ["git", "clone"]:
            Path(cmd[-1]).mkdir(parents=True)
        elif cmd[:2] == ["meson", "setup"]:
            build_dir = Path(cmd[2])
            build_dir.mkdir(parents=True)
            (build_dir / "build.ninja").touch()
            prefix = next(a.removeprefix("--prefix=") for a in cmd if a.startswith("--prefix="))
            (build_dir / "prefix").write_text(prefix, encoding="utf-8")
        elif cmd[:2] == ["meson", "install"]:
            prefix = Path((Path(cmd[3]) / "prefix").read_text(encoding="utf-8"))
            (prefix / "lib").mkdir(parents=True, exist_ok=True)
            (prefix / "lib" / "libgstreamer-1.0.dylib").write_text(prefix.name, encoding="utf-8")

    def matching(self, *head: str) -> list[list[str]]:
        return [c for c in self.commands if c[: len(head)] == list(head)]


def test_macos_universal_builds_each_arch_in_its_own_dir_from_one_clone(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake = _FakeMeson()
    monkeypatch.setattr("setup.gstreamer.meson.run_cmd", fake)
    cfg = BuildConfig("macos", "universal", "1.28.4", work_dir=tmp_path)
    builder = MesonBuilder(cfg)

    builder.build_universal()
    builder.build_universal()

    assert len(fake.matching("git", "clone")) == 1
    setups = fake.matching("meson", "setup")
    assert [Path(c[2]).name for c in setups] == ["builddir-x86_64", "builddir-arm64"]
    for setup, arch in zip(setups, ("x86_64", "arm64"), strict=True):
        assert f"--prefix={tmp_path / f'gst-macos-{arch}'}" in setup
        assert f"-Dc_link_args=['-arch', '{arch}']" in setup
    assert all("CC" not in env and "CXX" not in env for env in fake.envs)
    installs = [Path(c[3]).name for c in fake.matching("meson", "install")]
    assert installs == ["builddir-x86_64", "builddir-arm64"] * 2
    x86_lib, arm_lib = (
        tmp_path / f"gst-macos-{a}/lib/libgstreamer-1.0.dylib" for a in ("x86_64", "arm64")
    )
    universal_lib = tmp_path / "gst-macos-universal/lib/libgstreamer-1.0.dylib"
    assert fake.matching("lipo")[-1] == [
        "lipo",
        "-create",
        str(x86_lib),
        str(arm_lib),
        "-output",
        str(universal_lib),
    ]


def test_macos_universal_clean_reclones_once(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake = _FakeMeson()
    monkeypatch.setattr("setup.gstreamer.meson.run_cmd", fake)
    (tmp_path / "gstreamer" / "builddir-arm64").mkdir(parents=True)

    MesonBuilder(
        BuildConfig("macos", "universal", "1.28.4", work_dir=tmp_path, clean=True)
    ).build_universal()

    assert len(fake.matching("git", "clone")) == 1
    assert len(fake.matching("meson", "setup")) == 2
