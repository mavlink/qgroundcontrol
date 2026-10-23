"""Tests for tools/analyzers/qmlformat.py."""

from __future__ import annotations

from subprocess import CompletedProcess
from typing import TYPE_CHECKING

import pytest
from analyze import get_analyzer
from analyzers import qmlformat
from analyzers.qmlformat import QmlFormatAnalyzer

if TYPE_CHECKING:
    from pathlib import Path

FORMATTED = "Item {\n    id: root\n}\n"


@pytest.fixture
def fake_qmlformat(monkeypatch: pytest.MonkeyPatch) -> list[list[str]]:
    """Make qmlformat print FORMATTED, or fail for files named broken.qml."""
    commands: list[list[str]] = []

    def run(command: list[str]) -> CompletedProcess[str]:
        commands.append(command)
        if command[-1].endswith("broken.qml"):
            return CompletedProcess(command, 1, "", "Error: Expected token `}'\n")
        return CompletedProcess(command, 0, FORMATTED, "")

    monkeypatch.setattr("common.proc.shutil.which", lambda _: "/qt/bin/qmlformat")
    monkeypatch.setattr(qmlformat, "run_captured", run)
    return commands


def test_missing_qmlformat_is_execution_failure(tmp_path: Path, monkeypatch: pytest.MonkeyPatch):
    monkeypatch.setattr("common.proc.shutil.which", lambda _: None)
    result = QmlFormatAnalyzer(tmp_path, tmp_path).run([tmp_path / "file.qml"])
    assert result.execution_error


def test_formatted_files_pass(tmp_path: Path, fake_qmlformat: list[list[str]]):
    file = tmp_path / "Formatted.qml"
    file.write_text(FORMATTED, encoding="utf-8")

    result = QmlFormatAnalyzer(tmp_path, tmp_path).run([file])

    assert result.passed
    assert fake_qmlformat == [["qmlformat", str(file)]]


def test_differences_are_reported_without_rewriting(
    tmp_path: Path, fake_qmlformat: list[list[str]]
):
    file = tmp_path / "Unformatted.qml"
    file.write_text("Item { id: root }\n", encoding="utf-8")

    result = QmlFormatAnalyzer(tmp_path, tmp_path).run([file], fix=True)

    assert not result.passed
    assert not result.error_findings
    assert result.files_with_issues == ["Unformatted.qml"]
    assert "-Item { id: root }\n+Item {\n" in result.output
    assert file.read_text(encoding="utf-8") == "Item { id: root }\n"


def test_crlf_line_endings_are_reported(tmp_path: Path, fake_qmlformat: list[list[str]]):
    file = tmp_path / "Windows.qml"
    file.write_bytes(FORMATTED.replace("\n", "\r\n").encode())

    assert QmlFormatAnalyzer(tmp_path, tmp_path).run([file]).files_with_issues == ["Windows.qml"]


def test_qmlformat_errors_fail(tmp_path: Path, fake_qmlformat: list[list[str]]):
    file = tmp_path / "broken.qml"
    file.write_text("Item {\n", encoding="utf-8")

    result = QmlFormatAnalyzer(tmp_path, tmp_path).run([file])

    assert result.error_findings
    assert result.files_with_issues == ["broken.qml"]


def test_hook_entry_prints_diff_and_fails(
    tmp_path: Path,
    fake_qmlformat: list[list[str]],
    monkeypatch: pytest.MonkeyPatch,
    capsys: pytest.CaptureFixture[str],
):
    monkeypatch.chdir(tmp_path)
    (tmp_path / "Formatted.qml").write_text(FORMATTED, encoding="utf-8")
    (tmp_path / "Unformatted.qml").write_text("Item { id: root }\n", encoding="utf-8")

    assert qmlformat.main(["Formatted.qml"]) == 0
    assert qmlformat.main(["Unformatted.qml"]) == 1
    assert "+++ Unformatted.qml (qmlformat)" in capsys.readouterr().out


def test_analyze_selects_qmlformat(tmp_path: Path):
    assert isinstance(get_analyzer("qmlformat", tmp_path, tmp_path), QmlFormatAnalyzer)
