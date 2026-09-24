"""Tests for the Simplified Chinese Qt catalog audit."""

from __future__ import annotations

from pathlib import Path
from unittest.mock import patch

import pytest

from ._helpers import load_script_module

audit = load_script_module("translations/check_zh_cn.py", "check_zh_cn")


def make_catalog(path: Path, body: str) -> Path:
    path.write_text(f'<TS><context><name>Example</name>{body}</context></TS>', encoding="utf-8")
    return path


def test_counts_active_messages_and_unchanged_copy(tmp_path: Path) -> None:
    path = make_catalog(
        tmp_path / "example.ts",
        "<message><source>Fly</source><translation>飞行</translation></message>"
        '<message><source>Land</source><translation type="unfinished">Land</translation></message>'
        '<message><source>MAVLink</source><translation>MAVLink</translation></message>'
        '<message><source>Old</source><translation type="vanished">旧</translation></message>',
    )
    total, contexts, errors = audit.audit_catalog(path)
    assert (total.total, total.unfinished, total.empty, total.unchanged) == (3, 1, 0, 1)
    assert contexts["Example"].total == 3
    assert errors == []


def test_detects_missing_and_mismatched_placeholders(tmp_path: Path) -> None:
    path = make_catalog(
        tmp_path / "example.ts",
        "<message><source>Altitude %1 %L2</source><translation>高度 %1</translation></message>"
        "<message><source>Rate %n</source><translation>速率 %n</translation></message>"
        "<message><source>Missing</source></message>",
    )
    total, _, errors = audit.audit_catalog(path)
    assert (total.total, total.unfinished, total.empty, total.placeholders) == (3, 1, 1, 1)
    assert len(errors) == 1
    assert "Altitude %1 %L2" in errors[0]


def test_checks_each_plural_variant(tmp_path: Path) -> None:
    path = make_catalog(
        tmp_path / "example.ts",
        '<message numerus="yes"><source>%n items</source><translation>'
        "<numerusform>%n 项</numerusform><numerusform>若干项</numerusform>"
        "</translation></message>",
    )
    total, _, errors = audit.audit_catalog(path)
    assert total.placeholders == 1
    assert len(errors) == 1


@pytest.mark.parametrize("strict,expected", [(False, 0), (True, 1)])
def test_strict_mode(tmp_path: Path, capsys: pytest.CaptureFixture[str], strict: bool, expected: int) -> None:
    path = make_catalog(
        tmp_path / "example.ts",
        '<message><source>Land</source><translation type="unfinished">Land</translation></message>',
    )
    with patch.object(audit, "CATALOGS", (path,)), patch(
        "sys.argv", ["check_zh_cn.py", *(["--strict"] if strict else [])]
    ):
        assert audit.main() == expected
    assert "unfinished=1" in capsys.readouterr().out
