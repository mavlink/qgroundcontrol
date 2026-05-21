"""Fact JSON completion providers."""

from __future__ import annotations

import re

from lsprotocol import types

from ..fact_schema import COMMON_UNITS, FACT_PROPERTIES, ROOT_KEYS, get_type_values

_BOOL_KEY_RE = re.compile(
    r'"(volatileValue|hasControl|readOnly|writeOnly|nanUnchanged|rebootRequired)"\s*:\s*(\w*)$'
)

_TYPE_VALUE_DESCRIPTIONS: dict[str, str] = {
    "uint8": "Unsigned 8-bit integer (0-255)",
    "int8": "Signed 8-bit integer (-128 to 127)",
    "uint16": "Unsigned 16-bit integer (0-65535)",
    "int16": "Signed 16-bit integer",
    "uint32": "Unsigned 32-bit integer",
    "int32": "Signed 32-bit integer",
    "uint64": "Unsigned 64-bit integer",
    "int64": "Signed 64-bit integer",
    "float": "32-bit floating point",
    "double": "64-bit floating point",
    "string": "Text string",
    "bool": "Boolean (true/false)",
    "elapsedTimeInSeconds": "Time duration in seconds",
    "custom": "Custom type (requires special handling)",
}


def get_fact_json_completions(
    lines: list[str], line_num: int, char_pos: int, prefix_text: str
) -> list[types.CompletionItem]:
    """Dispatch a Fact-JSON completion request to the right per-context provider."""
    in_facts_array = _is_in_facts_array(lines, line_num)

    key_context = re.search(r'[{,]\s*"(\w*)$', prefix_text)
    if key_context:
        filter_text = key_context.group(1).lower()
        return (
            _get_fact_property_completions(filter_text)
            if in_facts_array
            else _get_root_key_completions(filter_text)
        )

    type_value = re.search(r'"type"\s*:\s*"(\w*)$', prefix_text)
    if type_value:
        return _get_type_value_completions(type_value.group(1).lower())

    units_value = re.search(r'"units"\s*:\s*"([^"]*)$', prefix_text)
    if units_value:
        return _get_units_value_completions(units_value.group(1).lower())

    bool_value = _BOOL_KEY_RE.search(prefix_text)
    if bool_value:
        return _get_boolean_completions(bool_value.group(2).lower())

    return []


def _is_in_facts_array(lines: list[str], line_num: int) -> bool:
    bracket_depth = 0
    for i in range(line_num, max(-1, line_num - 100), -1):
        line = lines[i] if i < len(lines) else ""
        bracket_depth += line.count("]") - line.count("[")
        if '"QGC.MetaData.Facts"' in line or '"QGC.MetaData.Facts":' in line:
            return bracket_depth <= 0
    return False


def _get_fact_property_completions(filter_text: str) -> list[types.CompletionItem]:
    items: list[types.CompletionItem] = []
    for prop in FACT_PROPERTIES:
        if filter_text and not prop.name.lower().startswith(filter_text):
            continue
        sort_prefix = "0" if prop.required else "1"
        detail = "Required" if prop.required else "Optional"
        if prop.type == "enum":
            detail += f" | {', '.join(prop.enum_values[:3])}..."

        items.append(types.CompletionItem(
            label=prop.name,
            kind=types.CompletionItemKind.Property,
            detail=detail,
            documentation=prop.description,
            insert_text=f'"{prop.name}": ',
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"{sort_prefix}_{prop.name}",
        ))
    return items


def _get_root_key_completions(filter_text: str) -> list[types.CompletionItem]:
    items: list[types.CompletionItem] = []
    for key, description in ROOT_KEYS:
        if filter_text and not key.lower().startswith(filter_text):
            continue
        items.append(types.CompletionItem(
            label=key,
            kind=types.CompletionItemKind.Property,
            detail="Root property",
            documentation=description,
            insert_text=f'"{key}": ',
            insert_text_format=types.InsertTextFormat.PlainText,
        ))
    return items


def _get_type_value_completions(filter_text: str) -> list[types.CompletionItem]:
    items: list[types.CompletionItem] = []
    for type_val in get_type_values():
        if filter_text and not type_val.lower().startswith(filter_text):
            continue
        items.append(types.CompletionItem(
            label=type_val,
            kind=types.CompletionItemKind.EnumMember,
            detail="Fact type",
            documentation=_TYPE_VALUE_DESCRIPTIONS.get(type_val, ""),
            insert_text=f'{type_val}"',
            insert_text_format=types.InsertTextFormat.PlainText,
        ))
    return items


def _get_units_value_completions(filter_text: str) -> list[types.CompletionItem]:
    items: list[types.CompletionItem] = []
    for unit, description in COMMON_UNITS:
        if filter_text and not unit.lower().startswith(filter_text):
            continue
        items.append(types.CompletionItem(
            label=unit,
            kind=types.CompletionItemKind.Unit,
            detail=description,
            insert_text=f'{unit}"',
            insert_text_format=types.InsertTextFormat.PlainText,
        ))
    return items


def _get_boolean_completions(filter_text: str) -> list[types.CompletionItem]:
    return [
        types.CompletionItem(
            label=val,
            kind=types.CompletionItemKind.Keyword,
            detail="Boolean",
            insert_text=val,
            insert_text_format=types.InsertTextFormat.PlainText,
        )
        for val in ("true", "false")
        if not filter_text or val.startswith(filter_text)
    ]
