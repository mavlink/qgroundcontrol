#!/usr/bin/env python3
"""QGroundControl Language Server.

Provides IDE integration for QGC-specific patterns:
- Vehicle / Fact null-check diagnostics (see lsp.diagnostics)
- MAVLink message and Fact JSON completions (see lsp.completions)
- Go-to-definition for Fact references (see lsp.goto)

Usage:
    python -m tools.lsp.server          # STDIO mode (for editors)
    python -m tools.lsp.server --tcp    # TCP mode (for debugging)

Requirements:
    pip install pygls lsprotocol
"""

from __future__ import annotations

import argparse
import logging
import re
import sys
from pathlib import Path

from lsprotocol import types
from pygls.lsp.server import LanguageServer

from .completions import (
    get_case_completions,
    get_decode_completions,
    get_fact_json_completions,
    get_message_id_completions,
    is_in_switch_context,
)
from .diagnostics import check_parameter_access, check_vehicle_null_safety
from .goto import extract_fact_name, find_fact_definition
from .mavlink_data import MAVLinkMessage, get_all_messages

logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(name)s: %(message)s",
    handlers=[logging.StreamHandler(sys.stderr)],
)
logger = logging.getLogger("qgc-lsp")


class QGCLanguageServer(LanguageServer):
    """Language server for QGroundControl C++ development."""

    _CPP_EXTS = (".cpp", ".cc", ".cxx", ".h", ".hpp", ".hxx")

    def __init__(self):
        super().__init__(name="qgc-language-server", version="0.1.0")
        self.diagnostics: dict[str, tuple[int | None, list[types.Diagnostic]]] = {}
        self._project_root: Path | None = None
        self._mavlink_messages: list[MAVLinkMessage] | None = None

    @property
    def project_root(self) -> Path | None:
        return self._project_root

    def set_project_root(self, root: Path) -> None:
        self._project_root = root
        self._mavlink_messages = None

    def get_mavlink_messages(self) -> list[MAVLinkMessage]:
        if self._mavlink_messages is None:
            self._mavlink_messages = get_all_messages(self._project_root)
        return self._mavlink_messages

    def analyze_document(self, document) -> list[types.Diagnostic]:
        if not self._is_cpp_file(document.uri):
            return []
        return [*check_vehicle_null_safety(document), *check_parameter_access(document)]

    def is_cpp_file(self, uri: str) -> bool:
        return self._is_cpp_file(uri)

    def is_fact_json_file(self, uri: str) -> bool:
        return self._is_fact_json_file(uri)

    def _is_cpp_file(self, uri: str) -> bool:
        path = uri.lower()
        return any(path.endswith(ext) for ext in self._CPP_EXTS)

    def _is_fact_json_file(self, uri: str) -> bool:
        path = uri.lower()
        if path.endswith("fact.json") or path.endswith(".factmetadata.json"):
            return True
        return "/factgroups/" in path and path.endswith(".json")


server = QGCLanguageServer()


@server.feature(types.INITIALIZED)
def initialized(ls: QGCLanguageServer, params: types.InitializedParams) -> None:
    """Resolve workspace root for MAVLink XML loading and Fact go-to-definition."""
    root: Path | None = None
    if ls.workspace.root_path:
        root = Path(ls.workspace.root_path).resolve()
    elif ls.workspace.root_uri:
        from urllib.parse import unquote, urlparse
        parsed = urlparse(ls.workspace.root_uri)
        root = Path(unquote(parsed.path)).resolve()
    if root:
        ls.set_project_root(root)
        logger.info(f"Set project root to: {root}")


def _publish(ls: QGCLanguageServer, doc, diagnostics: list[types.Diagnostic], include_version: bool = True) -> None:
    ls.diagnostics[doc.uri] = (doc.version, diagnostics)
    params = types.PublishDiagnosticsParams(
        uri=doc.uri,
        version=doc.version if include_version else None,
        diagnostics=diagnostics,
    )
    ls.text_document_publish_diagnostics(params)


@server.feature(types.TEXT_DOCUMENT_DID_OPEN)
def did_open(ls: QGCLanguageServer, params: types.DidOpenTextDocumentParams) -> None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    diagnostics = ls.analyze_document(doc)
    _publish(ls, doc, diagnostics)
    logger.info(f"Analyzed {doc.uri}: {len(diagnostics)} diagnostics")


@server.feature(types.TEXT_DOCUMENT_DID_CHANGE)
def did_change(ls: QGCLanguageServer, params: types.DidChangeTextDocumentParams) -> None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    _publish(ls, doc, ls.analyze_document(doc))


@server.feature(types.TEXT_DOCUMENT_DID_CLOSE)
def did_close(ls: QGCLanguageServer, params: types.DidCloseTextDocumentParams) -> None:
    uri = params.text_document.uri
    ls.diagnostics.pop(uri, None)
    ls.text_document_publish_diagnostics(
        types.PublishDiagnosticsParams(uri=uri, diagnostics=[])
    )


@server.feature(types.TEXT_DOCUMENT_DID_SAVE)
def did_save(ls: QGCLanguageServer, params: types.DidSaveTextDocumentParams) -> None:
    doc = ls.workspace.get_text_document(params.text_document.uri)
    _publish(ls, doc, ls.analyze_document(doc), include_version=False)


@server.feature(
    types.TEXT_DOCUMENT_COMPLETION,
    types.CompletionOptions(
        trigger_characters=["_", "M", "m", '"', ":"],
        resolve_provider=True,
    ),
)
def completion(ls: QGCLanguageServer, params: types.CompletionParams) -> types.CompletionList:
    """Dispatch a completion request to the right per-file-type provider."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    line_num = params.position.line
    char_pos = params.position.character
    lines = doc.lines

    if line_num >= len(lines):
        return types.CompletionList(is_incomplete=False, items=[])

    line = lines[line_num]
    prefix_text = line[:char_pos]

    if ls.is_fact_json_file(doc.uri):
        items = get_fact_json_completions(lines, line_num, char_pos, prefix_text)
        if items:
            logger.debug(f"Fact JSON completion: {len(items)} items")
        return types.CompletionList(is_incomplete=False, items=items)

    if not ls.is_cpp_file(doc.uri):
        return types.CompletionList(is_incomplete=False, items=[])

    messages = ls.get_mavlink_messages()

    msg_id_match = re.search(r"MAVLINK_MSG_ID_(\w*)$", prefix_text, re.IGNORECASE)
    if msg_id_match:
        filter_text = msg_id_match.group(1).upper()
        items = get_message_id_completions(filter_text, messages)
        return types.CompletionList(is_incomplete=False, items=items)

    decode_match = re.search(r"mavlink_msg_(\w*)$", prefix_text, re.IGNORECASE)
    if decode_match:
        filter_text = decode_match.group(1).lower()
        items = get_decode_completions(filter_text, messages)
        return types.CompletionList(is_incomplete=False, items=items)

    if is_in_switch_context(lines, line_num, char_pos):
        case_id_match = re.search(r"case\s+MAVLINK_MSG_ID_(\w*)$", prefix_text, re.IGNORECASE)
        if case_id_match:
            items = get_message_id_completions(case_id_match.group(1).upper(), messages, include_case=True)
        elif prefix_text.strip().endswith("case") or prefix_text.strip() == "":
            items = get_case_completions(messages)
        else:
            items = []
        return types.CompletionList(is_incomplete=False, items=items)

    return types.CompletionList(is_incomplete=False, items=[])


@server.feature(types.COMPLETION_ITEM_RESOLVE)
def completion_resolve(ls: QGCLanguageServer, item: types.CompletionItem) -> types.CompletionItem:
    """Attach hover-style markdown to a previously-suggested MAVLink completion item."""
    if not isinstance(item.data, dict):
        return item
    msg_name = item.data.get("message")
    if not msg_name:
        return item
    msg = next((m for m in ls.get_mavlink_messages() if m.name == msg_name), None)
    if not msg:
        return item

    doc_lines = [f"**{msg.name}** (ID: {msg.id})", "", msg.description, "", "**Fields:**"]
    for field in msg.fields:
        line = f"- `{field.name}`: {field.type}"
        if field.units:
            line += f" ({field.units})"
        if field.description:
            line += f" — {field.description}"
        doc_lines.append(line)
    item.documentation = types.MarkupContent(
        kind=types.MarkupKind.Markdown,
        value="\n".join(doc_lines),
    )
    return item


@server.feature(types.TEXT_DOCUMENT_DEFINITION)
def definition(ls: QGCLanguageServer, params: types.DefinitionParams) -> types.Location | None:
    """Resolve a Fact reference to its JSON definition."""
    doc = ls.workspace.get_text_document(params.text_document.uri)
    if not ls.is_cpp_file(doc.uri):
        return None

    line_num = params.position.line
    char_pos = params.position.character
    lines = doc.lines
    if line_num >= len(lines):
        return None

    line = lines[line_num]
    word_start = char_pos
    word_end = char_pos
    while word_start > 0 and (line[word_start - 1].isalnum() or line[word_start - 1] == "_"):
        word_start -= 1
    while word_end < len(line) and (line[word_end].isalnum() or line[word_end] == "_"):
        word_end += 1
    word = line[word_start:word_end]

    fact_name = extract_fact_name(word, line, word_start, word_end)
    if not fact_name:
        return None

    location = find_fact_definition(ls.project_root, fact_name)
    if location:
        logger.info(f"Found Fact definition for '{fact_name}' at {location.uri}")
    return location


def main() -> None:
    parser = argparse.ArgumentParser(description="QGroundControl Language Server")
    parser.add_argument("--tcp", action="store_true", help="Run in TCP mode (default: STDIO)")
    parser.add_argument("--host", default="127.0.0.1", help="TCP host (default: 127.0.0.1)")
    parser.add_argument("--port", type=int, default=2087, help="TCP port (default: 2087)")
    parser.add_argument("--verbose", "-v", action="store_true", help="Enable verbose logging")
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
