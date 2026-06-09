"""Per-document diagnostic checks for the QGC language server.

The on-disk equivalents in `tools/analyzers/vehicle_null_check.py` operate
on file paths and emit a project-wide report. The LSP needs incremental,
TextDocument-based scanning with LSP Diagnostic output, so the two stay
separate — duplicated *intent*, not duplicated *code*.
"""

from __future__ import annotations

import re

from lsprotocol import types
from pygls.workspace import TextDocument  # noqa: TC002

_ACTIVE_VEHICLE_PATTERN = re.compile(
    r"(\w+)\s*=\s*"
    r"(?:MultiVehicleManager::instance\(\)|_vehicleManager"
    r"|qgcApp\(\)->toolbox\(\)->multiVehicleManager\(\))->activeVehicle\(\)"
)
_GET_PARAM_PATTERN = re.compile(
    r"(\w+)\s*=\s*\w+->parameterManager\(\)->getParameter\s*\([^)]+\)"
)


def check_vehicle_null_safety(document: TextDocument) -> list[types.Diagnostic]:
    """Flag activeVehicle() dereferences without a preceding null check."""
    diagnostics: list[types.Diagnostic] = []
    lines = document.lines
    vehicle_vars: dict[str, int] = {}
    null_checked_vars: set[str] = set()

    for idx, line in enumerate(lines):
        match = _ACTIVE_VEHICLE_PATTERN.search(line)
        if match:
            var_name = match.group(1)
            vehicle_vars[var_name] = idx
            null_checked_vars.discard(var_name)

        for var_name in vehicle_vars:
            if re.search(
                rf"\bif\s*\(\s*!?\s*{re.escape(var_name)}\s*"
                r"(?:==\s*nullptr|!=\s*nullptr|\))",
                line,
            ):
                null_checked_vars.add(var_name)
            if re.search(
                rf"if\s*\(\s*!{re.escape(var_name)}\s*\)\s*(?:return|continue|break)",
                line,
            ):
                null_checked_vars.add(var_name)

        for var_name, decl_line in vehicle_vars.items():
            if var_name in null_checked_vars or idx <= decl_line:
                continue
            deref = re.search(rf"\b{re.escape(var_name)}->", line)
            if not deref:
                continue
            col = deref.start()
            diagnostics.append(
                types.Diagnostic(
                    message=(
                        f"Potential null pointer dereference: '{var_name}' from "
                        "activeVehicle() used without null check"
                    ),
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
                                    end=types.Position(
                                        line=decl_line, character=len(lines[decl_line])
                                    ),
                                ),
                            ),
                            message=f"'{var_name}' assigned here",
                        )
                    ],
                )
            )
            null_checked_vars.add(var_name)  # one report per var per scope

    return diagnostics


def check_parameter_access(document: TextDocument) -> list[types.Diagnostic]:
    """Flag getParameter() dereferences without a preceding null check."""
    diagnostics: list[types.Diagnostic] = []
    lines = document.lines
    param_vars: dict[str, int] = {}
    null_checked_vars: set[str] = set()

    for idx, line in enumerate(lines):
        match = _GET_PARAM_PATTERN.search(line)
        if match:
            param_vars[match.group(1)] = idx

        for var_name in param_vars:
            if re.search(
                rf"\bif\s*\(\s*!?\s*{re.escape(var_name)}\s*"
                r"(?:==\s*nullptr|!=\s*nullptr|\))",
                line,
            ):
                null_checked_vars.add(var_name)

        for var_name, decl_line in param_vars.items():
            if var_name in null_checked_vars or idx <= decl_line:
                continue
            deref = re.search(rf"\b{re.escape(var_name)}->", line)
            if not deref:
                continue
            col = deref.start()
            diagnostics.append(
                types.Diagnostic(
                    message=(
                        f"Potential null pointer dereference: '{var_name}' from "
                        "getParameter() used without null check"
                    ),
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
