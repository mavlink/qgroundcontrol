#!/usr/bin/env python3
"""Tests for tools/translations/qgc_lupdate.py."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

import pytest

from ._helpers import load_script_module

qgc_lupdate = load_script_module("translations/qgc_lupdate.py", "qgc_lupdate")


def test_collect_lupdate_errors_clean_output() -> None:
    assert qgc_lupdate.collect_lupdate_errors("Updating qgc.ts...\n") == []


def test_collect_lupdate_errors_detects_missing_context() -> None:
    output = "src/foo.cpp:42: tr() cannot be called without context\n"
    errors = qgc_lupdate.collect_lupdate_errors(output)
    assert len(errors) == 1
    assert "QT_TRANSLATE_NOOP" in errors[0]
    assert "src/foo.cpp:42" in errors[0]


def test_collect_lupdate_errors_detects_missing_qobject() -> None:
    output = "src/Foo.h:10: Class 'Foo' lacks Q_OBJECT macro\n"
    errors = qgc_lupdate.collect_lupdate_errors(output)
    assert len(errors) == 1
    assert "Q_OBJECT" in errors[0]


def test_collect_lupdate_errors_reports_both_categories() -> None:
    output = (
        "src/a.cpp: tr() cannot be called without context\n"
        "src/B.h: Class 'B' lacks Q_OBJECT macro\n"
    )
    assert len(qgc_lupdate.collect_lupdate_errors(output)) == 2


def test_resolve_lupdate_prefers_qt_root_dir(tmp_path: Path) -> None:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    lupdate = bin_dir / "lupdate"
    lupdate.write_text("#!/bin/sh\n")
    lupdate.chmod(0o755)
    with patch.dict("os.environ", {"QT_ROOT_DIR": str(tmp_path)}, clear=False):
        assert qgc_lupdate.resolve_lupdate() == lupdate


def test_resolve_lupdate_falls_back_to_path(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("QT_ROOT_DIR", raising=False)
    with (
        patch.object(qgc_lupdate, "get_build_config_value", return_value=""),
        patch.object(qgc_lupdate.shutil, "which", return_value="/usr/bin/lupdate"),
    ):
        assert qgc_lupdate.resolve_lupdate() == Path("/usr/bin/lupdate")


def test_resolve_lupdate_raises_when_not_found(monkeypatch: pytest.MonkeyPatch) -> None:
    monkeypatch.delenv("QT_ROOT_DIR", raising=False)
    with (
        patch.object(qgc_lupdate, "get_build_config_value", return_value=""),
        patch.object(qgc_lupdate.shutil, "which", return_value=None),
        pytest.raises(FileNotFoundError),
    ):
        qgc_lupdate.resolve_lupdate()
