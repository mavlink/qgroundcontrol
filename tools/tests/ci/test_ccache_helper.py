"""Tests for ccache_helper.py."""

from __future__ import annotations

import json
import os
import re
import stat
import subprocess
import sys
import tarfile
import zipfile
from pathlib import Path
from unittest.mock import patch

import pytest
from ci.cache import ccache_helper
from ci.cache.ccache_helper import (
    ccache_misses,
    main,
    moccache_misses,
    parse_min_misses,
    pinned_release,
    should_save_cache,
)
from common.build_config import load_build_config

from .._helpers import read_github_file

pytestmark = pytest.mark.windows

PINS = load_build_config()["tools"]
CCACHE_VERSION, CCACHE_SHA256 = PINS["ccache"]["version"], PINS["ccache"]["sha256"]
MOLD_VERSION, MOLD_SHA256 = PINS["mold"]["version"], PINS["mold"]["sha256"]
SCCACHE_VERSION, SCCACHE_SHA256 = PINS["sccache"]["version"], PINS["sccache"]["sha256"]
CCACHE_URL = f"https://github.com/ccache/ccache/releases/download/v{CCACHE_VERSION}"
MOLD_URL = f"https://github.com/rui314/mold/releases/download/v{MOLD_VERSION}"
SCCACHE_URL = f"https://github.com/mozilla/sccache/releases/download/v{SCCACHE_VERSION}"


@pytest.mark.parametrize(
    ("digests", "platforms"),
    [
        (
            CCACHE_SHA256,
            {
                "linux-x86_64-glibc",
                "linux-aarch64-glibc",
                "windows-x86_64",
                "windows-aarch64",
                "darwin",
            },
        ),
        (MOLD_SHA256, {"x86_64", "aarch64"}),
        (SCCACHE_SHA256, {"x86_64", "aarch64"}),
    ],
)
def test_pinned_digests_cover_every_runner_platform(digests, platforms):
    # A malformed or missing digest would otherwise surface as a checksum mismatch mid-install.
    assert set(digests) == platforms
    for platform, digest in digests.items():
        assert re.fullmatch(r"[0-9a-f]{64}", digest), platform


def test_pinned_release_reads_the_build_config_tools_section(tmp_path, monkeypatch):
    config = tmp_path / "build-config.json"
    pin = {"repository": "rui314/mold", "version": "9.8.7", "sha256": {"x86_64": "a" * 64}}
    config.write_text(json.dumps({"tools": {"mold": pin}}), encoding="utf-8")
    monkeypatch.setenv("CONFIG_FILE", str(config))
    assert pinned_release("mold") == (
        "9.8.7",
        {"x86_64": "a" * 64},
        "https://github.com/rui314/mold/releases/download/v9.8.7",
    )


class TestCLI:
    """Tests for subcommand dispatch."""

    def test_no_subcommand_returns_error(self):
        assert main([]) == 1


class TestInstall:
    """Tests for the per-OS compiler-tool install dispatch."""

    @pytest.fixture
    def runner(self, monkeypatch, gh_path, gh_env):
        monkeypatch.setenv("RUNNER_ARCH", "ARM64")
        installed = []

        def fake_install(url, sha256, member, dest, *, sudo=False):
            installed.append((url, sha256, member, dest, sudo))
            return dest

        monkeypatch.setattr("ci.cache.ccache_helper._install_pinned", fake_install)
        return installed

    @staticmethod
    def _platform(monkeypatch, name):
        for check in ("linux", "macos", "windows"):
            monkeypatch.setattr(
                f"ci.cache.ccache_helper.is_{check}", lambda check=check: check == name
            )

    @pytest.fixture
    def linux(self, runner, monkeypatch, tmp_path):
        self._platform(monkeypatch, "linux")
        commands = []
        monkeypatch.setattr(
            "ci.cache.ccache_helper._run", lambda command: commands.append(command) or ""
        )
        return commands

    def test_linux_installs_pinned_ccache_and_mold_for_the_runner_arch(
        self, runner, linux, monkeypatch, gh_path, gh_env
    ):
        monkeypatch.setattr("ci.cache.ccache_helper._on_path", lambda tool, version: False)
        assert main(["install"]) == 0
        ccache = f"ccache-{CCACHE_VERSION}-linux-aarch64-glibc"
        mold = f"mold-{MOLD_VERSION}-aarch64-linux"
        assert runner == [
            (
                f"{CCACHE_URL}/{ccache}.tar.xz",
                CCACHE_SHA256["linux-aarch64-glibc"],
                f"{ccache}/ccache",
                Path("/usr/local/bin/ccache"),
                True,
            ),
            (
                f"{MOLD_URL}/{mold}.tar.gz",
                MOLD_SHA256["aarch64"],
                f"{mold}/bin/mold",
                Path("/usr/local/bin/mold"),
                True,
            ),
        ]
        assert linux == [["sudo", "ln", "-sf", "mold", str(Path("/usr/local/bin/ld.mold"))]]
        assert gh_path.read_text() == gh_env.read_text() == ""

    @pytest.mark.parametrize(("current", "installed"), [("ccache", "mold"), ("mold", "ccache")])
    def test_linux_skips_a_tool_whose_pinned_version_is_on_path(
        self, runner, linux, monkeypatch, current, installed
    ):
        versions = []

        def on_path(tool, version):
            versions.append((tool, version))
            return tool == current

        monkeypatch.setattr("ci.cache.ccache_helper._on_path", on_path)
        assert main(["install"]) == 0
        assert versions == [("ccache", CCACHE_VERSION), ("mold", MOLD_VERSION)]
        assert [dest.name for _, _, _, dest, _ in runner] == [installed]
        assert bool(linux) == (installed == "mold")

    @pytest.mark.parametrize(
        ("argv", "environment"),
        [(["install", "--compiler-cache", "sccache"], ""), (["install"], "sccache")],
    )
    def test_linux_sccache_pilot_installs_and_selects_pinned_sccache(
        self, runner, linux, monkeypatch, gh_env, argv, environment
    ):
        monkeypatch.setenv("QGC_COMPILER_CACHE_TOOL", environment)
        monkeypatch.setattr("ci.cache.ccache_helper._on_path", lambda tool, version: False)
        assert main(argv) == 0
        stem = f"sccache-v{SCCACHE_VERSION}-aarch64-unknown-linux-musl"
        # ccache stays installed: build jobs report compiler-cache statistics only when it is on PATH.
        assert [dest.name for _, _, _, dest, _ in runner] == ["ccache", "mold", "sccache"]
        assert runner[-1] == (
            f"{SCCACHE_URL}/{stem}.tar.gz",
            SCCACHE_SHA256["aarch64"],
            f"{stem}/sccache",
            Path("/usr/local/bin/sccache"),
            True,
        )
        expected = f"QGC_CACHE_PROGRAM={Path('/usr/local/bin/sccache')}\n"
        assert gh_env.read_text() == expected

    def test_linux_selects_a_pinned_sccache_already_on_path(
        self, runner, linux, monkeypatch, gh_env
    ):
        monkeypatch.setattr("ci.cache.ccache_helper._on_path", lambda tool, version: True)
        monkeypatch.setattr("ci.cache.ccache_helper.shutil.which", lambda tool: f"/opt/{tool}")
        assert main(["install", "--compiler-cache", "sccache"]) == 0
        assert runner == []
        assert gh_env.read_text() == f"QGC_CACHE_PROGRAM={Path('/opt/sccache')}\n"

    @pytest.mark.parametrize("platform", ["macos", "windows"])
    def test_sccache_pilot_is_linux_only(self, runner, monkeypatch, tmp_path, platform):
        self._platform(monkeypatch, platform)
        with pytest.raises(RuntimeError, match="sccache pilot"):
            main(["install", "--compiler-cache", "sccache", "--runner-temp", str(tmp_path)])
        assert runner == []

    def test_linux_ccache_failure_skips_mold(self, linux, monkeypatch):
        attempts = []

        def broken(url, sha256, member, dest, *, sudo=False):
            attempts.append(dest.name)
            raise RuntimeError("download failed")

        monkeypatch.setattr("ci.cache.ccache_helper._install_pinned", broken)
        monkeypatch.setattr("ci.cache.ccache_helper._on_path", lambda tool, version: False)
        with pytest.raises(RuntimeError, match="download failed"):
            main(["install"])
        assert attempts == ["ccache"]
        assert linux == []

    @pytest.mark.parametrize(
        ("probed", "pinned", "on_path"),
        [((2, 41), "2.41.0", True), ((4, 13, 6), "4.13.6", True), ((4, 12, 1), "4.13.6", False)],
    )
    def test_on_path_compares_the_pinned_version(self, probed, pinned, on_path, capsys):
        with patch("ci.cache.ccache_helper.probe_version", return_value=probed):
            assert ccache_helper._on_path("tool", pinned) is on_path
        assert ("already installed" in capsys.readouterr().out) is on_path

    def test_missing_tool_is_not_on_path(self):
        with patch("ci.cache.ccache_helper.probe_version", return_value=None):
            assert ccache_helper._on_path("mold", MOLD_VERSION) is False

    def test_macos_installs_under_runner_temp_and_selects_the_binary(
        self, runner, monkeypatch, tmp_path, gh_path, gh_env
    ):
        self._platform(monkeypatch, "macos")
        assert main(["install", "--runner-temp", str(tmp_path)]) == 0
        binary = tmp_path / "ccache" / "bin" / "ccache"
        stem = f"ccache-{CCACHE_VERSION}-darwin"
        assert runner == [
            (
                f"{CCACHE_URL}/{stem}.tar.gz",
                CCACHE_SHA256["darwin"],
                f"{stem}/ccache",
                binary,
                False,
            )
        ]
        assert gh_path.read_text() == f"{binary.parent}\n"
        assert gh_env.read_text() == f"QGC_CACHE_PROGRAM={binary}\n"

    def test_windows_installs_the_runner_arch_binary_on_path(
        self, runner, monkeypatch, tmp_path, gh_path, gh_env
    ):
        self._platform(monkeypatch, "windows")
        monkeypatch.setenv("RUNNER_TEMP", str(tmp_path))
        assert main(["install"]) == 0
        stem = f"ccache-{CCACHE_VERSION}-windows-aarch64"
        install_dir = tmp_path / stem
        assert runner == [
            (
                f"{CCACHE_URL}/{stem}.zip",
                CCACHE_SHA256["windows-aarch64"],
                f"{stem}/ccache.exe",
                install_dir / "ccache.exe",
                False,
            )
        ]
        assert gh_path.read_text() == f"{install_dir}\n"
        assert gh_env.read_text() == ""

    @pytest.mark.parametrize("platform", ["macos", "windows"])
    def test_unusable_binary_fails_the_install(
        self, runner, monkeypatch, tmp_path, gh_path, gh_env, platform
    ):
        self._platform(monkeypatch, platform)

        def broken(*args, **kwargs):
            raise RuntimeError("ccache --version failed: broken")

        monkeypatch.setattr("ci.cache.ccache_helper._install_pinned", broken)
        with pytest.raises(RuntimeError, match="broken"):
            main(["install", "--runner-temp", str(tmp_path)])
        assert gh_path.read_text() == gh_env.read_text() == ""

    @pytest.mark.parametrize("platform", ["macos", "windows"])
    def test_runner_temp_is_required_off_linux(self, runner, monkeypatch, platform):
        self._platform(monkeypatch, platform)
        with pytest.raises(RuntimeError, match="RUNNER_TEMP"):
            main(["install"])
        assert runner == []


class TestInstallPinned:
    """The shared installer verifies, extracts, installs and runs one release binary."""

    @pytest.fixture
    def release(self, tmp_path, monkeypatch):
        assets = tmp_path / "assets"
        assets.mkdir()
        downloads = []
        commands = []

        def download(url, dest, *, expected_sha256):
            downloads.append((url, expected_sha256))
            dest.write_bytes((assets / url.rsplit("/", 1)[-1]).read_bytes())

        def run(command):
            commands.append(command)
            return subprocess.CompletedProcess(command, 0, "tool version 1.2.3\n", "")

        monkeypatch.setattr("common.net.download_file", download)
        monkeypatch.setattr("ci.cache.ccache_helper.run_captured", run)
        return assets, downloads, commands

    @staticmethod
    def _tar(path: Path, member: str) -> None:
        source = path.parent / "payload"
        source.write_text("binary")
        with tarfile.open(path, "w:xz") as tar:
            tar.add(source, arcname=member)

    def test_zip_member_is_copied_executable_and_checked(self, release, tmp_path, capsys):
        assets, downloads, commands = release
        with zipfile.ZipFile(assets / "tool-1.2.3.zip", "w") as archive:
            archive.writestr("tool-1.2.3/tool.exe", "binary")
        dest = tmp_path / "install" / "tool.exe"

        result = ccache_helper._install_pinned(
            "https://example.test/v1.2.3/tool-1.2.3.zip", "a" * 64, "tool-1.2.3/tool.exe", dest
        )

        assert result == dest
        assert dest.read_text() == "binary"
        if sys.platform != "win32":
            assert stat.S_IMODE(dest.stat().st_mode) == 0o755
        assert downloads == [("https://example.test/v1.2.3/tool-1.2.3.zip", "a" * 64)]
        assert commands == [[str(dest), "--version"]]
        assert "tool version 1.2.3" in capsys.readouterr().out

    def test_sudo_installs_tar_member_into_the_system_prefix(self, release):
        assets, _, commands = release
        self._tar(assets / "tool-linux.tar.xz", "tool-linux/bin/tool")
        dest = Path("/usr/local/bin/tool")

        ccache_helper._install_pinned(
            "https://example.test/tool-linux.tar.xz",
            "b" * 64,
            "tool-linux/bin/tool",
            dest,
            sudo=True,
        )

        install, version = commands
        assert install[:5] == ["sudo", "install", "-D", "-m", "0755"]
        assert Path(install[5]).as_posix().endswith("tool-linux/bin/tool")
        assert install[6] == str(dest)
        assert version == [str(dest), "--version"]

    def test_sccache_pin_installs_from_the_release_archive_layout(self, release, monkeypatch):
        assets, downloads, commands = release
        stem = f"sccache-v{SCCACHE_VERSION}-x86_64-unknown-linux-musl"
        source = assets / "payload"
        source.write_text("binary")
        with tarfile.open(assets / f"{stem}.tar.gz", "w:gz") as tar:
            tar.add(source, arcname=f"{stem}/sccache")
            tar.add(source, arcname=f"{stem}/README.md")
        monkeypatch.setenv("RUNNER_ARCH", "X64")
        monkeypatch.setattr("ci.cache.ccache_helper.is_linux", lambda: True)
        monkeypatch.setattr(
            "ci.cache.ccache_helper._on_path", lambda tool, version: tool != "sccache"
        )

        ccache_helper.install_compiler_tools(None, "sccache")

        assert downloads == [(f"{SCCACHE_URL}/{stem}.tar.gz", SCCACHE_SHA256["x86_64"])]
        install, version = commands
        assert Path(install[5]).as_posix().endswith(f"{stem}/sccache")
        assert install[6] == version[0] == str(Path("/usr/local/bin/sccache"))

    def test_missing_member_fails(self, release, tmp_path):
        assets, _, commands = release
        self._tar(assets / "tool.tar.xz", "other/tool")
        with pytest.raises(RuntimeError, match="No regular file tool/bin/tool"):
            ccache_helper._install_pinned(
                "https://example.test/tool.tar.xz", "c" * 64, "tool/bin/tool", tmp_path / "tool"
            )
        assert commands == []

    def test_symlinked_member_is_rejected(self, release, tmp_path):
        assets, _, commands = release
        with tarfile.open(assets / "tool.tar.gz", "w:gz") as tar:
            link = tarfile.TarInfo("tool/tool")
            link.type = tarfile.SYMTYPE
            link.linkname = "other"
            tar.addfile(link)
        with pytest.raises(RuntimeError, match="No regular file tool/tool"):
            ccache_helper._install_pinned(
                "https://example.test/tool.tar.gz", "e" * 64, "tool/tool", tmp_path / "tool"
            )
        assert commands == []

    def test_failing_install_command_reports_stderr(self, release, tmp_path, monkeypatch):
        assets, _, _ = release
        self._tar(assets / "tool.tar.xz", "tool/tool")
        monkeypatch.setattr(
            "ci.cache.ccache_helper.run_captured",
            lambda command: subprocess.CompletedProcess(command, 1, "", "permission denied"),
        )
        with pytest.raises(RuntimeError, match="permission denied"):
            ccache_helper._install_pinned(
                "https://example.test/tool.tar.xz", "d" * 64, "tool/tool", tmp_path / "t", sudo=True
            )


_DOCKER_SHOW_STATS = """Cacheable calls:   1932 / 1932 (100.0%)
  Hits:            1926 / 1932 (99.69%)
    Direct:        1926 / 1926 (100.0%)
    Preprocessed:     0 / 1926 ( 0.00%)
  Misses:             6 / 1932 ( 0.31%)
Local storage:
  Cache size (GB):  4.9 /  5.0 (98.80%)
  Hits:            1926 / 1932 (99.69%)
  Misses:             6 / 1932 ( 0.31%)
"""


class TestSaveGate:
    """Pull requests skip compiler/moc cache saves that would add few entries."""

    @pytest.mark.parametrize(
        ("event", "misses", "minimum", "save"),
        [
            ("pull_request", 5, 20, False),
            ("pull_request", 0, 20, False),
            ("pull_request", 19, 20, False),
            ("pull_request", 20, 20, True),
            ("pull_request", 1500, 20, True),
            ("pull_request", None, 20, True),
            ("pull_request", 5, 0, True),
            ("pull_request", 5, -1, True),
            ("push", 0, 20, True),
            ("schedule", 0, 20, True),
            ("workflow_dispatch", 0, 20, True),
            ("merge_group", 0, 20, True),
            ("pull_request_target", 0, 20, True),
        ],
    )
    def test_should_save_cache(self, event, misses, minimum, save):
        assert should_save_cache(event, misses, minimum) is save

    def test_ccache_misses_from_json_stats(self):
        with patch("ci.cache.ccache_helper.ccache_stats", return_value={"cache_miss": 5}):
            assert ccache_misses() == 5
        with patch("ci.cache.ccache_helper.ccache_stats", return_value=None):
            assert ccache_misses() is None
        with patch("ci.cache.ccache_helper.ccache_stats", return_value={"direct_cache_hit": 1}):
            assert ccache_misses() is None

    @pytest.mark.parametrize(
        ("text", "misses"),
        [
            (_DOCKER_SHOW_STATS, 6),
            ("Summary:\n  Hits:  10 / 12\n  Misses:  2\n", 2),
            ("cache hit (direct)  10\ncache miss  3\n", 3),
            ("ccache: command not found", None),
            (None, None),
        ],
    )
    def test_ccache_misses_from_docker_report(self, tmp_path, text, misses):
        report = tmp_path / "docker-build-report.json"
        report.write_text(json.dumps({"ccache": text, "moccache": ""}))
        with patch("ci.cache.ccache_helper.ccache_stats") as host_stats:
            assert ccache_misses(report) == misses
        host_stats.assert_not_called()

    @pytest.mark.parametrize("content", [None, "not json", "[]"])
    def test_unreadable_docker_report_is_unknown(self, tmp_path, content):
        report = tmp_path / "docker-build-report.json"
        if content is not None:
            report.write_text(content)
        assert ccache_misses(report) is None

    @pytest.mark.parametrize(
        ("records", "misses"),
        [
            (["hit", "hit"], 0),
            (["hit", "miss-header-changed", "miss-first-seen", "miss", "cache-trimmed"], 3),
            (["miss-cache-evicted"], 1),
            (["cache-trimmed"], None),
            ([], None),
        ],
    )
    def test_moccache_misses_from_stats_log(self, tmp_path, records, misses):
        lines = [f"{kind}\t/src/a.h\t1700000000.000\t/build\t" for kind in records]
        (tmp_path / "stats.log").write_text("".join(f"{line}\n" for line in lines))
        assert moccache_misses(tmp_path) == misses

    def test_missing_moccache_stats_are_unknown(self, tmp_path):
        assert moccache_misses(tmp_path) is None

    @pytest.mark.parametrize(("value", "parsed"), [("20", 20), ("", 0), (" 5 ", 5), ("x", 0)])
    def test_parse_min_misses(self, value, parsed):
        assert parse_min_misses(value) == parsed


class TestSavePlan:
    """save-plan picks the snapshot and cache keys that save-build-cache uploads or saves."""

    @pytest.fixture
    def workspace(self, tmp_path, monkeypatch, gh_output, gh_env):
        workspace = tmp_path / "workspace"
        (workspace / ".cache" / "ccache").mkdir(parents=True)
        (workspace / ".cache" / "ccache" / "object").write_text("o")
        (workspace / ".cache" / "moccache").mkdir()
        (workspace / ".cache" / "moccache" / "stats.log").write_text(
            "miss-first-seen\tx.h\t1\t/b\t\n" * 25
        )
        runner_temp = tmp_path / "runner-temp"
        runner_temp.mkdir()
        env = {
            "GITHUB_WORKSPACE": str(workspace),
            "RUNNER_TEMP": str(runner_temp),
            "QGC_PR_CACHE_SAVE_MIN_MISSES": "20",
            "QGC_CCACHE_KEY": "ccache-key",
            "QGC_MOCCACHE_KEY": "moccache-key",
        }
        for name, value in env.items():
            monkeypatch.setenv(name, value)
        return workspace

    def _plan(self, *argv: str, ccache=None):
        with patch("ci.cache.ccache_helper.ccache_stats", return_value=ccache):
            assert main(["save-plan", *argv]) == 0

    @pytest.mark.parametrize(
        ("event", "ccache_key"),
        [("pull_request", ""), ("push", "ccache-key"), ("merge_group", "ccache-key")],
    )
    def test_keys_follow_each_cache_decision(
        self, workspace, monkeypatch, gh_output, gh_env, event, ccache_key
    ):
        monkeypatch.setenv("GITHUB_EVENT_NAME", event)
        self._plan(ccache={"cache_miss": 6})
        assert read_github_file(gh_output) == {
            "artifact_path": "",
            "ccache_key": ccache_key,
            "moccache_key": "moccache-key",
        }
        assert read_github_file(gh_env) == {"QGC_BUILD_CACHE_SAVED": "true"}

    def test_unavailable_statistics_save_both(self, workspace, monkeypatch, gh_output):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
        (workspace / ".cache" / "moccache" / "stats.log").unlink()
        self._plan()
        assert read_github_file(gh_output)["ccache_key"] == "ccache-key"
        assert read_github_file(gh_output)["moccache_key"] == "moccache-key"

    def test_failed_decision_still_writes_both_keys(
        self, workspace, monkeypatch, gh_output, capsys
    ):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
        with patch("ci.cache.ccache_helper.ccache_stats", side_effect=RuntimeError("broken")):
            assert main(["save-plan"]) == 0
        assert "::warning::Cache save decision failed, saving both caches: broken" in (
            capsys.readouterr().out
        )
        assert read_github_file(gh_output)["ccache_key"] == "ccache-key"
        assert read_github_file(gh_output)["moccache_key"] == "moccache-key"

    def test_docker_report_supplies_the_compiler_misses(
        self, workspace, monkeypatch, gh_output, tmp_path
    ):
        report = tmp_path / "docker-build-report.json"
        report.write_text(json.dumps({"ccache": _DOCKER_SHOW_STATS}))
        monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
        monkeypatch.setenv("INPUT_DOCKER_REPORT", str(report))
        with patch("ci.cache.ccache_helper.ccache_stats") as host_stats:
            assert main(["save-plan"]) == 0
        host_stats.assert_not_called()
        assert read_github_file(gh_output)["ccache_key"] == ""

    def test_sccache_pilot_has_no_compiler_key(self, workspace, monkeypatch, gh_output):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
        monkeypatch.setenv("QGC_CCACHE_KEY", "")
        self._plan()
        assert read_github_file(gh_output)["ccache_key"] == ""
        assert read_github_file(gh_output)["moccache_key"] == "moccache-key"

    def test_artifact_snapshot_packs_both_caches(self, workspace, monkeypatch, gh_output):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
        monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT", "true")
        self._plan()
        outputs = read_github_file(gh_output)
        # The keys stay set: the action saves them only when the upload fails.
        assert outputs["ccache_key"] == "ccache-key"
        archive = Path(outputs["artifact_path"])
        assert archive.parent.parent == Path(os.environ["RUNNER_TEMP"])
        with tarfile.open(archive) as tar:
            assert sorted(tar.getnames()) == [
                ".cache/ccache/object",
                ".cache/moccache/stats.log",
            ]

    def test_skipped_caches_pack_no_snapshot(self, workspace, monkeypatch, gh_output):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "pull_request")
        monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT", "true")
        (workspace / ".cache" / "moccache" / "stats.log").write_text("hit\tx.h\t1\t/b\t\n")
        with patch("ci.cache.ccache_helper.pack_snapshot") as pack:
            self._plan(ccache={"cache_miss": 1})
        pack.assert_not_called()
        assert read_github_file(gh_output) == {
            "artifact_path": "",
            "ccache_key": "",
            "moccache_key": "",
        }

    def test_failed_pack_falls_back_to_cache_keys(self, workspace, monkeypatch, gh_output, capsys):
        monkeypatch.setenv("GITHUB_EVENT_NAME", "push")
        monkeypatch.setenv("QGC_BUILD_CACHE_ARTIFACT", "true")
        with patch("ci.cache.build_cache_artifact.pack_cache", side_effect=OSError("disk full")):
            self._plan()
        assert "::warning::Compiler cache artifact pack unavailable: disk full" in (
            capsys.readouterr().out
        )
        assert read_github_file(gh_output) == {
            "artifact_path": "",
            "ccache_key": "ccache-key",
            "moccache_key": "moccache-key",
        }
