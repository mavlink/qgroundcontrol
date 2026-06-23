#!/usr/bin/env python3
"""Tests for tools/common/tool_version.py."""

from __future__ import annotations

import re
import subprocess
from unittest.mock import patch

from common.tool_version import probe_version


def _fake_completed(stdout: str = "", stderr: str = "", returncode: int = 0) -> subprocess.CompletedProcess[str]:
    return subprocess.CompletedProcess(args=[], returncode=returncode, stdout=stdout, stderr=stderr)


def test_probe_version_three_components() -> None:
    with patch("common.tool_version.shutil.which", return_value="/usr/bin/clang-format"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("clang-format version 21.1.7\n")):
        assert probe_version("clang-format") == (21, 1, 7)


def test_probe_version_two_components() -> None:
    with patch("common.tool_version.shutil.which", return_value="/usr/bin/foo"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("foo v4.13\n")):
        assert probe_version("foo") == (4, 13)


def test_probe_version_uses_stderr_when_stdout_empty() -> None:
    with patch("common.tool_version.shutil.which", return_value="/usr/bin/javac"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("", "javac 17.0.10\n")):
        assert probe_version("javac") == (17, 0, 10)


def test_probe_version_missing_tool() -> None:
    with patch("common.tool_version.shutil.which", return_value=None):
        assert probe_version("nonexistent") is None


def test_probe_version_nonzero_exit() -> None:
    with patch("common.tool_version.shutil.which", return_value="/bin/false"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("", "", 1)):
        assert probe_version("false") is None


def test_probe_version_unparseable() -> None:
    with patch("common.tool_version.shutil.which", return_value="/bin/x"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("not a version")):
        assert probe_version("x") is None


def test_probe_version_custom_pattern() -> None:
    pattern = re.compile(r"v(\d+)-(\d+)")
    with patch("common.tool_version.shutil.which", return_value="/bin/x"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("release v2-7\n")):
        assert probe_version("x", pattern=pattern) == (2, 7)


def test_probe_version_custom_args() -> None:
    with patch("common.tool_version.shutil.which", return_value="/bin/git"), \
         patch("common.tool_version.run_captured", return_value=_fake_completed("git version 2.42.0\n")) as run:
        probe_version("git", args=("version",))
        run.assert_called_once()
        assert run.call_args[0][0] == ["git", "version"]
