"""Tests for tools/translations/qgc_lupdate.py."""

from __future__ import annotations

import subprocess
from pathlib import Path
from unittest.mock import patch

import pytest

from translations import qgc_lupdate


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


def test_collect_lupdate_errors_detects_tr_called_on_object() -> None:
    output = "src/Foo.cc:87: Cannot invoke tr() like this\n"
    errors = qgc_lupdate.collect_lupdate_errors(output)
    assert len(errors) == 1
    assert "ClassName::tr" in errors[0]
    assert "src/Foo.cc:87" in errors[0]


def test_collect_lupdate_errors_reports_every_category() -> None:
    output = (
        "src/a.cpp: tr() cannot be called without context\n"
        "src/B.h: Class 'B' lacks Q_OBJECT macro\n"
        "src/c.cc: Cannot invoke tr() like this\n"
    )
    assert len(qgc_lupdate.collect_lupdate_errors(output)) == 3


def test_resolve_lupdate_prefers_qt_root_dir(tmp_path: Path) -> None:
    bin_dir = tmp_path / "bin"
    bin_dir.mkdir()
    lupdate = bin_dir / "lupdate"
    lupdate.write_text("#!/bin/sh\n")
    lupdate.chmod(0o755)
    with patch.dict("os.environ", {"QT_ROOT_DIR": str(tmp_path)}, clear=False):
        assert qgc_lupdate.resolve_lupdate() == lupdate


def test_resolve_lupdate_searches_configured_qt_version() -> None:
    found = Path("/qt/6.12.0/gcc_64/bin/lupdate")
    with (
        patch.object(qgc_lupdate, "get_build_config_value", return_value="6.12.0"),
        patch.object(qgc_lupdate, "find_qt_tool", return_value=found) as find,
    ):
        assert qgc_lupdate.resolve_lupdate() == found
    find.assert_called_once_with("lupdate", version="6.12.0")


def test_resolve_lupdate_falls_back_to_path() -> None:
    with (
        patch.object(qgc_lupdate, "get_build_config_value", return_value=""),
        patch.object(qgc_lupdate, "find_qt_tool", return_value=None) as find,
        patch.object(qgc_lupdate.shutil, "which", return_value="/usr/bin/lupdate"),
    ):
        assert qgc_lupdate.resolve_lupdate() == Path("/usr/bin/lupdate")
    find.assert_called_once_with("lupdate", version="*")


def test_resolve_lupdate_raises_when_not_found() -> None:
    with (
        patch.object(qgc_lupdate, "find_qt_tool", return_value=None),
        patch.object(qgc_lupdate.shutil, "which", return_value=None),
        pytest.raises(FileNotFoundError),
    ):
        qgc_lupdate.resolve_lupdate()


def test_main_runs_json_extractor_and_pseudo_loc_in_process(
    tmp_path: Path, monkeypatch: pytest.MonkeyPatch
) -> None:
    lupdate = tmp_path / "bin" / "lupdate"
    lupdate.parent.mkdir()
    lupdate.write_text("#!/bin/sh\n")
    lupdate.chmod(0o755)
    calls: list[str] = []
    monkeypatch.setenv("QT_ROOT_DIR", str(tmp_path))
    monkeypatch.setattr("sys.argv", ["qgc_lupdate.py"])
    monkeypatch.setattr(qgc_lupdate, "run_lupdate", lambda _: "")
    monkeypatch.setattr(qgc_lupdate.qgc_lupdate_json, "main", lambda: calls.append("json"))
    monkeypatch.setattr(qgc_lupdate.pseudo_loc, "main", lambda: calls.append("pseudo_loc"))
    assert qgc_lupdate.main() == 0
    assert calls == ["json", "pseudo_loc"]


@pytest.mark.parametrize(
    ("lupdate_output", "exit_code"),
    [
        ("Updating 'qgc.ts'...\n", 0),
        ("src/Foo.cc:87: Cannot invoke tr() like this\n", 1),
    ],
)
def test_main_check_extracts_into_scratch_catalog_only(
    monkeypatch: pytest.MonkeyPatch, lupdate_output: str, exit_code: int
) -> None:
    ts_files: list[Path] = []

    def fake_lupdate(
        command: list[str], *, cwd: Path, **_: object
    ) -> subprocess.CompletedProcess[str]:
        ts_file = cwd / command[command.index("-ts") + 1]
        assert ts_file.parent.is_dir()
        ts_files.append(ts_file)
        return subprocess.CompletedProcess(command, 0, stdout=lupdate_output, stderr="")

    def unexpected_generator() -> None:
        pytest.fail("--check must not regenerate translation files")

    monkeypatch.setattr("sys.argv", ["qgc_lupdate.py", "--check"])
    monkeypatch.setattr(qgc_lupdate, "resolve_lupdate", lambda: Path("lupdate"))
    monkeypatch.setattr(qgc_lupdate.subprocess, "run", fake_lupdate)
    monkeypatch.setattr(qgc_lupdate.qgc_lupdate_json, "main", unexpected_generator)
    monkeypatch.setattr(qgc_lupdate.pseudo_loc, "main", unexpected_generator)

    assert qgc_lupdate.main() == exit_code

    [ts_file] = ts_files
    assert ts_file.parent != qgc_lupdate.REPO_ROOT / "translations"
    assert not ts_file.parent.exists()
