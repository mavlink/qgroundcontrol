#!/usr/bin/env python3
"""
QGroundControl Language Server

Provides IDE integration for QGC-specific patterns:
- Vehicle null-check diagnostics
- Fact usage validation
- MAVLink message completion

Usage:
    python -m tools.lsp.server          # STDIO mode (for editors)
    python -m tools.lsp.server --tcp    # TCP mode (for debugging)

Requirements:
    pip install pygls lsprotocol
"""

import argparse
import logging
import re
import sys
from pathlib import Path
from typing import Optional

from lsprotocol import types
from pygls.lsp.server import LanguageServer
from pygls.workspace import TextDocument

from .mavlink_data import MAVLinkMessage, get_all_messages
from .fact_schema import (
    FACT_PROPERTIES,
    COMMON_UNITS,
    ROOT_KEYS,
    get_type_values,
)

# Logging setup
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    handlers=[logging.StreamHandler(sys.stderr)],
)
logger = logging.getLogger("qgc-lsp")


class QGCLanguageServer(LanguageServer):
    """Language server for QGroundControl C++ development."""

    def __init__(self):
        super().__init__(
            name="qgc-language-server",
            version="0.1.0",
        )
        self.diagnostics: dict[str, tuple[Optional[int], list[types.Diagnostic]]] = {}
        self._project_root: Optional[Path] = None
        self._mavlink_messages: Optional[list[MAVLinkMessage]] = None

    def set_project_root(self, root: Path):
        """Set the project root directory for dynamic loading."""
        self._project_root = root
        self._mavlink_messages = None  # Reset cache

    def get_mavlink_messages(self) -> list[MAVLinkMessage]:
        """Get MAVLink messages, loading from XML if project root is set."""
        if self._mavlink_messages is None:
            self._mavlink_messages = get_all_messages(self._project_root)
        return self._mavlink_messages

    def analyze_document(self, document: TextDocument) -> list[types.Diagnostic]:
        """Analyze a document and return diagnostics."""
        # Only analyze C++ files
        if not self._is_cpp_file(document.uri):
            return []

        diagnostics = []
        diagnostics.extend(self._check_vehicle_null_safety(document))
        diagnostics.extend(self._check_parameter_access(document))

        return diagnostics

    def _is_cpp_file(self, uri: str) -> bool:
        """Check if the URI points to a C++ file."""
        path = uri.lower()
        return any(path.endswith(ext) for ext in ('.cpp', '.cc', '.cxx', '.h', '.hpp', '.hxx'))

    def _is_fact_json_file(self, uri: str) -> bool:
        """Check if the URI points to a Fact metadata JSON file."""
        path = uri.lower()
        # Match *Fact.json, *.FactMetaData.json, or files in Vehicle/FactGroups
        if path.endswith('fact.json') or path.endswith('.factmetadata.json'):
            return True
        if '/factgroups/' in path and path.endswith('.json'):
            return True
        return False

    def _check_vehicle_null_safety(self, document: TextDocument) -> list[types.Diagnostic]:
        """Check for unsafe activeVehicle() access patterns."""
        diagnostics = []

        # Pattern: activeVehicle() followed by -> without null check
        # This is a simplified check - the real analyzer is more sophisticated
        active_vehicle_pattern = re.compile(
            r'(\w+)\s*=\s*(?:MultiVehicleManager::instance\(\)|_vehicleManager|qgcApp\(\)->toolbox\(\)->multiVehicleManager\(\))->activeVehicle\(\)'
        )

        lines = document.lines
        vehicle_vars: dict[str, int] = {}  # var_name -> line_declared
        null_checked_vars: set[str] = set()

        for idx, line in enumerate(lines):
            # Track vehicle variable assignments
            match = active_vehicle_pattern.search(line)
            if match:
                var_name = match.group(1)
                vehicle_vars[var_name] = idx
                # Reset null-checked status for this var
                if var_name in null_checked_vars:
                    null_checked_vars.remove(var_name)

            # Track null checks
            for var_name in vehicle_vars:
                # Check for if (!var) or if (var == nullptr) patterns
                if re.search(rf'\bif\s*\(\s*!?\s*{re.escape(var_name)}\s*(?:==\s*nullptr|!=\s*nullptr|\))', line):
                    null_checked_vars.add(var_name)
                # Also check early return pattern
                if re.search(rf'if\s*\(\s*!{re.escape(var_name)}\s*\)\s*(?:return|continue|break)', line):
                    null_checked_vars.add(var_name)

            # Check for unsafe dereferences
            for var_name, decl_line in vehicle_vars.items():
                if var_name not in null_checked_vars and idx > decl_line:
                    # Look for var-> usage
                    deref_match = re.search(rf'\b{re.escape(var_name)}->', line)
                    if deref_match:
                        col = deref_match.start()
                        diagnostics.append(
                            types.Diagnostic(
                                message=f"Potential null pointer dereference: '{var_name}' from activeVehicle() used without null check",
                                severity=types.DiagnosticSeverity.Warning,
                                source="qgc-lsp",
                                code="null-vehicle",
                                range=types.Range(
                                    start=types.Position(line=idx, character=col),
                                    end=types.Position(line=idx, character=col + len(var_name) + 2),
                                ),
                                related_information=[
                                    types.DiagnosticRelatedInformation(
                                        location=types.Location(
                                            uri=document.uri,
                                            range=types.Range(
                                                start=types.Position(line=decl_line, character=0),
                                                end=types.Position(line=decl_line, character=len(lines[decl_line])),
                                            ),
                                        ),
                                        message=f"'{var_name}' assigned here",
                                    )
                                ],
                            )
                        )
                        # Only report once per variable per function scope
                        null_checked_vars.add(var_name)

        return diagnostics

    def _check_parameter_access(self, document: TextDocument) -> list[types.Diagnostic]:
        """Check for unsafe getParameter() access patterns."""
        diagnostics = []

        # Pattern: getParameter() result used without null check
        get_param_pattern = re.compile(
            r'(\w+)\s*=\s*\w+->parameterManager\(\)->getParameter\s*\([^)]+\)'
        )

        lines = document.lines
        param_vars: dict[str, int] = {}
        null_checked_vars: set[str] = set()

        for idx, line in enumerate(lines):
            # Track parameter variable assignments
            match = get_param_pattern.search(line)
            if match:
                var_name = match.group(1)
                param_vars[var_name] = idx

            # Track null checks
            for var_name in param_vars:
                if re.search(rf'\bif\s*\(\s*!?\s*{re.escape(var_name)}\s*(?:==\s*nullptr|!=\s*nullptr|\))', line):
                    null_checked_vars.add(var_name)

            # Check for unsafe dereferences
            for var_name, decl_line in param_vars.items():
                if var_name not in null_checked_vars and idx > decl_line:
                    deref_match = re.search(rf'\b{re.escape(var_name)}->', line)
                    if deref_match:
                        col = deref_match.start()
                        diagnostics.append(
                            types.Diagnostic(
                                message=f"Potential null pointer dereference: '{var_name}' from getParameter() used without null check",
                                severity=types.DiagnosticSeverity.Warning,
                                source="qgc-lsp",
                                code="null-parameter",
                                range=types.Range(
                                    start=types.Position(line=idx, character=col),
                                    end=types.Position(line=idx, character=col + len(var_name) + 2),
                                ),
                            )
                        )
                        null_checked_vars.add(var_name)

        return diagnostics


# Create server instance
server = QGCLanguageServer()


@server.feature(types.INITIALIZED)
def initialized(ls: QGCLanguageServer, params: types.InitializedParams):
    """Handle post-initialization setup."""
    # Try to get the workspace root for dynamic MAVLink loading
    if ls.workspace.root_path:
        root = Path(ls.workspace.root_path).resolve()
        ls.set_project_root(root)
        logger.info(f"Set project root to: {root}")
    elif ls.workspace.root_uri:
        # Convert URI to path (resolve to canonical path for security)
        from urllib.parse import urlparse, unquote
        parsed = urlparse(ls.workspace.root_uri)
        root = Path(unquote(parsed.path)).resolve()
        ls.set_project_root(root)
        logger.info(f"Set project root to: {root}")


@server.feature(types.TEXT_DOCUMENT_DID_OPEN)
def did_open(ls: QGCLanguageServer, params: types.DidOpenTextDocumentParams):
    """Analyze document when opened."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    diagnostics = ls.analyze_document(doc)

    ls.diagnostics[doc.uri] = (doc.version, diagnostics)
    ls.text_document_publish_diagnostics(
        types.PublishDiagnosticsParams(
            uri=doc.uri,
            version=doc.version,
            diagnostics=diagnostics,
        )
    )
    logger.info(f"Analyzed {doc.uri}: {len(diagnostics)} diagnostics")


@server.feature(types.TEXT_DOCUMENT_DID_CHANGE)
def did_change(ls: QGCLanguageServer, params: types.DidChangeTextDocumentParams):
    """Re-analyze document when changed."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    diagnostics = ls.analyze_document(doc)

    ls.diagnostics[doc.uri] = (doc.version, diagnostics)
    ls.text_document_publish_diagnostics(
        types.PublishDiagnosticsParams(
            uri=doc.uri,
            version=doc.version,
            diagnostics=diagnostics,
        )
    )


@server.feature(types.TEXT_DOCUMENT_DID_CLOSE)
def did_close(ls: QGCLanguageServer, params: types.DidCloseTextDocumentParams):
    """Clear diagnostics when document is closed."""
    uri = params.text_document.uri
    if uri in ls.diagnostics:
        del ls.diagnostics[uri]
    # Publish empty diagnostics to clear
    ls.text_document_publish_diagnostics(
        types.PublishDiagnosticsParams(uri=uri, diagnostics=[])
    )


@server.feature(types.TEXT_DOCUMENT_DID_SAVE)
def did_save(ls: QGCLanguageServer, params: types.DidSaveTextDocumentParams):
    """Re-analyze on save (optional, for editors that don't sync on change)."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    diagnostics = ls.analyze_document(doc)

    ls.diagnostics[doc.uri] = (doc.version, diagnostics)
    ls.text_document_publish_diagnostics(
        types.PublishDiagnosticsParams(
            uri=doc.uri,
            diagnostics=diagnostics,
        )
    )


@server.feature(
    types.TEXT_DOCUMENT_COMPLETION,
    types.CompletionOptions(
        trigger_characters=["_", "M", "m", '"', ":"],
        resolve_provider=True,
    ),
)
def completion(ls: QGCLanguageServer, params: types.CompletionParams) -> types.CompletionList:
    """Provide MAVLink message and Fact JSON completions."""
    doc = ls.workspace.get_text_document(params.text_document.uri)

    # Get the current line and cursor position
    line_num = params.position.line
    char_pos = params.position.character
    lines = doc.lines

    if line_num >= len(lines):
        return types.CompletionList(is_incomplete=False, items=[])

    line = lines[line_num]
    prefix_text = line[:char_pos]

    # Handle Fact JSON files
    if ls._is_fact_json_file(doc.uri):
        items = _get_fact_json_completions(lines, line_num, char_pos, prefix_text)
        if items:
            logger.debug(f"Fact JSON completion: {len(items)} items")
            return types.CompletionList(is_incomplete=False, items=items)
        return types.CompletionList(is_incomplete=False, items=[])

    # Handle C++ files
    if not ls._is_cpp_file(doc.uri):
        return types.CompletionList(is_incomplete=False, items=[])

    items: list[types.CompletionItem] = []

    # Get MAVLink messages (uses dynamic loading if project root is set)
    messages = ls.get_mavlink_messages()

    # Check for MAVLINK_MSG_ID_ completion
    match = re.search(r'MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE)
    if match:
        filter_text = match.group(1).upper()
        items = _get_message_id_completions(filter_text, messages)
        logger.debug(f"MAVLINK_MSG_ID_ completion: {len(items)} items for '{filter_text}'")
        return types.CompletionList(is_incomplete=False, items=items)

    # Check for mavlink_msg_ completion (decode/get functions)
    match = re.search(r'mavlink_msg_(\w*)$', prefix_text, re.IGNORECASE)
    if match:
        filter_text = match.group(1).lower()
        items = _get_decode_completions(filter_text, messages)
        logger.debug(f"mavlink_msg_ completion: {len(items)} items for '{filter_text}'")
        return types.CompletionList(is_incomplete=False, items=items)

    # Check for switch(message.msgid) context - suggest case statements
    if _is_in_switch_context(lines, line_num, char_pos):
        if re.search(r'case\s+MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE):
            match = re.search(r'case\s+MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE)
            if match:
                filter_text = match.group(1).upper()
                items = _get_message_id_completions(filter_text, messages, include_case=True)
        elif prefix_text.strip().endswith("case") or prefix_text.strip() == "":
            items = _get_case_completions(messages)
        logger.debug(f"Switch context completion: {len(items)} items")
        return types.CompletionList(is_incomplete=False, items=items)

    return types.CompletionList(is_incomplete=False, items=items)


@server.feature(types.COMPLETION_ITEM_RESOLVE)
def completion_resolve(ls: QGCLanguageServer, item: types.CompletionItem) -> types.CompletionItem:
    """Resolve additional details for a completion item."""
    # Add documentation from the stored data
    if item.data and isinstance(item.data, dict):
        msg_name = item.data.get("message")
        if msg_name:
            # Search in dynamic messages first
            messages = ls.get_mavlink_messages()
            msg = next((m for m in messages if m.name == msg_name), None)
            if msg:
                doc_lines = [f"**{msg.name}** (ID: {msg.id})", "", msg.description, "", "**Fields:**"]
                for field in msg.fields:
                    field_doc = f"- `{field.name}`: {field.type}"
                    if field.units:
                        field_doc += f" ({field.units})"
                    if field.description:
                        field_doc += f" — {field.description}"
                    doc_lines.append(field_doc)
                item.documentation = types.MarkupContent(
                    kind=types.MarkupKind.Markdown,
                    value="\n".join(doc_lines),
                )
    return item


def _get_message_id_completions(
    filter_text: str,
    messages: list[MAVLinkMessage],
    include_case: bool = False,
) -> list[types.CompletionItem]:
    """Get completion items for MAVLINK_MSG_ID_ constants."""
    items = []
    for msg in messages:
        if filter_text and not msg.name.startswith(filter_text):
            continue

        label = f"MAVLINK_MSG_ID_{msg.name}"
        if include_case:
            # Include full case statement with decode pattern
            insert_text = f"""MAVLINK_MSG_ID_{msg.name}: {{
        mavlink_{msg.name.lower()}_t {msg.name.lower()};
        mavlink_msg_{msg.name.lower()}_decode(&message, &{msg.name.lower()});
        // TODO: Update Facts from {msg.name.lower()}
        break;
    }}"""
        else:
            insert_text = label

        items.append(types.CompletionItem(
            label=label,
            kind=types.CompletionItemKind.Constant,
            detail=f"[{msg.id}] {msg.category}",
            documentation=msg.description,
            insert_text=insert_text,
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"{msg.id:03d}",  # Sort by message ID
            data={"message": msg.name},
        ))
    return items


def _get_decode_completions(filter_text: str, messages: list[MAVLinkMessage]) -> list[types.CompletionItem]:
    """Get completion items for mavlink_msg_* functions."""
    items = []
    for msg in messages:
        lower_name = msg.name.lower()

        # Check if filter matches message name
        if filter_text:
            # Allow matching from any part of the message name
            if not lower_name.startswith(filter_text) and filter_text not in lower_name:
                continue

        # Add decode function
        decode_label = f"mavlink_msg_{lower_name}_decode"
        items.append(types.CompletionItem(
            label=decode_label,
            kind=types.CompletionItemKind.Function,
            detail=f"Decode {msg.name} message",
            documentation=f"Decode a {msg.name} message from the raw buffer.",
            insert_text=f"mavlink_msg_{lower_name}_decode(&message, &{lower_name})",
            insert_text_format=types.InsertTextFormat.PlainText,
            sort_text=f"0_{lower_name}",  # Decode first
            data={"message": msg.name},
        ))

        # Add getter functions for each field
        for field in msg.fields:
            getter_label = f"mavlink_msg_{lower_name}_get_{field.name}"
            field_detail = f"Get {msg.name}.{field.name}"
            if field.units:
                field_detail += f" ({field.units})"

            items.append(types.CompletionItem(
                label=getter_label,
                kind=types.CompletionItemKind.Function,
                detail=field_detail,
                documentation=field.description or f"Get the {field.name} field from a {msg.name} message.",
                insert_text=f"mavlink_msg_{lower_name}_get_{field.name}(&message)",
                insert_text_format=types.InsertTextFormat.PlainText,
                sort_text=f"1_{lower_name}_{field.name}",
                data={"message": msg.name, "field": field.name},
            ))

    return items


def _get_case_completions(messages: list[MAVLinkMessage]) -> list[types.CompletionItem]:
    """Get completion items for case statements in a switch(msgid) block."""
    items = []
    for msg in messages:
        lower_name = msg.name.lower()

        # Full case statement with decode pattern
        snippet = f"""case MAVLINK_MSG_ID_{msg.name}: {{
        mavlink_{lower_name}_t {lower_name};
        mavlink_msg_{lower_name}_decode(&message, &{lower_name});
        // TODO: Update Facts from {lower_name}
        break;
    }}"""

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


def _is_in_switch_context(lines: list[str], line_num: int, char_pos: int) -> bool:
    """Check if cursor is inside a switch(message.msgid) block."""
    # Look backwards for switch statement
    brace_depth = 0
    for i in range(line_num, max(-1, line_num - 50), -1):
        line = lines[i] if i < len(lines) else ""

        # Count braces (simplified - doesn't handle strings/comments)
        brace_depth += line.count('}') - line.count('{')

        # Check for switch on msgid
        if re.search(r'switch\s*\(\s*\w+\.msgid\s*\)', line):
            return brace_depth <= 0  # Inside the switch block
        if re.search(r'switch\s*\(\s*message\.msgid\s*\)', line):
            return brace_depth <= 0

    return False


def _get_fact_json_completions(
    lines: list[str], line_num: int, char_pos: int, prefix_text: str
) -> list[types.CompletionItem]:
    """Get completion items for Fact JSON files."""
    items: list[types.CompletionItem] = []

    # Determine context: are we in the Facts array or at root level?
    in_facts_array = _is_in_facts_array(lines, line_num)

    # Check if we're typing a property key (after { or ,)
    # Pattern: after opening brace or comma, possibly with whitespace and quote
    key_context = re.search(r'[{,]\s*"(\w*)$', prefix_text)
    if key_context:
        filter_text = key_context.group(1).lower()
        if in_facts_array:
            items = _get_fact_property_completions(filter_text)
        else:
            items = _get_root_key_completions(filter_text)
        return items

    # Check if we're typing a value for "type": "
    type_value_context = re.search(r'"type"\s*:\s*"(\w*)$', prefix_text)
    if type_value_context:
        filter_text = type_value_context.group(1).lower()
        items = _get_type_value_completions(filter_text)
        return items

    # Check if we're typing a value for "units": "
    units_value_context = re.search(r'"units"\s*:\s*"([^"]*)$', prefix_text)
    if units_value_context:
        filter_text = units_value_context.group(1).lower()
        items = _get_units_value_completions(filter_text)
        return items

    # Check if we're typing a boolean value
    bool_value_context = re.search(
        r'"(volatileValue|hasControl|readOnly|writeOnly|nanUnchanged|rebootRequired)"\s*:\s*(\w*)$',
        prefix_text
    )
    if bool_value_context:
        filter_text = bool_value_context.group(2).lower()
        items = _get_boolean_completions(filter_text)
        return items

    return items


def _is_in_facts_array(lines: list[str], line_num: int) -> bool:
    """Check if cursor is inside the QGC.MetaData.Facts array."""
    # Look backwards for "QGC.MetaData.Facts" key
    bracket_depth = 0
    for i in range(line_num, max(-1, line_num - 100), -1):
        line = lines[i] if i < len(lines) else ""
        bracket_depth += line.count(']') - line.count('[')

        if '"QGC.MetaData.Facts"' in line or '"QGC.MetaData.Facts":' in line:
            return bracket_depth <= 0

    return False


def _get_fact_property_completions(filter_text: str) -> list[types.CompletionItem]:
    """Get completion items for Fact property keys."""
    items = []
    for prop in FACT_PROPERTIES:
        if filter_text and not prop.name.lower().startswith(filter_text):
            continue

        # Required properties get higher priority
        sort_prefix = "0" if prop.required else "1"

        detail = f"{'Required' if prop.required else 'Optional'}"
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
    """Get completion items for root-level JSON keys."""
    items = []
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
    """Get completion items for Fact type values."""
    items = []
    type_values = get_type_values()

    for type_val in type_values:
        if filter_text and not type_val.lower().startswith(filter_text):
            continue

        # Add description based on type
        descriptions = {
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

        items.append(types.CompletionItem(
            label=type_val,
            kind=types.CompletionItemKind.EnumMember,
            detail="Fact type",
            documentation=descriptions.get(type_val, ""),
            insert_text=f'{type_val}"',
            insert_text_format=types.InsertTextFormat.PlainText,
        ))
    return items


def _get_units_value_completions(filter_text: str) -> list[types.CompletionItem]:
    """Get completion items for common unit values."""
    items = []
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
    """Get completion items for boolean values."""
    items = []
    for val in ["true", "false"]:
        if filter_text and not val.startswith(filter_text):
            continue

        items.append(types.CompletionItem(
            label=val,
            kind=types.CompletionItemKind.Keyword,
            detail="Boolean",
            insert_text=val,
            insert_text_format=types.InsertTextFormat.PlainText,
        ))
    return items


# ============================================================================
# Go-to-Definition for Facts
# ============================================================================

@server.feature(types.TEXT_DOCUMENT_DEFINITION)
def definition(
    ls: QGCLanguageServer, params: types.DefinitionParams
) -> Optional[types.Location]:
    """Provide go-to-definition for Fact references."""
    doc = ls.workspace.get_text_document(params.text_document.uri)

    # Only handle C++ files
    if not ls._is_cpp_file(doc.uri):
        return None

    line_num = params.position.line
    char_pos = params.position.character
    lines = doc.lines

    if line_num >= len(lines):
        return None

    line = lines[line_num]

    # Find the word under cursor
    word_start = char_pos
    word_end = char_pos

    while word_start > 0 and (line[word_start - 1].isalnum() or line[word_start - 1] == '_'):
        word_start -= 1
    while word_end < len(line) and (line[word_end].isalnum() or line[word_end] == '_'):
        word_end += 1

    word = line[word_start:word_end]

    # Try to find a Fact reference
    fact_name = _extract_fact_name(word, line, word_start, word_end)
    if not fact_name:
        return None

    logger.debug(f"Looking for Fact definition: {fact_name}")

    # Search for the Fact JSON definition
    location = _find_fact_definition(ls, fact_name)
    if location:
        logger.info(f"Found Fact definition for '{fact_name}' at {location.uri}")
        return location

    return None


def _extract_fact_name(word: str, line: str, word_start: int, word_end: int) -> Optional[str]:
    """Extract the Fact name from various patterns."""
    # Pattern 1: _nameFact member variable
    if word.startswith('_') and word.endswith('Fact'):
        return word[1:-4]  # Remove _ prefix and Fact suffix

    # Pattern 2: nameFact() accessor method
    if word.endswith('Fact'):
        # Check if followed by ()
        rest = line[word_end:].lstrip()
        if rest.startswith('('):
            return word[:-4]  # Remove Fact suffix

    # Pattern 3: QStringLiteral("name") in Fact constructor
    # Look for pattern like: Fact(0, QStringLiteral("name"), ...)
    string_match = re.search(r'QStringLiteral\s*\(\s*"(\w+)"\s*\)', line)
    if string_match:
        name = string_match.group(1)
        # Check if word is near the string literal
        if word_start <= string_match.end() and word_end >= string_match.start():
            return name

    # Pattern 4: Direct string in Fact JSON path
    # Like: ":/json/Vehicle/VehicleFact.json"
    json_match = re.search(r'":/json/([^"]+)Fact\.json"', line)
    if json_match:
        return None  # This is a FactGroup, not a single Fact

    return None


def _find_fact_definition(
    ls: QGCLanguageServer, fact_name: str
) -> Optional[types.Location]:
    """Find the JSON definition of a Fact."""
    if not ls._project_root:
        return None

    # Search for Fact JSON files in the project
    import glob

    json_patterns = [
        str(ls._project_root / "src" / "**" / "*Fact.json"),
        str(ls._project_root / "src" / "**" / "*Facts.json"),
        str(ls._project_root / "src" / "**" / "FactMetaData" / "*.json"),
    ]

    for pattern in json_patterns:
        for json_path in glob.glob(pattern, recursive=True):
            location = _search_fact_in_json(json_path, fact_name)
            if location:
                return location

    return None


def _search_fact_in_json(json_path: str, fact_name: str) -> Optional[types.Location]:
    """Search for a Fact name in a JSON file and return its location."""
    import json

    try:
        with open(json_path, 'r', encoding='utf-8') as f:
            content = f.read()

        # Parse JSON to find the Fact
        data = json.loads(content)
        facts = data.get("QGC.MetaData.Facts", [])

        for fact in facts:
            if fact.get("name") == fact_name:
                # Find the line number of this Fact in the file
                line_num = _find_fact_line(content, fact_name)
                if line_num >= 0:
                    return types.Location(
                        uri=f"file://{json_path}",
                        range=types.Range(
                            start=types.Position(line=line_num, character=0),
                            end=types.Position(line=line_num, character=0),
                        ),
                    )
    except (json.JSONDecodeError, IOError) as e:
        logger.debug(f"Failed to parse {json_path}: {e}")

    return None


def _find_fact_line(content: str, fact_name: str) -> int:
    """Find the line number where a Fact is defined in JSON content."""
    lines = content.split('\n')
    for i, line in enumerate(lines):
        # Look for "name": "factName"
        if re.search(rf'"name"\s*:\s*"{re.escape(fact_name)}"', line):
            return i
    return -1


def main():
    parser = argparse.ArgumentParser(description="QGroundControl Language Server")
    parser.add_argument(
        "--tcp",
        action="store_true",
        help="Run in TCP mode (default: STDIO)",
    )
    parser.add_argument(
        "--host",
        default="127.0.0.1",
        help="TCP host (default: 127.0.0.1)",
    )
    parser.add_argument(
        "--port",
        type=int,
        default=2087,
        help="TCP port (default: 2087)",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="Enable verbose logging",
    )

    args = parser.parse_args()

    if args.verbose:
        logging.getLogger().setLevel(logging.DEBUG)

    if args.tcp:
        logger.info(f"Starting QGC LSP server on {args.host}:{args.port}")
        server.start_tcp(args.host, args.port)
    else:
        logger.info("Starting QGC LSP server in STDIO mode")
        server.start_io()


if __name__ == "__main__":
    main()
