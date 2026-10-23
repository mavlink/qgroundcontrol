"""Tests for tools/common/tool_version.py."""

from __future__ import annotations

import re
from unittest.mock import patch

import pytest
from common.tool_version import probe_version, version_prefix_matches
from hypothesis import given
from hypothesis import strategies as st

from ._helpers import completed

pytestmark = pytest.mark.windows


def test_probe_version_three_components() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/usr/bin/clang-format"),
        patch(
            "common.tool_version.run_captured",
            return_value=completed("clang-format version 21.1.7\n"),
        ),
    ):
        assert probe_version("clang-format") == (21, 1, 7)


def test_probe_version_two_components() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/usr/bin/foo"),
        patch("common.tool_version.run_captured", return_value=completed("foo v4.13\n")),
    ):
        assert probe_version("foo") == (4, 13)


def test_probe_version_uses_stderr_when_stdout_empty() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/usr/bin/javac"),
        patch("common.tool_version.run_captured", return_value=completed(stderr="javac 17.0.10\n")),
    ):
        assert probe_version("javac") == (17, 0, 10)


def test_probe_version_missing_tool() -> None:
    with patch("common.tool_version.shutil.which", return_value=None):
        assert probe_version("nonexistent") is None


def test_probe_version_nonzero_exit() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/bin/false"),
        patch("common.tool_version.run_captured", return_value=completed(returncode=1)),
    ):
        assert probe_version("false") is None


def test_probe_version_unparseable() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/bin/x"),
        patch("common.tool_version.run_captured", return_value=completed("not a version")),
    ):
        assert probe_version("x") is None


def test_probe_version_custom_pattern() -> None:
    pattern = re.compile(r"v(\d+)-(\d+)")
    with (
        patch("common.tool_version.shutil.which", return_value="/bin/x"),
        patch("common.tool_version.run_captured", return_value=completed("release v2-7\n")),
    ):
        assert probe_version("x", pattern=pattern) == (2, 7)


def test_probe_version_custom_args() -> None:
    with (
        patch("common.tool_version.shutil.which", return_value="/bin/git"),
        patch(
            "common.tool_version.run_captured", return_value=completed("git version 2.42.0\n")
        ) as run,
    ):
        probe_version("git", args=("version",))
        run.assert_called_once()
        assert run.call_args[0][0] == ["git", "version"]


def test_version_prefix_matches_shared_components() -> None:
    for actual, expected, matches in (
        ((4, 13), "4.13.6", True),
        ((4, 13, 6), "4.13", True),
        ((4, 12, 6), "4.13.6", False),
        ((4, 13, 6), "latest", False),
    ):
        assert version_prefix_matches(actual, expected) is matches


def test_probe_timeout_is_unavailable() -> None:
    import subprocess

    with (
        patch("common.tool_version.shutil.which", return_value="tool"),
        patch("common.tool_version.run_captured", side_effect=subprocess.TimeoutExpired("tool", 1)),
    ):
        assert probe_version("tool") is None


def test_lock_version_parses_toml_order_and_rejects_ambiguity(tmp_path) -> None:
    from common.tool_version import uv_lock_version

    path = tmp_path / "uv.lock"
    path.write_text('[[package]]\nversion = "1.2.0rc1"\nname = "demo"\n')
    assert uv_lock_version("demo", lock_path=path) == "1.2.0rc1"
    with path.open("a") as stream:
        stream.write('[[package]]\nname = "demo"\nversion = "2.0"\n')
    with pytest.raises(ValueError, match="Multiple locked versions"):
        uv_lock_version("demo", lock_path=path)


_VERSION = st.lists(st.integers(min_value=0, max_value=10**6), min_size=1, max_size=4).map(tuple)
# Text around a version without digits or dots, which would extend or precede the version token.
_NOISE = st.text(alphabet=st.characters(exclude_categories=("Nd",), exclude_characters="."))


@given(installed=_VERSION, expected=_VERSION)
def test_version_prefix_matches_iff_shared_components_are_equal(
    installed: tuple[int, ...], expected: tuple[int, ...]
) -> None:
    shared = min(len(installed), len(expected))
    matches = version_prefix_matches(installed, ".".join(map(str, expected)))
    assert matches is (installed[:shared] == expected[:shared])
    assert matches is version_prefix_matches(expected, ".".join(map(str, installed)))


@given(
    version=_VERSION.filter(lambda version: len(version) >= 2),
    prefix=_NOISE,
    suffix=_NOISE,
    on_stderr=st.booleans(),
)
def test_probe_version_extracts_the_first_version_token(
    version: tuple[int, ...], prefix: str, suffix: str, on_stderr: bool
) -> None:
    text = f"{prefix}{'.'.join(map(str, version))}{suffix}"
    output = completed(stderr=text) if on_stderr else completed(text, stderr="other 9.9.9")
    with (
        patch("common.tool_version.shutil.which", return_value="/bin/tool"),
        patch("common.tool_version.run_captured", return_value=output),
    ):
        assert probe_version("tool") == version[:3]
