"""Tests for tools/analyzers/qt_translate_noop_check.py."""

from __future__ import annotations

import json
from typing import TYPE_CHECKING

import pytest
from analyzers.qt_translate_noop_check import GUIDANCE, analyze_file, main

if TYPE_CHECKING:
    from pathlib import Path


def _violations(tmp_path: Path, body: str) -> list[tuple[int, str]]:
    source = tmp_path / "sample.cc"
    source.write_text(body, encoding="utf-8")
    return [(v.line, v.message) for v in analyze_file(source)]


@pytest.mark.parametrize(
    ("line", "message"),
    [
        ('    return QT_TRANSLATE_NOOP("Ctx", "Text");', "QT_TRANSLATE_NOOP result returned"),
        ('    return QT_TR_NOOP("Text");', "QT_TR_NOOP result returned"),
        ('    s = fmt.arg(QT_TRANSLATE_NOOP("Ctx", "Text"));', "QT_TRANSLATE_NOOP result passed"),
        ('    s = fmt.arg( QT_TR_NOOP("Text"));', "QT_TR_NOOP result passed"),
        ('    s = QString(QT_TRANSLATE_NOOP("Ctx", "Text"));', "QT_TRANSLATE_NOOP result wrapped"),
        ('    s = QString (QT_TR_NOOP("Text"));', "QT_TR_NOOP result wrapped"),
    ],
)
def test_reports_runtime_use_of_noop_macros(tmp_path: Path, line: str, message: str) -> None:
    ((number, reported),) = _violations(tmp_path, f"QString f() {{\n{line}\n}}\n")
    assert number == 2
    assert reported.startswith(message)


@pytest.mark.parametrize(
    "line",
    [
        'static const char* kText = QT_TRANSLATE_NOOP("Ctx", "Text");',
        'static const char* kTexts[] = { QT_TR_NOOP("A"), QT_TR_NOOP("B") };',
        'return QCoreApplication::translate("Ctx", "Text");',
        'return tr("Text");',
        '// return QT_TRANSLATE_NOOP("Ctx", "Text");',
        '    // s = QString(QT_TR_NOOP("Text"));',
    ],
)
def test_ignores_markers_translations_and_comments(tmp_path: Path, line: str) -> None:
    assert _violations(tmp_path, f"{line}\n") == []


def test_reports_one_violation_per_line(tmp_path: Path) -> None:
    body = 'return QString(QT_TR_NOOP("A")).arg(QT_TR_NOOP("B"));\n'
    assert len(_violations(tmp_path, body)) == 1


def test_cli_reports_findings_in_directories(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    (tmp_path / "sub").mkdir()
    bad = tmp_path / "sub" / "bad.cc"
    bad.write_text('QString f() { return QT_TR_NOOP("Text"); }\n', encoding="utf-8")
    (tmp_path / "good.h").write_text(
        'inline QString g() { return tr("Text"); }\n', encoding="utf-8"
    )

    assert main([str(tmp_path)]) == 1
    captured = capsys.readouterr()
    assert captured.out.startswith(f"{bad}:1: error: QT_TR_NOOP result returned")
    assert captured.out.endswith(GUIDANCE)
    assert "1 qt-translate-noop-check finding(s)" in captured.err


def test_cli_json_lists_findings(tmp_path: Path, capsys: pytest.CaptureFixture[str]) -> None:
    source = tmp_path / "bad.cc"
    source.write_text('QString f() { return QT_TR_NOOP("Text"); }\n', encoding="utf-8")

    assert main(["--json", str(source)]) == 1
    (finding,) = json.loads(capsys.readouterr().out)
    assert finding == {
        "file": str(source),
        "line": 1,
        "code": 'QString f() { return QT_TR_NOOP("Text"); }',
        "message": "QT_TR_NOOP result returned directly — string will not be translated",
        "suggestion": 'return tr("source")',
    }


def test_cli_passes_clean_files_and_requires_paths(
    tmp_path: Path, capsys: pytest.CaptureFixture[str]
) -> None:
    source = tmp_path / "good.cc"
    source.write_text('static const char* kText = QT_TR_NOOP("Text");\n', encoding="utf-8")

    assert main([str(source)]) == 0
    assert capsys.readouterr() == ("", "")
    # Without paths the hook used to wait for a file list on stdin.
    with pytest.raises(SystemExit) as exc:
        main([])
    assert exc.value.code == 2
