#!/usr/bin/env python3
"""
QGroundControl Language Server

Provides IDE integration for QGC-specific patterns:
- Vehicle null-check diagnostics
- Fact usage validation
- MAVLink message completion (planned)

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
