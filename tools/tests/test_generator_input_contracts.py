"""Reject malformed nested definitions before producing invalid source files."""

from __future__ import annotations

import pytest
from generators._jinja import make_env
from generators.common.controls import (
    parse_button,
    parse_dialog_button,
    parse_linked_params,
    parse_radio_options,
)
from jinja2 import UndefinedError


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
