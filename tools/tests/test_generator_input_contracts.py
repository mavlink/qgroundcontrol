"""Reject malformed nested definitions before producing invalid source files."""

from __future__ import annotations

import json

import pytest
from generators._jinja import make_env
from generators.common.controls import (
    parse_button,
    parse_dialog_button,
    parse_linked_params,
    parse_radio_options,
    qml_string,
    qml_tr,
)
from generators.common.validation import require_param_name
from jinja2 import UndefinedError

_TRICKY = 'Say "hi" to C:\\temp\n°'


@pytest.mark.parametrize(
    "parse,value",
    [
        (parse_button, {"text": 3}),
        (parse_dialog_button, {"dialogParams": []}),
        (parse_dialog_button, {"dialogParams": {"value": None}}),
        (parse_dialog_button, {"buttonAfter": "false"}),
        (parse_linked_params, {"PARAM": []}),
        (parse_radio_options, [{"checked": False}]),
    ],
)
def test_invalid_nested_control_fields(parse, value):
    with pytest.raises(ValueError):
        parse(value)


def test_missing_template_field_fails_instead_of_emitting_empty_source(tmp_path):
    (tmp_path / "control.jinja").write_text(
        "property int count: {{ missing_count }}", encoding="utf-8"
    )
    with pytest.raises(UndefinedError, match="missing_count"):
        make_env(tmp_path).get_template("control.jinja").render()


def test_qml_string_escapes_quotes_backslashes_and_newlines():
    literal = qml_string(_TRICKY)
    assert literal == '"Say \\"hi\\" to C:\\\\temp\\n°"'
    assert json.loads(literal) == _TRICKY


def test_qml_string_leaves_plain_text_unescaped():
    assert qml_string("Battery 1") == '"Battery 1"'


def test_qml_tr_escapes_text_and_context():
    assert qml_tr('a "b"') == 'qsTr("a \\"b\\"")'
    assert qml_tr("x\\y", 'C"tx') == 'qsTranslate("C\\"tx", "x\\\\y")'


def test_make_env_registers_qml_string_filter(tmp_path):
    (tmp_path / "t.qml.j2").write_text("text: {{ value|qml_string }}", encoding="utf-8")
    rendered = make_env(tmp_path).get_template("t.qml.j2").render(value=_TRICKY)
    assert rendered == f"text: {qml_string(_TRICKY)}"


@pytest.mark.parametrize("name", ["", "BATT_MONITOR", "_SOURCE", "noremap.RTL_ALT_M", "bat1"])
def test_param_name_accepted(name):
    assert require_param_name(name, "param", "test.json") == name


@pytest.mark.parametrize(
    "name", ['BAD"PARAM', "A\\B", "BAT[0]", "two words", "a.b", "noremap.", 7, None]
)
def test_param_name_rejected(name):
    with pytest.raises(ValueError, match="param"):
        require_param_name(name, "param", "test.json")
