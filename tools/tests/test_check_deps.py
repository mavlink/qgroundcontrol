"""Tests for the standalone dependency checker."""

from __future__ import annotations

from unittest.mock import patch

from ._helpers import load_script_module

check_deps = load_script_module("check_deps.py", "qgc_check_deps")


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
        "fetch_gstreamer_package_index",
        side_effect=indexes,
    ) as fetch_index:
        latest = check_deps.fetch_latest_common_gstreamer_patch("1.28.4")

    assert latest == "1.28.6"
    assert fetch_index.call_count == 4


def test_parse_args_supports_gstreamer_only() -> None:
    args = check_deps.parse_args(["--gstreamer", "--fail-if-outdated"])

    assert args.gstreamer is True
    assert args.fail_if_outdated is True


def test_main_strict_gstreamer_check_passes_when_current() -> None:
    with (
        patch.object(check_deps, "check_gstreamer_version", return_value=True),
        patch.object(check_deps, "find_repo_root"),
    ):
        result = check_deps.main(["--gstreamer", "--fail-if-outdated"])

    assert result == 0


def test_main_strict_gstreamer_check_fails_when_stale_or_unavailable() -> None:
    for check_result in (False, None):
        with (
            patch.object(check_deps, "check_gstreamer_version", return_value=check_result),
            patch.object(check_deps, "find_repo_root"),
        ):
            result = check_deps.main(["--gstreamer", "--fail-if-outdated"])

        assert result == 1
