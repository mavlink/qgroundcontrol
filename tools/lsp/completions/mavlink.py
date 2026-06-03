"""MAVLink-related completion providers."""

from __future__ import annotations

import re

from lsprotocol import types

from ..mavlink_data import MAVLinkMessage  # noqa: TC001


def get_message_id_completions(
    filter_text: str,
    messages: list[MAVLinkMessage],
    include_case: bool = False,
) -> list[types.CompletionItem]:
    """Suggest MAVLINK_MSG_ID_* constants matching the filter prefix."""
    items: list[types.CompletionItem] = []
    for msg in messages:
        if filter_text and not msg.name.startswith(filter_text):
            continue

        label = f"MAVLINK_MSG_ID_{msg.name}"
        if include_case:
            insert_text = (
                f"MAVLINK_MSG_ID_{msg.name}: {{\n"
                f"        mavlink_{msg.name.lower()}_t {msg.name.lower()};\n"
                f"        mavlink_msg_{msg.name.lower()}_decode(&message, &{msg.name.lower()});\n"
                f"        // TODO: Update Facts from {msg.name.lower()}\n"
                f"        break;\n"
                f"    }}"
            )
        else:
            insert_text = label

        items.append(types.CompletionItem(
            label=label,
            kind=types.CompletionItemKind.Constant,
            detail=f"[{msg.id}] {msg.category}",
            documentation=msg.description,
            insert_text=insert_text,
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"{msg.id:03d}",
            data={"message": msg.name},
        ))
    return items


def get_decode_completions(
    filter_text: str, messages: list[MAVLinkMessage]
) -> list[types.CompletionItem]:
    """Suggest mavlink_msg_<name>_decode and mavlink_msg_<name>_get_<field> helpers."""
    items: list[types.CompletionItem] = []
    for msg in messages:
        lower_name = msg.name.lower()
        if filter_text and not lower_name.startswith(filter_text) and filter_text not in lower_name:
            continue

        items.append(types.CompletionItem(
            label=f"mavlink_msg_{lower_name}_decode",
            kind=types.CompletionItemKind.Function,
            detail=f"Decode {msg.name} message",
            documentation=f"Decode a {msg.name} message from the raw buffer.",
            insert_text=f"mavlink_msg_{lower_name}_decode(&message, &{lower_name})",
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"0_{lower_name}",
            data={"message": msg.name},
        ))

        for field in msg.fields:
            detail = f"Get {msg.name}.{field.name}"
            if field.units:
                detail += f" ({field.units})"
            items.append(types.CompletionItem(
                label=f"mavlink_msg_{lower_name}_get_{field.name}",
                kind=types.CompletionItemKind.Function,
                detail=detail,
                documentation=field.description or f"Get the {field.name} field from a {msg.name} message.",
                insert_text=f"mavlink_msg_{lower_name}_get_{field.name}(&message)",
                insert_text_format=types.InsertTextFormat.PlainText,
                sort_text=f"1_{lower_name}_{field.name}",
                data={"message": msg.name, "field": field.name},
            ))

    return items


def get_case_completions(messages: list[MAVLinkMessage]) -> list[types.CompletionItem]:
    """Suggest full `case MAVLINK_MSG_ID_*:` snippets inside a switch(msgid) block."""
    items: list[types.CompletionItem] = []
    for msg in messages:
        lower_name = msg.name.lower()
        snippet = (
            f"case MAVLINK_MSG_ID_{msg.name}: {{\n"
            f"        mavlink_{lower_name}_t {lower_name};\n"
            f"        mavlink_msg_{lower_name}_decode(&message, &{lower_name});\n"
            f"        // TODO: Update Facts from {lower_name}\n"
            f"        break;\n"
            f"    }}"
        )
        items.append(types.CompletionItem(
            label=f"case MAVLINK_MSG_ID_{msg.name}",
            kind=types.CompletionItemKind.Snippet,
            detail=f"[{msg.id}] {msg.description}",
            documentation=f"Handle {msg.name} message with decode pattern.",
            insert_text=snippet,
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"{msg.id:03d}",
            data={"message": msg.name},
        ))
    return items


def is_in_switch_context(lines: list[str], line_num: int, char_pos: int) -> bool:
    """Return True when the cursor sits inside a switch(<x>.msgid) block."""
    brace_depth = 0
    for i in range(line_num, max(-1, line_num - 50), -1):
        line = lines[i] if i < len(lines) else ""
        brace_depth += line.count("}") - line.count("{")
        if re.search(r"switch\s*\(\s*\w+\.msgid\s*\)", line):
            return brace_depth <= 0
        if re.search(r"switch\s*\(\s*message\.msgid\s*\)", line):
            return brace_depth <= 0
    return False
