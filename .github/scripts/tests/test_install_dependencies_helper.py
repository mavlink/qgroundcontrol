"""Contracts for CI dependency post-install helpers."""

from __future__ import annotations

import sys
from unittest.mock import patch

import install_dependencies_helper as mod
from _helpers import completed


def test_detect_python_version_emits_interpreter_minor() -> None:
    captured: dict[str, str] = {}
    with patch.object(mod, "write_github_output", side_effect=captured.update):
        mod.detect_python_version()
    assert captured == {"minor": f"{sys.version_info.major}.{sys.version_info.minor}"}


def test_ldconfig_blas_detection() -> None:
    for output, expected in (
        ("libblas.so.3 (libc6,x86-64) => /usr/lib/libblas.so.3\n", True),
        ("libm.so.6 (libc6,x86-64) => /lib/libm.so.6\n", False),
    ):
        with patch.object(mod, "run_captured", return_value=completed(stdout=output)):
            assert mod._ldconfig_has_blas() is expected
