#!/usr/bin/env python3
"""Qt translation-tool discovery and diagnostic contracts."""

from __future__ import annotations

from pathlib import Path

import pytest

from ._helpers import load_script_module

mod = load_script_module("translations/qgc_lupdate.py", "qgc_lupdate")


def test_lupdate_diagnostics_report_both_actionable_categories() -> None:
    assert mod.collect_lupdate_errors("Updating qgc.ts...\n") == []
    errors = mod.collect_lupdate_errors(
        "src/a.cpp:42: tr() cannot be called without context\n"
        "src/B.h:10: Class 'B' lacks Q_OBJECT macro\n"
    )
    assert len(errors) == 2
    assert "QT_TRANSLATE_NOOP" in errors[0] and "src/a.cpp:42" in errors[0]
    assert "Q_OBJECT" in errors[1]


def test_lupdate_resolution_prefers_qt_root_then_path_or_errors(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    lupdate = tmp_path / "bin" / "lupdate"
    lupdate.parent.mkdir()
    lupdate.write_text("#!/bin/sh\n")
    lupdate.chmod(0o755)
    monkeypatch.setenv("QT_ROOT_DIR", str(tmp_path))
    assert mod.resolve_lupdate() == lupdate

    monkeypatch.delenv("QT_ROOT_DIR")
    monkeypatch.setattr(mod, "get_build_config_value", lambda _key: "")
    monkeypatch.setattr(mod.shutil, "which", lambda _name: "/usr/bin/lupdate")
    assert mod.resolve_lupdate() == Path("/usr/bin/lupdate")
    monkeypatch.setattr(mod.shutil, "which", lambda _name: None)
    with pytest.raises(FileNotFoundError):
        mod.resolve_lupdate()
