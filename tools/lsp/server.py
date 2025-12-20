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

from .mavlink_data import MAVLINK_MESSAGES, MAVLinkMessage

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

    def _check_vehicle_null_safety(self, document: TextDocument) -> list[types.Diagnostic]:
        """Check for unsafe activeVehicle() access patterns."""
        diagnostics = []

        # Pattern: activeVehicle() followed by -> without null check
        # This is a simplified check - the real analyzer is more sophisticated
        active_vehicle_pattern = re.compile(
            r'(\w+)\s*=\s*(?:MultiVehicleManager::instance\(\)|_vehicleManager|qgcApp\(\)->toolbox\(\)->multiVehicleManager\(\))->activeVehicle\(\)'
        )
        unsafe_deref_pattern = re.compile(r'\b(\w+)->(?!$)')

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
        trigger_characters=["_", "M", "m"],
        resolve_provider=True,
    ),
)
def completion(ls: QGCLanguageServer, params: types.CompletionParams) -> types.CompletionList:
    """Provide MAVLink message completions."""
    doc = ls.workspace.get_text_document(params.text_document.uri)

    # Only provide completions for C++ files
    if not ls._is_cpp_file(doc.uri):
        return types.CompletionList(is_incomplete=False, items=[])

    # Get the current line and cursor position
    line_num = params.position.line
    char_pos = params.position.character
    lines = doc.lines

    if line_num >= len(lines):
        return types.CompletionList(is_incomplete=False, items=[])

    line = lines[line_num]
    prefix_text = line[:char_pos]

    items: list[types.CompletionItem] = []

    # Check for MAVLINK_MSG_ID_ completion
    match = re.search(r'MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE)
    if match:
        filter_text = match.group(1).upper()
        items = _get_message_id_completions(filter_text)
        logger.debug(f"MAVLINK_MSG_ID_ completion: {len(items)} items for '{filter_text}'")
        return types.CompletionList(is_incomplete=False, items=items)

    # Check for mavlink_msg_ completion (decode/get functions)
    match = re.search(r'mavlink_msg_(\w*)$', prefix_text, re.IGNORECASE)
    if match:
        filter_text = match.group(1).lower()
        items = _get_decode_completions(filter_text)
        logger.debug(f"mavlink_msg_ completion: {len(items)} items for '{filter_text}'")
        return types.CompletionList(is_incomplete=False, items=items)

    # Check for switch(message.msgid) context - suggest case statements
    if _is_in_switch_context(lines, line_num, char_pos):
        if re.search(r'case\s+MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE):
            match = re.search(r'case\s+MAVLINK_MSG_ID_(\w*)$', prefix_text, re.IGNORECASE)
            if match:
                filter_text = match.group(1).upper()
                items = _get_message_id_completions(filter_text, include_case=True)
        elif prefix_text.strip().endswith("case") or prefix_text.strip() == "":
            items = _get_case_completions()
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
            from .mavlink_data import MESSAGES_BY_NAME
            msg = MESSAGES_BY_NAME.get(msg_name)
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


def _get_message_id_completions(filter_text: str, include_case: bool = False) -> list[types.CompletionItem]:
    """Get completion items for MAVLINK_MSG_ID_ constants."""
    items = []
    for msg in MAVLINK_MESSAGES:
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


def _get_decode_completions(filter_text: str) -> list[types.CompletionItem]:
    """Get completion items for mavlink_msg_* functions."""
    items = []
    for msg in MAVLINK_MESSAGES:
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


def _get_case_completions() -> list[types.CompletionItem]:
    """Get completion items for case statements in a switch(msgid) block."""
    items = []
    for msg in MAVLINK_MESSAGES:
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
