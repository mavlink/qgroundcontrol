"""Tests for tools/setup/build_gstreamer.py and its setup/gstreamer package."""

from __future__ import annotations

import io
import tarfile
import tempfile
from pathlib import Path
from unittest.mock import patch

import pytest
from common.build_config import load_build_config
from setup.gstreamer import cli
from setup.gstreamer.cerbero import CerberoBuilder
from setup.gstreamer.config import (
    PLUGIN_KEYS,
    PLUGIN_OPTIONS,
    PLUGINS_WITHOUT_OPTION,
    BuildConfig,
    plugin_options,
)
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


def test_a_windows_build_needs_neither_pkg_config_nor_python3_on_path(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    monkeypatch.setattr("common.proc.shutil.which", lambda name: name if name == "git" else None)
    MesonBuilder(BuildConfig("windows", "x64", "1.28.9", work_dir=tmp_path)).check_dependencies()


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


def _main_config(monkeypatch: pytest.MonkeyPatch, *args: str) -> BuildConfig:
    monkeypatch.setattr("sys.argv", ["build_gstreamer.py", "--version", "1.24.0", *args])
    with (
        patch.object(cli, "MesonBuilder") as meson,
        patch.object(cli, "CerberoBuilder") as cerbero,
    ):
        assert cli.main() == 0
    return (meson if meson.called else cerbero).call_args.args[0]


def test_main_defaults_the_prefix_under_the_work_dir(
    monkeypatch: pytest.MonkeyPatch, tmp_path: Path
) -> None:
    config = _main_config(
        monkeypatch, "--platform", "windows", "--arch", "x86_64", "--work-dir", str(tmp_path)
    )

    assert config.prefix == tmp_path / "gst-windows-x64"


def test_get_meson_args_contains_prefix(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert f"--prefix={cfg.prefix}" in args
    assert "--buildtype=release" in args


@pytest.mark.parametrize(
    ("plat", "expected"),
    [
        # Linux links the system's libraries and builds FFmpeg alone; the others have none to link.
        ("linux", ["--wrap-mode=nofallback", "--force-fallback-for=FFmpeg"]),
        ("macos", ["--wrap-mode=forcefallback"]),
        ("windows", ["--wrap-mode=forcefallback"]),
    ],
)
def test_get_meson_args_force_fallbacks_only_where_the_system_has_no_libraries(
    plat: str, expected: list[str], tmp_path: Path
) -> None:
    args = MesonBuilder(BuildConfig(plat, "arm64", "1.28.9", work_dir=tmp_path)).get_meson_args()
    assert [a for a in args if a.startswith(("--wrap-mode", "--force-fallback-for"))] == expected


@pytest.mark.parametrize(
    ("arch", "x86_assembly"), [("x86_64", True), ("x64", True), ("arm64", False)]
)
def test_get_meson_args_keep_what_disabled_auto_features_would_drop(
    arch: str, x86_assembly: bool, tmp_path: Path
) -> None:
    """A real build showed what each one's absence costs, down to a libav plugin without a codec."""
    args = MesonBuilder(BuildConfig("linux", arch, "1.28.9", work_dir=tmp_path)).get_meson_args()
    assert {"-Dauto_features=disabled", "-Dtools=enabled", "-Dgstreamer:check=enabled"} <= set(args)
    # What QGC builds against besides the plugins: the RTSP server library, and libdrm through the allocators.
    assert {"-Drtsp_server=enabled", "-Dgst-plugins-base:drm=enabled"} <= set(args)
    assert {"-DFFmpeg:h264_decoder=enabled", "-DFFmpeg:hevc_decoder=enabled"} <= set(args)
    # The MPEG-4 decoder is left out without the H.263 decoder and parser it is built on.
    assert {
        "-DFFmpeg:mpeg4_decoder=enabled",
        "-DFFmpeg:h263_decoder=enabled",
        "-DFFmpeg:h263_parser=enabled",
    } <= set(args)
    assert ("-DFFmpeg:x86asm=enabled" in args) == x86_assembly
    # Read only by a static build, which this is not.
    assert not any(a.startswith("-Dgst-full") for a in args)


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


def test_a_linux_build_gives_what_it_installs_a_run_path_into_the_prefix(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.28.9", work_dir=tmp_path)
    args = MesonBuilder(cfg).get_meson_args()
    # Meson's default library directory on Debian is lib/<multiarch>, which is not known before configure.
    assert "--libdir=lib" in args
    for lang in ("c", "cpp"):
        assert f"-D{lang}_link_args=['-Wl,-rpath,{tmp_path / 'gst-linux-x86_64' / 'lib'}']" in args


def test_get_meson_args_includes_plugins(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    builder = MesonBuilder(cfg)
    args = builder.get_meson_args()
    assert "-Dgst-plugins-base:gl=enabled" in args
    assert "-Dgst-plugins-bad:videoparsers=enabled" in args


@pytest.mark.parametrize("plat", ["linux", "macos", "windows"])
def test_meson_builds_every_allowlisted_plugin(plat: str) -> None:
    allowlist = load_build_config()["gstreamer"]["plugins"]
    options = plugin_options(plat, allowlist)
    for name in [*allowlist["common"], *allowlist[PLUGIN_KEYS[plat]]]:
        assert name in PLUGINS_WITHOUT_OPTION or options.get(PLUGIN_OPTIONS[name]) == "enabled", (
            name
        )
    assert options["gst-plugins-base:videoconvertscale"] == "enabled"


def test_plugin_options_reject_an_unmapped_plugin() -> None:
    with pytest.raises(ValueError, match="No Meson option for GStreamer plugins: newplugin"):
        plugin_options("linux", {"common": ["app", "newplugin"]})
    with pytest.raises(ValueError, match="No Meson option for GStreamer plugins: newplugin"):
        plugin_options("linux", {"common": ["app"]}, ["newplugin"])


def test_meson_builds_what_a_debug_qgc_and_its_tests_make_streams_with(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.28.9", work_dir=tmp_path)
    args = MesonBuilder(cfg).get_meson_args()
    debug_plugins = load_build_config()["gstreamer_plugin_policy"]["mock_server_debug"]
    assert debug_plugins
    for name in debug_plugins:
        assert f"-D{PLUGIN_OPTIONS[name]}=enabled" in args, name
    assert "-DFFmpeg:mpeg2video_encoder=enabled" in args


def test_get_meson_args_use_only_gstreamer_1_28_options(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.28.4", work_dir=tmp_path)
    args = MesonBuilder(cfg).get_meson_args()
    assert "-Dgst-plugins-bad:va=enabled" in args
    assert not any(a.startswith("-Dvaapi=") for a in args)


def test_get_meson_args_builds_no_gpl_or_qt6_plugin(tmp_path: Path) -> None:
    cfg = BuildConfig(platform="linux", arch="x86_64", version="1.24.0", work_dir=tmp_path)
    args = MesonBuilder(cfg).get_meson_args()
    assert {"-Dgpl=disabled", "-Dugly=disabled", "-Dqt6=disabled"} <= set(args)
    assert not any("x264" in a or "x265" in a or ":qt" in a for a in args)


@pytest.mark.parametrize(
    ("plat", "arch", "simulator", "expected"),
    [
        ("android", "arm64", False, ["cross-android-arm64"]),
        ("android", "x86_64", False, ["cross-android-x86-64"]),
        ("android", "universal", False, ["cross-android-universal"]),
        ("android", "universal", True, ["cross-android-universal"]),
        # The device with the simulator, as the official SDK holds: not cross-ios-universal, whose one
        # library mixes the two.
        ("ios", "universal", False, ["cross-ios-arm64", "cross-ios-sim-universal"]),
        ("ios", "arm64", False, ["cross-ios-arm64"]),
        ("ios", "arm64", True, ["cross-ios-sim-arm64"]),
        ("ios", "x86_64", True, ["cross-ios-sim-x86-64"]),
        ("ios", "universal", True, ["cross-ios-sim-universal"]),
    ],
)
def test_cerbero_config_names_match_the_pinned_cerbero_configs(
    tmp_path: Path, plat: str, arch: str, simulator: bool, expected: list[str]
) -> None:
    cfg = BuildConfig(plat, arch, "1.28.9", work_dir=tmp_path, simulator=simulator)
    assert list(CerberoBuilder(cfg).get_config_names()) == expected


class _FakeCerbero:
    """Stands in for git and Cerbero: records the commands and writes the package files they would."""

    def __init__(self, packages: dict[str, dict[str, list[str]]]) -> None:
        # The files each Cerbero command leaves, by its config (or "xcframework"), with the tree each holds.
        self.packages = packages
        self.commands: list[list[str]] = []
        self.cwds: list[Path | None] = []

    def __call__(self, cmd: list[str], cwd: Path | None = None, **_: object) -> None:
        self.commands.append(cmd)
        self.cwds.append(cwd)
        command = next((arg for arg in cmd if arg in ("package", "xcframework")), None)
        if command is None:
            return
        output = Path(cmd[cmd.index("-o") + 1]) if "-o" in cmd else cwd
        assert output is not None
        key = cmd[cmd.index("-c") + 1] if command == "package" else command
        for name, tree in self.packages.get(key, {}).items():
            with tarfile.open(output / name, "w:xz") as tar:
                for member in tree:
                    info = tarfile.TarInfo(member)
                    info.size = 0
                    tar.addfile(info, io.BytesIO())


def _run_cerbero(monkeypatch: pytest.MonkeyPatch, cfg: BuildConfig, fake: _FakeCerbero) -> None:
    monkeypatch.setattr("setup.gstreamer.cerbero.run_cmd", fake)
    monkeypatch.setattr(CerberoBuilder, "check_dependencies", lambda self: None)
    CerberoBuilder(cfg).run()


_PC = "lib/pkgconfig/gstreamer-1.0.pc"
# What Cerbero's package command leaves for an Android universal build.
_ANDROID_PACKAGES = {
    "cross-android-universal": {
        "gstreamer-1.0-android-universal-1.28.9.tar.xz": [f"arm64/{_PC}", f"x86_64/{_PC}"],
        "gstreamer-1.0-android-universal-1.28.9-runtime.tar.xz": ["arm64/lib/libz.so"],
    }
}


@pytest.mark.parametrize(
    ("plat", "arch", "simulator"),
    [("ios", "x86_64", False), ("ios", "armv7", True), ("android", "aarch64", False)],
)
def test_cerbero_rejects_archs_without_a_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, plat: str, arch: str, simulator: bool
) -> None:
    fake = _FakeCerbero({})
    cfg = BuildConfig(plat, arch, "1.28.9", work_dir=tmp_path, simulator=simulator)
    with pytest.raises(ValueError, match=f"Unsupported arch {arch}"):
        _run_cerbero(monkeypatch, cfg, fake)
    assert fake.commands == []


@pytest.mark.parametrize(
    ("build_type", "jobs", "config_args", "jobs_args"),
    [
        ("release", None, [], []),
        # Cerbero's debug variant, the symbols, is on either way: a debug build is the unoptimized one.
        ("debug", None, ["-v", "nooptimization"], []),
        ("release", 6, ["-c", "{work}/cerbero-jobs.cbc"], ["--jobs", "6"]),
    ],
)
def test_cerbero_android_build_unpacks_the_sdk_into_the_prefix(
    tmp_path: Path,
    monkeypatch: pytest.MonkeyPatch,
    build_type: str,
    jobs: int | None,
    config_args: list[str],
    jobs_args: list[str],
) -> None:
    fake = _FakeCerbero(_ANDROID_PACKAGES)
    # A relative work directory: Cerbero, which runs in its own, is given absolute paths.
    monkeypatch.chdir(tmp_path)
    work = (tmp_path / "work").resolve()
    cfg = BuildConfig(
        "android", "universal", "1.28.9", build_type=build_type, work_dir=Path("work"), jobs=jobs
    )

    _run_cerbero(monkeypatch, cfg, fake)

    cerbero = str(work / "cerbero" / "cerbero-uninstalled")
    config = ["-c", "cross-android-universal", *(arg.format(work=work) for arg in config_args)]
    packages = str(work / "cerbero-packages")
    assert fake.commands[1:] == [
        [cerbero, *config, "bootstrap", *jobs_args],
        [cerbero, *config, "package", "gstreamer-1.0", *jobs_args, "-o", packages],
    ]
    if jobs:
        assert (work / "cerbero-jobs.cbc").read_text() == "num_of_cpus = 6\n"
    # The development tarball, with one tree per ABI: the runtime one has no headers or archives.
    assert cfg.prefix is not None
    assert (cfg.prefix / "arm64" / _PC).is_file()
    assert (cfg.prefix / "x86_64" / _PC).is_file()
    assert not (cfg.prefix / "arm64/lib/libz.so").exists()


def test_cerbero_ios_build_combines_device_and_simulator_into_an_xcframework(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    device = "gstreamer-1.0-1.28.9-ios-arm64.xcframework.tar.xz"
    simulator = "gstreamer-1.0-1.28.9-ios-simulator-universal.xcframework.tar.xz"
    fake = _FakeCerbero(
        {
            "cross-ios-arm64": {device: ["GStreamer.framework/Versions/1.0/lib/libGStreamer.a"]},
            "cross-ios-sim-universal": {
                simulator: ["GStreamer.framework/Versions/1.0/lib/libGStreamer.a"]
            },
            "xcframework": {
                "gstreamer-1.28.9-xcframework.tar.xz": ["GStreamer.xcframework/Info.plist"]
            },
        }
    )
    cfg = BuildConfig("ios", "universal", "1.28.9", work_dir=tmp_path)

    _run_cerbero(monkeypatch, cfg, fake)

    cerbero = str(tmp_path.resolve() / "cerbero" / "cerbero-uninstalled")
    packages = tmp_path.resolve() / "cerbero-packages"
    assert [cmd[1:] for cmd in fake.commands[1:]] == [
        ["-c", "cross-ios-arm64", "bootstrap"],
        [
            "-c",
            "cross-ios-arm64",
            "package",
            "gstreamer-1.0",
            "--artifact=xcframework",
            "-o",
            str(packages),
        ],
        ["-c", "cross-ios-sim-universal", "bootstrap"],
        [
            "-c",
            "cross-ios-sim-universal",
            "package",
            "gstreamer-1.0",
            "--artifact=xcframework",
            "-o",
            str(packages),
        ],
        # Cerbero reads the version from the start of the first source, so they are file names in
        # the directory the command runs in.
        ["xcframework", "gstreamer-1.0", "--source", device, "--source", simulator],
    ]
    assert all(cmd[0] == cerbero for cmd in fake.commands[1:])
    assert fake.cwds[-1] == packages
    assert cfg.prefix is not None
    assert (cfg.prefix / "GStreamer.xcframework" / "Info.plist").is_file()


def test_cerbero_replaces_only_a_prefix_it_filled(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    cfg = BuildConfig("android", "universal", "1.28.9", work_dir=tmp_path)
    assert cfg.prefix is not None
    cfg.prefix.mkdir()
    (cfg.prefix / "notes.txt").write_text("mine")

    # Checked before the build, which takes hours.
    fake = _FakeCerbero(_ANDROID_PACKAGES)
    with pytest.raises(RuntimeError, match="is not empty and holds no SDK of an earlier build"):
        _run_cerbero(monkeypatch, cfg, fake)
    assert fake.commands == []
    assert (cfg.prefix / "notes.txt").read_text() == "mine"

    # The SDK of an earlier build goes, with whatever it had that the new one lacks.
    (cfg.prefix / "notes.txt").unlink()
    _run_cerbero(monkeypatch, cfg, fake)
    (cfg.prefix / "armv7").mkdir()
    _run_cerbero(monkeypatch, cfg, _FakeCerbero(_ANDROID_PACKAGES))
    assert (cfg.prefix / "arm64" / _PC).is_file()
    assert not (cfg.prefix / "armv7").exists()


@pytest.mark.parametrize(
    ("packages", "message"),
    [
        (
            {"gstreamer-1.0-android-universal-1.28.9-runtime.tar.xz": ["arm64/lib/libz.so"]},
            "Expected one package from Cerbero config cross-android-universal, found: none",
        ),
        (
            {"gstreamer-1.0-android-universal-1.28.9.tar.xz": ["arm64/include/gst/gst.h"]},
            "holds no GStreamer SDK that QGC can use",
        ),
    ],
)
def test_cerbero_fails_without_an_sdk_in_its_packages(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch, packages: dict[str, list[str]], message: str
) -> None:
    cfg = BuildConfig("android", "universal", "1.28.9", work_dir=tmp_path)
    fake = _FakeCerbero({"cross-android-universal": packages})
    with pytest.raises(RuntimeError, match=message):
        _run_cerbero(monkeypatch, cfg, fake)
    # Nothing half-made is left where configure would be pointed.
    assert cfg.prefix is not None
    assert not cfg.prefix.exists()


MACH_O = bytes.fromhex("cffaedfe")


class _FakeMeson:
    """Stands in for git, meson and lipo: creates the files the builder reads afterwards."""

    def __init__(self) -> None:
        self.commands: list[list[str]] = []
        self.envs: list[dict] = []

    def __call__(self, cmd: list, cwd: Path | None = None, env: dict | None = None, **_) -> None:
        cmd = [str(c) for c in cmd]
        self.commands.append(cmd)
        self.envs.append(env or {})
        if cmd[:2] == ["git", "clone"]:
            # What GStreamer's Windows build downloads while it configures.
            (Path(cmd[-1]) / "subprojects/win-pkgconfig").mkdir(parents=True)
            (Path(cmd[-1]) / "subprojects/win-pkgconfig/pkg-config.exe").write_bytes(b"MZ")
        elif cmd[:2] == ["meson", "setup"]:
            build_dir = Path(cmd[2])
            build_dir.mkdir(parents=True)
            (build_dir / "build.ninja").touch()
            prefix = next(a.removeprefix("--prefix=") for a in cmd if a.startswith("--prefix="))
            (build_dir / "prefix").write_text(prefix, encoding="utf-8")
        elif cmd[:2] == ["meson", "install"]:
            build_dir = Path(cmd[3])
            root = Path((build_dir / "prefix").read_text(encoding="utf-8"))
            if "--destdir" in cmd:
                root = Path(cmd[cmd.index("--destdir") + 1]) / root.relative_to(root.anchor)
            arch = build_dir.name.removeprefix("builddir-").encode()
            for binary in (
                "lib/libgstreamer-1.0.0.dylib",
                "libexec/gstreamer-1.0/gst-plugin-scanner",
            ):
                (root / binary).parent.mkdir(parents=True, exist_ok=True)
                (root / binary).write_bytes(MACH_O + arch)
            (root / "lib/libgstreamer-1.0.dylib").unlink(missing_ok=True)
            (root / "lib/libgstreamer-1.0.dylib").symlink_to("libgstreamer-1.0.0.dylib")
            (root / "lib/pkgconfig").mkdir(exist_ok=True)
            (root / "lib/pkgconfig/gstreamer-1.0.pc").write_bytes(b"arch=" + arch)
        elif cmd[0] == "lipo":
            Path(cmd[-1]).write_bytes(b"".join(Path(slice_).read_bytes() for slice_ in cmd[2:4]))

    def matching(self, *head: str) -> list[list[str]]:
        return [c for c in self.commands if c[: len(head)] == list(head)]


def test_macos_universal_builds_each_arch_for_the_prefix_and_merges_every_binary(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake = _FakeMeson()
    monkeypatch.setattr("setup.gstreamer.meson.run_cmd", fake)
    cfg = BuildConfig("macos", "universal", "1.28.4", work_dir=tmp_path)
    prefix = tmp_path / "gst-macos-universal"
    assert cfg.prefix == prefix
    # The prefix may be a directory other things are installed in: nothing else in it is removed.
    (prefix / "share").mkdir(parents=True)
    (prefix / "share/kept").touch()
    builder = MesonBuilder(cfg)

    builder.build_universal()
    builder.build_universal()

    assert len(fake.matching("git", "clone")) == 1
    setups = fake.matching("meson", "setup")
    assert [Path(c[2]).name for c in setups] == ["builddir-x86_64", "builddir-arm64"]
    for setup, arch in zip(setups, ("x86_64", "arm64"), strict=True):
        # Each arch is built for the final prefix, which its install names and .pc files then name.
        assert f"--prefix={prefix}" in setup
        assert f"-Dc_link_args=['-arch', '{arch}']" in setup
    assert all("CC" not in env and "CXX" not in env for env in fake.envs)
    installs = fake.matching("meson", "install")
    assert [Path(c[3]).name for c in installs] == ["builddir-x86_64", "builddir-arm64"] * 2
    assert [Path(c[c.index("--destdir") + 1]).name for c in installs[:2]] == [
        "gst-macos-universal-x86_64",
        "gst-macos-universal-arm64",
    ]

    # Libraries and programs hold both slices; the symlink, the text file and the stranger are as they were.
    for binary in ("lib/libgstreamer-1.0.0.dylib", "libexec/gstreamer-1.0/gst-plugin-scanner"):
        assert (prefix / binary).read_bytes() == MACH_O + b"x86_64" + MACH_O + b"arm64"
    assert (prefix / "lib/libgstreamer-1.0.dylib").readlink() == Path("libgstreamer-1.0.0.dylib")
    assert (prefix / "lib/pkgconfig/gstreamer-1.0.pc").read_bytes() == b"arch=arm64"
    assert (prefix / "share/kept").exists()
    assert len(fake.matching("lipo")) == 4


def test_macos_universal_rejects_a_build_dir_configured_for_another_prefix(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    fake = _FakeMeson()
    monkeypatch.setattr("setup.gstreamer.meson.run_cmd", fake)
    MesonBuilder(
        BuildConfig("macos", "universal", "1.28.4", prefix=tmp_path / "first", work_dir=tmp_path)
    ).build_universal()

    with pytest.raises(RuntimeError, match="configured for another prefix"):
        MesonBuilder(
            BuildConfig(
                "macos", "universal", "1.28.4", prefix=tmp_path / "second", work_dir=tmp_path
            )
        ).build_universal()
    assert not (tmp_path / "second").exists()


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


def _run_meson(monkeypatch: pytest.MonkeyPatch, cfg: BuildConfig) -> _FakeMeson:
    fake = _FakeMeson()
    monkeypatch.setattr("setup.gstreamer.meson.run_cmd", fake)
    monkeypatch.setattr(MesonBuilder, "check_dependencies", lambda self: None)
    monkeypatch.setattr(MesonBuilder, "ensure_meson", lambda self: None)
    MesonBuilder(cfg).run()
    return fake


def test_a_windows_build_puts_pkg_config_where_configure_runs_it(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    cfg = BuildConfig("windows", "x64", "1.28.9", work_dir=tmp_path)
    _run_meson(monkeypatch, cfg)
    assert cfg.prefix is not None
    # cmake/GStreamer/acquire/Windows.cmake takes no SDK without it.
    assert (cfg.prefix / "bin/pkg-config.exe").read_bytes() == b"MZ"

    (cfg.source_dir / "subprojects/win-pkgconfig/pkg-config.exe").unlink()
    with pytest.raises(RuntimeError, match="left no pkg-config"):
        _run_meson(monkeypatch, cfg)


def test_a_linux_build_copies_no_pkg_config(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    cfg = BuildConfig("linux", "x86_64", "1.28.9", work_dir=tmp_path)
    _run_meson(monkeypatch, cfg)
    assert cfg.prefix is not None
    assert not (cfg.prefix / "bin").exists()
