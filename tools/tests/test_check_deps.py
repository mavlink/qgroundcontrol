"""Tests for the standalone dependency checker."""

from __future__ import annotations

import subprocess
from http.client import IncompleteRead
from typing import TYPE_CHECKING
from unittest.mock import patch
from urllib.error import URLError

import check_deps
import pytest

if TYPE_CHECKING:
    from pathlib import Path


def test_parse_gstreamer_package_versions() -> None:
    index_html = """
    <a href="1.28.4/">1.28.4/</a>
    <a href='1.28.6/'>1.28.6/</a>
    <a href="1.29.2/">1.29.2/</a>
    """

    assert check_deps.parse_gstreamer_package_versions(index_html) == {
        (1, 28, 4),
        (1, 28, 6),
        (1, 29, 2),
    }


def test_latest_common_gstreamer_patch_uses_platform_intersection() -> None:
    platform_versions = {
        "android": {(1, 28, 4), (1, 28, 5), (1, 28, 6)},
        "ios": {(1, 28, 4), (1, 28, 5), (1, 28, 6)},
        "macos": {(1, 28, 4), (1, 28, 5)},
        "windows": {(1, 28, 4), (1, 28, 5), (1, 29, 1)},
    }

    assert check_deps.latest_common_gstreamer_patch("1.28.4", platform_versions) == "1.28.5"


def test_latest_common_gstreamer_patch_stays_in_configured_minor() -> None:
    platform_versions = {
        platform: {(1, 28, 6), (1, 29, 2)} for platform in ("android", "ios", "macos", "windows")
    }

    assert check_deps.latest_common_gstreamer_patch("1.28.4", platform_versions) == "1.28.6"


def test_latest_common_gstreamer_patch_requires_every_platform() -> None:
    platform_versions = {
        "android": {(1, 28, 6)},
        "ios": {(1, 28, 6)},
        "macos": set(),
        "windows": {(1, 28, 6)},
    }

    assert check_deps.latest_common_gstreamer_patch("1.28.4", platform_versions) is None

    platform_versions.pop("macos")
    assert check_deps.latest_common_gstreamer_patch("1.28.4", platform_versions) is None


def test_fetch_latest_common_gstreamer_patch_uses_all_platform_indexes() -> None:
    indexes = [
        '<a href="1.28.4/">1.28.4</a><a href="1.28.6/">1.28.6</a>',
        '<a href="1.28.4/">1.28.4</a><a href="1.28.6/">1.28.6</a>',
        '<a href="1.28.4/">1.28.4</a><a href="1.28.6/">1.28.6</a>',
        '<a href="1.28.4/">1.28.4</a><a href="1.28.6/">1.28.6</a>',
    ]
    with patch.object(
        check_deps,
        "fetch_text",
        side_effect=indexes,
    ) as fetch_index:
        latest = check_deps.fetch_latest_common_gstreamer_patch("1.28.4")

    assert latest == "1.28.6"
    assert fetch_index.call_count == 4


@pytest.mark.parametrize(
    ("latest", "message"),
    [
        ("1.28.10", "Newer GStreamer patch available"),
        ("1.28.9", "Using latest GStreamer patch"),
        ("1.28.8", "Configured GStreamer is newer"),
    ],
)
def test_gstreamer_check_compares_versions_numerically(latest: str, message: str, capsys) -> None:
    with (
        patch.object(check_deps, "get_build_config_value", return_value="1.28.9"),
        patch.object(check_deps, "fetch_latest_common_gstreamer_patch", return_value=latest),
        patch.object(check_deps, "probe_version", return_value=None),
    ):
        check_deps.check_gstreamer_version()
    assert message in capsys.readouterr().out


def test_parse_args_supports_gstreamer_only() -> None:
    assert check_deps.parse_args(["--gstreamer"]).gstreamer is True
    with pytest.raises(SystemExit):
        check_deps.parse_args(["--gstreamer", "--fail-if-outdated"])


def test_main_gstreamer_check_reports_without_failing() -> None:
    with (
        patch.object(check_deps, "check_gstreamer_version") as gstreamer,
        patch.object(check_deps, "check_qt_version") as qt,
        patch.object(check_deps, "find_repo_root"),
    ):
        assert check_deps.main(["--gstreamer"]) == 0

    gstreamer.assert_called_once()
    qt.assert_not_called()


def test_main_runs_each_selected_check() -> None:
    with (
        patch.object(check_deps, "check_qt_version") as qt,
        patch.object(check_deps, "check_gstreamer_version") as gstreamer,
        patch.object(check_deps, "check_python_deps") as python_deps,
        patch.object(check_deps, "find_repo_root"),
    ):
        assert check_deps.main(["--qt", "--gstreamer"]) == 0

    qt.assert_called_once()
    gstreamer.assert_called_once()
    python_deps.assert_not_called()


def test_main_runs_every_check_by_default() -> None:
    with (
        patch.object(check_deps, "check_qt_version") as qt,
        patch.object(check_deps, "check_gstreamer_version") as gstreamer,
        patch.object(check_deps, "check_python_deps") as python_deps,
        patch.object(check_deps, "check_build_tools") as build_tools,
        patch.object(check_deps, "find_repo_root"),
    ):
        assert check_deps.main([]) == 0

    for check in (qt, gstreamer, python_deps, build_tools):
        check.assert_called_once()


@pytest.mark.parametrize(
    "error",
    [
        URLError("offline"),
        TimeoutError(),
        IncompleteRead(b""),
        UnicodeDecodeError("utf-8", b"", 0, 1, "x"),
    ],
)
def test_fetch_text_returns_none_on_errors(error: Exception) -> None:
    with patch.object(check_deps, "read_url_text", side_effect=error):
        assert check_deps.fetch_text("https://example.invalid/") is None


def test_check_qt_version_tolerates_missing_qt_tools(capsys: pytest.CaptureFixture[str]) -> None:
    with (
        patch.object(check_deps, "fetch_latest_qt_minor", return_value=None),
        patch("common.tool_version.shutil.which", return_value=None),
    ):
        check_deps.check_qt_version()

    assert "Installed:" not in capsys.readouterr().out


def test_check_qt_version_reports_qt_not_qmake_version(capsys: pytest.CaptureFixture[str]) -> None:
    qmake_output = "QMake version 3.1\nUsing Qt version 6.12.0 in /opt/qt/lib\n"
    with (
        patch.object(check_deps, "fetch_latest_qt_minor", return_value=None),
        patch("common.tool_version.shutil.which", return_value="/opt/qt/bin/qmake"),
        patch(
            "common.tool_version.run_captured",
            return_value=subprocess.CompletedProcess([], 0, stdout=qmake_output, stderr=""),
        ),
    ):
        check_deps.check_qt_version()

    assert "Installed: Qt 6.12.0" in capsys.readouterr().out


def test_check_build_tools_reports_missing_tools(capsys: pytest.CaptureFixture[str]) -> None:
    with patch("common.tool_version.shutil.which", return_value=None):
        check_deps.check_build_tools()

    out = capsys.readouterr().out
    for tool in check_deps.BUILD_TOOLS:
        assert f"{tool}: not installed" in out


def test_check_python_deps_tolerates_missing_uv(tmp_path: Path) -> None:
    (tmp_path / "tools").mkdir()
    (tmp_path / "tools" / "pyproject.toml").touch()
    with patch.object(check_deps, "run_captured", side_effect=FileNotFoundError("uv")) as run:
        check_deps.check_python_deps(tmp_path)
    run.assert_called_once()


def test_check_python_deps_warns_without_manifest(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    with patch.object(check_deps, "run_captured") as run:
        check_deps.check_python_deps(tmp_path)
    run.assert_not_called()
    assert "manifest not found: tools/pyproject.toml" in capsys.readouterr().out
