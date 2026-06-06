#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
"""
qt_qml_lint.py -- Data-driven single-pass Qt6 QML linter.

Rules are defined as entries in typed tables (RULES_SIMPLE,
RULES_CONTEXT, RULES_FLAG) processed by a generic dispatch loop.
Block-level and ordering rules use a lightweight brace-tracking
state machine. No external dependencies -- Python 3.6+ only.

Rule categories:
  IMP  -- Import hygiene
  ORD  -- QML attribute ordering conventions
  BND  -- Binding & property patterns
  LAY  -- Layout & anchoring correctness
  LDR  -- Loader & dynamic object creation
  DEL  -- ListView & delegate patterns
  STA  -- States & transitions
  IMG  -- Image best practices
  PRF  -- Performance & rendering
  STY  -- Style & naming conventions
  SIG  -- Signal & connection patterns
  JS   -- JavaScript quality

Usage:
    python qt_qml_lint.py <file1.qml> [file2.qml ...]
    python qt_qml_lint.py --files-from=- < filelist.txt
    python qt_qml_lint.py --json <files...>

Output: FILE:LINE RULE-ID MESSAGE (one per line)
        With --json: JSON array to stdout.
Exit code: 0 if no findings, 1 if findings found.

Known limitations:
  - No full QML parser; multiline expressions or QML-like
    patterns inside string literals can cause false positives.
  - Block tracking uses brace counting; unbalanced braces in
    strings or block comments can desync the tracker.
  - Comment stripping is line-level (// only); interior lines
    of /* */ blocks that don't start with * are linted as code.
  - Cannot resolve types across files (e.g. whether a custom
    component's parent is a Layout).
  - Delegate detection is heuristic (looks for delegate:,
    model. context, or ListView/Repeater parent).
"""

from __future__ import annotations

import json
import re
import sys
from dataclasses import dataclass, field, asdict
from pathlib import Path
from typing import List, Dict, Optional, Set, Tuple


# ---------------------------------------------------------------------------
# Data model
# ---------------------------------------------------------------------------

@dataclass
class Finding:
    file: str
    line: int
    rule: str
    message: str

    def __str__(self) -> str:
        return f"{self.file}:{self.line} {self.rule} {self.message}"


@dataclass
class Rule:
    """A single lint rule processed by the generic dispatch loop.

    Tier A (simple):  pattern + optional exclude
    Tier B (context): pattern + context_before/after + context_pattern
    Tier C (flag):    pattern + requires_flag / requires_no_flag
    """
    id: str
    pattern: "re.Pattern[str]"
    message: str
    exclude: "Optional[re.Pattern[str]]" = None
    # Context checking (Tier B)
    context_before: int = 0
    context_after: int = 0
    context_pattern: "Optional[re.Pattern[str]]" = None
    context_must_match: bool = True  # True = context must match to fire
    # File-level flag gating (Tier C)
    requires_flag: Optional[str] = None
    requires_no_flag: Optional[str] = None


# ---------------------------------------------------------------------------
# Block tracker for structural rules (ORD, LAY, IMG, PRF, STY)
# ---------------------------------------------------------------------------

# Line classification categories, in expected order.
# ORD rules check that lines appear in non-decreasing category order.
CAT_ID = 0
CAT_PROP_DECL = 1       # property type name / required property
CAT_SIGNAL_DECL = 2     # signal name()
CAT_PROP_ASSIGN = 3     # name: value
CAT_ATTACHED = 4        # Name.prop: value (Layout.*, Drag.*, etc.)
CAT_STATES = 5          # states: [...]
CAT_TRANSITIONS = 6     # transitions: [...]
CAT_HANDLER = 7         # onFoo: / Component.onCompleted:
CAT_CHILD = 8           # Type { (starts a new block)
CAT_FUNCTION = 9        # function name()
CAT_UNKNOWN = 99


@dataclass
class ImportTracker:
    """Tracks import statements for IMP-1, IMP-3, IMP-4, IMP-6 checks."""
    imports_seen: "Dict[str, int]" = field(default_factory=dict)
    qt_import_lines: "List[int]" = field(default_factory=list)
    other_import_lines: "List[int]" = field(default_factory=list)

    def process_line(self, lineno: int, stripped: str, emit) -> None:
        """Check for duplicate imports (IMP-6), track Qt vs other (IMP-4)."""
        imp_match = RE_IMPORT_LINE.match(stripped)
        if not imp_match:
            return
        imp_text = imp_match.group(1).strip()
        # IMP-6: duplicate import
        if imp_text in self.imports_seen:
            emit(lineno, "IMP-6",
                 f"Duplicate import -- already imported at line "
                 f"{self.imports_seen[imp_text]}")
        else:
            self.imports_seen[imp_text] = lineno

        # Track Qt vs other imports for IMP-4
        if re.match(r'Qt\w*', imp_text):
            self.qt_import_lines.append(lineno)
        else:
            self.other_import_lines.append(lineno)

    def post_scan_checks(
        self,
        lines: "List[str]",
        file_flags: "Dict[str, bool]",
        emit,
    ) -> None:
        """Emit IMP-1, IMP-3, IMP-4 after the line-by-line scan."""
        # IMP-1: Redundant QtQuick.Window
        if file_flags["has_import_qtquick"]:
            for j, ln in enumerate(lines):
                if RE_IMP_QTQUICK_WINDOW.match(ln.strip()):
                    emit(j + 1, "IMP-1",
                         "import QtQuick.Window is redundant when "
                         "QtQuick is imported (Qt 6)")
                    break

        # IMP-3: Plain Controls import with customization
        if (file_flags["has_controls_plain"]
                and file_flags["has_control_custom"]):
            for j, ln in enumerate(lines):
                if RE_IMP_CONTROLS_PLAIN.match(ln.strip()):
                    emit(j + 1, "IMP-3",
                         "import QtQuick.Controls without style qualifier "
                         "-- use Controls.Basic (or specific style) when "
                         "customizing contentItem/background/indicator/"
                         "handle")
                    break

        # IMP-4: Import ordering (Qt imports should come before third-party)
        if self.other_import_lines and self.qt_import_lines:
            first_other = min(self.other_import_lines)
            last_qt = max(self.qt_import_lines)
            if first_other < last_qt:
                emit(first_other, "IMP-4",
                     "Non-Qt import before Qt import -- "
                     "order: Qt modules, third-party, local C++, "
                     "QML folders")


@dataclass
class Block:
    """Tracks a single QML object block between { and }."""
    type_name: str              # e.g. "Rectangle", "Image", "RowLayout"
    start_line: int
    has_id: bool = False
    id_value: str = ""
    properties: "Dict[str, int]" = field(default_factory=dict)
    # For ordering checks: list of (category, line_number)
    categories: "List[Tuple[int, int]]" = field(default_factory=list)
    # Parent type (for Layout child detection)
    parent_type: str = ""


@dataclass
class BlockTracker:
    """Tracks QML block nesting, brace depth, and structural sub-state-machines."""
    block_stack: "List[Block]" = field(default_factory=list)
    brace_depth: int = 0
    root_block_seen: bool = False
    root_block_has_id_root: bool = False
    # PropertyChanges sub-state (STA-1, STA-4)
    in_property_changes: bool = False
    property_changes_depth: int = 0
    # Connections sub-state (SIG-1, SIG-2, SIG-3)
    in_connections: "List[dict]" = field(default_factory=list)
    # onCompleted sub-state (DEL-3)
    in_on_completed: bool = False
    on_completed_brace_depth: int = 0
    # Delegate context heuristic
    is_delegate_file: bool = False

    def process_line(
        self,
        lineno: int,
        stripped: str,
        code_line: str,
        emit,
        file_flags: "Dict[str, bool]",
    ) -> None:
        """Update block state for one line. Emits structural findings."""
        open_braces = code_line.count('{')
        close_braces = code_line.count('}')

        # Determine if this line opens a new block
        line_opens_block = False
        type_name = ""
        if open_braces > 0:
            type_match = re.match(r'^\s*(\w[\w.]*)\s*\{', stripped)
            type_name = type_match.group(1) if type_match else ""
            line_opens_block = bool(type_match)

        # If this line opens a block, classify it as CAT_CHILD in
        # the PARENT block before pushing the new child block.
        # Exception: State{} and Transition{} inside states:/transitions:
        # arrays are part of those properties, not standalone children.
        if line_opens_block and self.block_stack:
            if type_name not in SKIP_CHILD_TYPES:
                self.block_stack[-1].categories.append(
                    (CAT_CHILD, lineno)
                )

        # Push new blocks
        if open_braces > 0:
            parent_type = ""
            if self.block_stack:
                parent_type = self.block_stack[-1].type_name

            for bi in range(open_braces):
                block = Block(
                    type_name=type_name if bi == 0 else "",
                    start_line=lineno,
                    parent_type=parent_type,
                )
                self.block_stack.append(block)

            # Track if this is the root block
            if self.brace_depth == 0 and open_braces > 0:
                self.root_block_seen = True

            # PropertyChanges tracking for STA-1
            if "PropertyChanges" in stripped:
                self.in_property_changes = True
                self.property_changes_depth = self.brace_depth + 1

            # Connections tracking for SIG-1/2/3
            if RE_SIG_CONNECTIONS.search(stripped):
                self.in_connections.append({
                    "start": lineno,
                    "has_target": False,
                    "has_old_handler": False,
                    "has_new_handler": False,
                    "depth": self.brace_depth + 1,
                })

        # Classify line for current block (if inside one).
        # Skip classification if this line opened the block (already
        # handled as CAT_CHILD in parent above).
        if self.block_stack and not line_opens_block:
            current = self.block_stack[-1]
            cat = classify_line(stripped)

            # Track id
            id_m = RE_STY_ID.match(stripped)
            if id_m:
                current.has_id = True
                current.id_value = id_m.group(1)

            # Track properties for block-level checks
            prop_m = re.match(r'^\s*(\w+(?:\.\w+)*)\s*:', stripped)
            if prop_m:
                prop_key = prop_m.group(1)
                if prop_key not in current.properties:
                    current.properties[prop_key] = lineno

            # Ordering tracking (only for non-unknown categories)
            if cat != CAT_UNKNOWN:
                current.categories.append((cat, lineno))

            # STA-1: target: inside PropertyChanges
            if (self.in_property_changes
                    and RE_STA_TARGET_LINE.match(stripped)
                    and self.brace_depth >= self.property_changes_depth):
                emit(lineno, "STA-1",
                     "PropertyChanges uses target: -- "
                     "in Qt 6 use id.property: value syntax")

            # STA-4: imperative = inside PropertyChanges
            if (self.in_property_changes
                    and self.brace_depth >= self.property_changes_depth
                    and RE_BND_IMPERATIVE_ASSIGN.search(code_line)
                    and not RE_BND_IMPERATIVE_EXCLUDE.search(
                        code_line)):
                emit(lineno, "STA-4",
                     "Imperative '=' inside PropertyChanges -- "
                     "use declarative ':' binding syntax")

            # Connections tracking
            if self.in_connections:
                conn = self.in_connections[-1]
                if re.match(r'^\s*target\s*:', stripped):
                    conn["has_target"] = True
                if RE_SIG_OLD_HANDLER.match(stripped):
                    conn["has_old_handler"] = True
                if RE_SIG_NEW_HANDLER.match(stripped):
                    conn["has_new_handler"] = True

        # DEL-3: connect() inside Component.onCompleted
        if RE_DEL_ON_COMPLETED.search(stripped):
            self.in_on_completed = True
            self.on_completed_brace_depth = self.brace_depth
        if self.in_on_completed:
            if RE_DEL_CONNECT.search(code_line) and self.is_delegate_file:
                emit(lineno, "DEL-3",
                     "connect() in Component.onCompleted inside delegate "
                     "-- use Connections{} object (destroyed with delegate)")

        # STY-1: root block id check
        # brace_depth is still pre-update here, so on the line that
        # opens the root block (depth 0 -> 1), check depth 0 + braces.
        effective_depth = self.brace_depth + open_braces - close_braces
        if (effective_depth == 1 or self.brace_depth == 1) \
                and RE_STY_ID_ROOT.match(stripped):
            self.root_block_has_id_root = True

        # Update brace depth
        self.brace_depth += open_braces - close_braces

        # Close blocks
        if close_braces > 0:
            for _ in range(close_braces):
                if self.block_stack:
                    closed = self.block_stack.pop()
                    _check_closed_block(closed, emit, file_flags)

            # End PropertyChanges tracking
            if (self.in_property_changes
                    and self.brace_depth < self.property_changes_depth):
                self.in_property_changes = False

            # End Connections tracking
            while (self.in_connections
                    and self.brace_depth
                    < self.in_connections[-1]["depth"]):
                conn = self.in_connections.pop()
                # SIG-1: no target
                if not conn["has_target"]:
                    emit(conn["start"], "SIG-1",
                         "Connections without explicit target -- "
                         "default is parent, which causes "
                         "unintended handling")
                # SIG-3: mixed handler syntax
                if (conn["has_old_handler"]
                        and conn["has_new_handler"]):
                    emit(conn["start"], "SIG-3",
                         "Connections mixes onFoo: and "
                         "function onFoo() -- function handlers "
                         "silently ignored; use one style "
                         "consistently")
                # SIG-2: old handler syntax
                elif conn["has_old_handler"]:
                    emit(conn["start"], "SIG-2",
                         "Connections uses deprecated onFoo: "
                         "handler syntax -- use "
                         "function onFoo() {} "
                         "(deprecated since Qt 5.15)")

            # End onCompleted tracking
            if (self.in_on_completed
                    and self.brace_depth
                    <= self.on_completed_brace_depth):
                self.in_on_completed = False


def classify_line(line: str) -> int:
    """Classify a stripped, non-comment QML line into an ordering category."""
    s = line.strip()
    if not s or s.startswith("//") or s.startswith("/*") or s.startswith("*"):
        return CAT_UNKNOWN

    # id: must be first
    if re.match(r'^id\s*:', s):
        return CAT_ID

    # required property / property declarations
    if re.match(r'^(required\s+)?(default\s+)?property\s+', s):
        return CAT_PROP_DECL

    # signal declarations
    if re.match(r'^signal\s+\w+', s):
        return CAT_SIGNAL_DECL

    # states: and transitions:
    if re.match(r'^states\s*:', s):
        return CAT_STATES
    if re.match(r'^transitions\s*:', s):
        return CAT_TRANSITIONS

    # function declarations
    if re.match(r'^function\s+\w+', s):
        return CAT_FUNCTION

    # Signal handlers: onFoo: or Component.onFoo:
    if re.match(r'^(on[A-Z]\w*|\w+\.on[A-Z]\w*)\s*:', s):
        return CAT_HANDLER

    # Attached properties: Name.prop: (capital letter start, has dot)
    if re.match(r'^[A-Z]\w*\.\w+\s*:', s):
        return CAT_ATTACHED

    # Child object: Type { (capital letter start, ends with or contains {)
    if re.match(r'^[A-Z]\w*(\.\w+)?\s*\{', s):
        return CAT_CHILD
    # component declaration: component Name: Type {
    if re.match(r'^component\s+\w+\s*:', s):
        return CAT_CHILD

    # Property assignments: name: value (lowercase start)
    if re.match(r'^[a-z]\w*(\.\w+)*\s*:', s):
        return CAT_PROP_ASSIGN

    return CAT_UNKNOWN


# Layout types for LAY-2/LAY-6 detection
LAYOUT_TYPES = {"RowLayout", "ColumnLayout", "GridLayout"}

# Types that are part of states/transitions arrays, not standalone
# children. These should not be classified as CAT_CHILD in the
# parent block's ordering check.
SKIP_CHILD_TYPES = {
    "State", "Transition", "PropertyChanges",
    "PropertyAnimation", "NumberAnimation",
    "ColorAnimation", "SequentialAnimation",
    "ParallelAnimation", "PauseAnimation",
    "ScriptAction", "PropertyAction",
}

# Types where standard QML attribute ordering does not apply.
# Connections has target: + function handlers; Behavior/Binding
# have property-specific internal structure; animation and state
# types have their own conventions.
SKIP_ORD_TYPES = SKIP_CHILD_TYPES | {
    "Connections", "Behavior", "Binding",
}


# ---------------------------------------------------------------------------
# Compiled regex patterns
# ---------------------------------------------------------------------------

# Comment line detection (same approach as C++ linter)
RE_COMMENT_LINE = re.compile(r"^\s*(//|/?\*)")

# --- IMP (Imports) ---

# IMP-1: Redundant QtQuick.Window when QtQuick is imported (Qt 6)
# Research: In Qt 6, Window types were folded into QtQuick module.
# QtQuick.Window import is unnecessary overhead and confuses newcomers.
RE_IMP_QTQUICK = re.compile(r'^\s*import\s+QtQuick(\s+\d[\d.]*)?$', re.M)
RE_IMP_QTQUICK_WINDOW = re.compile(r'^\s*import\s+QtQuick\.Window\b')

# IMP-2: Version numbers on imports (Qt 6 dropped the requirement).
# Research: qmlsc requires unversioned imports for optimal compilation;
# versioned imports cap API surface and cause "missing type" confusion
# when copying Qt 5 examples.
RE_IMP_VERSIONED = re.compile(r'^\s*import\s+\w[\w.]*\s+\d+\.\d+')

# IMP-3: Plain QtQuick.Controls without style qualifier when customizing.
# Research: Customizing contentItem/background/indicator/handle without a
# style-specific import uses the "default style" abstraction, which can
# produce unexpected rendering. Import Controls.Basic (or another style).
RE_IMP_CONTROLS_PLAIN = re.compile(
    r'^\s*import\s+QtQuick\.Controls(\s+\d[\d.]*)?\s*$', re.M
)
RE_CONTROL_CUSTOMIZATION = re.compile(
    r'\b(contentItem|background|indicator|handle)\s*:'
)

# IMP-5: Qt.include() is deprecated since Qt 5.14.
# Research: Removed in Qt 6 documentation; use explicit imports or
# JS module imports instead.
RE_IMP_QT_INCLUDE = re.compile(r'\bQt\.include\s*\(')

# IMP-6: Duplicate import detection (post-scan)
RE_IMPORT_LINE = re.compile(r'^\s*import\s+(.+)')

# --- BND (Bindings & Properties) ---

# BND-1: property var usage. Research: qmllint warns via
# prefer-non-var-properties; typed properties enable qmlsc compilation
# to C++ and eliminate meta-object overhead in property access.
RE_BND_PROP_VAR = re.compile(r'^\s*(required\s+)?property\s+var\s+\w+')

# BND-2: Imperative = on property that likely had a binding.
# Research: any `prop = value` in a JS block destroys the binding
# permanently. The qt.qml.binding.removal logging category (Qt 5.10+)
# is the only runtime diagnostic. qmllint does NOT detect this.
# Two patterns: bare `prop = value` and qualified `id.prop = value`.
RE_BND_IMPERATIVE_ASSIGN = re.compile(
    r'^\s+(\w+)\s*=\s*(?!==)'  # assignment in JS (indented, not ==)
)
RE_BND_QUALIFIED_ASSIGN = re.compile(
    r'^\s+(\w+)\.(\w+)\s*=\s*(?!==)'  # id.prop = value
)
RE_BND_IMPERATIVE_EXCLUDE = re.compile(
    r'(var |let |const |function |return |for |if |else |while |'
    r'switch |case |property |signal |import |//|readonly )'
)

# BND-3: Qt.binding with old-style function (not arrow).
# Research: arrow functions are cleaner and the recommended syntax
# per Qt docs. Old-style function(){} has `this` context issues
# inside Qt.binding().
RE_BND_BINDING_OLDFUNC = re.compile(r'Qt\.binding\s*\(\s*function\s*\(')

# BND-5: list<> property type warning.
# Research: QML list properties have no granular change signals for
# add/move/remove operations -- only whole-list replacement triggers
# notification. Binding expensive operations to list properties
# causes subtle update bugs.
RE_BND_LIST_PROP = re.compile(r'property\s+list\s*<')

# --- LAY (Layout & Anchoring) ---

# LAY-1: anchors + Layout on same item (detected in block tracker).
# LAY-3: Four anchor edges instead of fill (detected in block tracker).

# LAY-5: Cross-branch anchoring via parent.parent.
# Research: anchoring to parent.parent references a grandparent,
# which is fragile if the visual tree is refactored. Use an explicit
# id on the target instead.
RE_LAY_PARENT_PARENT = re.compile(r'anchors\.\w+\s*:\s*parent\.parent\b')

# LAY-2 / LAY-6: bare width/height or x/y inside Layout child
# (detected in block tracker by checking parent_type).

# --- LDR (Loader & Dynamic Creation) ---

# LDR-1: Loader.item access without status guard.
# Research: with asynchronous:true, Loader.item is null until
# status===Loader.Ready. Binding to Loader.item.prop without a
# null guard causes TypeError. qmllint does not catch this.
RE_LDR_ITEM_ACCESS = re.compile(r'\bloader\w*\.item\b', re.IGNORECASE)
RE_LDR_STATUS_GUARD = re.compile(
    r'(status\s*===?\s*Loader\.Ready|Loader\.status|\.item\?\.|\bonLoaded\b)'
)

# LDR-2: Qt.createComponent with string URL.
# Research: String-based createComponent loses tooling support and
# type checking. Prefer inline Component{} definitions.
RE_LDR_CREATE_COMPONENT = re.compile(r'Qt\.createComponent\s*\(')

# LDR-3: Qt.createQmlObject is slow and uncacheable.
# Research: parses QML string at runtime on every call; no component
# caching. Use Loader or createComponent for all non-trivial cases.
RE_LDR_CREATE_OBJECT = re.compile(r'Qt\.createQmlObject\s*\(')

# LDR-5: Loader with both source and sourceComponent.
# Research: Qt docs state these are mutually exclusive; setting both
# is unsupported and behavior is undefined.
RE_LDR_SOURCE = re.compile(r'^\s*source\s*:')
RE_LDR_SOURCE_COMPONENT = re.compile(r'^\s*sourceComponent\s*:')

# --- DEL (ListView & Delegates) ---

# DEL-1: Delegate using model.roleName without required property.
# Research: Once any required property is declared, the implicit
# model context object is no longer injected. But using model.*
# without required properties misses qmlsc compilation and type
# safety. Modern Qt 6 best practice is required properties.
RE_DEL_MODEL_ACCESS = re.compile(r'\bmodel\.\w+')
RE_DEL_REQUIRED_PROP = re.compile(r'required\s+property\b')

# DEL-2: var declaration in delegate (mutable JS state that won't
# reset on delegate reuse).
# Research: with reuseItems:true, Component.onCompleted doesn't
# re-fire. JS vars keep their old values, causing state bleed.
# Use QML properties instead (they get model-bound on reuse).
RE_DEL_VAR_IN_JS = re.compile(r'\bvar\s+\w+\s*=')

# DEL-3: connect() in Component.onCompleted (survives delegate destruction).
# Research: Direct connect() creates signal connections that outlive
# the delegate, causing TypeError when the signal fires after the
# delegate is destroyed. Use Connections{} objects instead.
RE_DEL_CONNECT = re.compile(r'\.connect\s*\(')
RE_DEL_ON_COMPLETED = re.compile(r'Component\.onCompleted\s*:')

# DEL-4: Component.onCompleted in delegate with reuseItems.
# Research: onCompleted fires once at creation, NOT on reuse.
# State initialization that belongs in onReused will be missed.
RE_DEL_REUSE_ITEMS = re.compile(r'reuseItems\s*:\s*true')

# --- STA (States & Transitions) ---

# STA-1: PropertyChanges with target: property (Qt 6 deprecated form).
# Research: Qt 6 uses id.property: value syntax inside PropertyChanges.
# The old target: form still works but is not recommended and is
# incompatible with Qt Design Studio.
RE_STA_TARGET_LINE = re.compile(r'^\s*target\s*:')

# STA-2: Transition without from/to (catch-all).
# Research: Transition{from:"*";to:"*"} fires on every state change
# including unintended ones. Explicit from/to prevents unexpected
# animations when new states are added.
RE_STA_TRANSITION = re.compile(r'Transition\s*\{')
RE_STA_FROM_TO = re.compile(r'\b(from|to)\s*:')

# STA-3: Top-level states in reusable component (detected in block tracker).
# Research: QML states is a QQmlListProperty -- assigning from outside
# adds rather than replaces, causing conflicts. Use StateGroup for
# internal states of reusable components.
RE_STA_STATES = re.compile(r'^\s*states\s*:\s*\[')

# --- IMG (Images) ---

# IMG-1: Image without sourceSize (detected in block tracker).
# Research: Without sourceSize, Qt decodes the full-resolution image
# into GPU memory. A 4000x3000 photo displayed at 100x75 still
# allocates ~48MB of texture memory.

# IMG-2: Image with network source without asynchronous:true
# (detected in block tracker).
# Research: Image decoding blocks the UI thread by default. For
# network sources this means the entire download+decode is synchronous.

# --- PRF (Performance & Rendering) ---

# PRF-1: Rectangle with color:"transparent" (block tracker).
# Research: Qt docs explicitly recommend Item for grouping.
# Rectangle creates a scene graph geometry node even when transparent,
# adding to batch count. Item generates no geometry node.
# Matches both literal `color: "transparent"` and conditional
# expressions that include "transparent" (e.g., ternary).
# Also matches continuation lines where "transparent" appears
# without the `color:` prefix (multiline ternary).
RE_PRF_TRANSPARENT = re.compile(
    r'color\s*:.*["\']transparent["\']'
)
RE_PRF_TRANSPARENT_CONT = re.compile(
    r'["\']transparent["\']'
)

# PRF-2: opacity: 0 without animation context.
# Research: opacity:0 still incurs rendering overhead and retains
# keyboard focus. visible:false skips rendering entirely and removes
# from input handling. Use opacity:0 only during fade animations.
RE_PRF_OPACITY_ZERO = re.compile(r'^\s*opacity\s*:\s*0\s*$')
RE_PRF_OPACITY_ANIM = re.compile(
    r'(Behavior\s+on\s+opacity|NumberAnimation.*opacity|'
    r'OpacityAnimator|PropertyAnimation.*"opacity")'
)

# PRF-3: clip:true warning.
# Research: Qt docs: "Clipping is a visual effect, NOT an optimization."
# Forces a separate scene graph batch (scissor/stencil). Acceptable
# on ListView (many children) but costly on small items.
RE_PRF_CLIP = re.compile(r'^\s*clip\s*:\s*true')

# PRF-4: font.pixelSize bound to animation.
# Research: Every font.pixelSize change triggers full text relayout
# (glyph shaping, line breaking). Use scale transform on the Text
# element instead for size animations.
RE_PRF_FONT_ANIM = re.compile(
    r'Behavior\s+on\s+font\.pixelSize|'
    r'NumberAnimation\s*\{[^}]*property\s*:\s*"font\.pixelSize"'
)

# PRF-5: Text with RichText format.
# Research: RichText is significantly more expensive than PlainText
# or StyledText. It invokes a full HTML/CSS parser. Use PlainText
# unless rich formatting is required.
RE_PRF_RICHTEXT = re.compile(r'textFormat\s*:\s*Text\.RichText')

# PRF-6: layer.enabled:true without clear animation purpose.
# Research: Renders the subtree to an offscreen FBO, then composites
# as texture. The layered item cannot be batched with siblings.
# Multisampling on layers is especially expensive.
RE_PRF_LAYER = re.compile(r'^\s*layer\.enabled\s*:\s*true')

# --- STY (Style & Conventions) ---

# STY-1: Top-level component missing id:root (detected in block tracker).
# Research: The QML coding convention is that the root element of every
# file uses id:root. This enables qualified lookup (root.prop) and
# future-proofs against QML 3 unqualified lookup removal.
RE_STY_ID_ROOT = re.compile(r'^\s*id\s*:\s*root\b')

# STY-3: Multiple grouped properties using dot notation
# (detected in block tracker).
# Research: Qt style guide recommends group notation when setting
# multiple sub-properties of the same group (e.g. sourceSize {},
# anchors {}, font {}).

# STY-6: id not using camelCase.
# Research: QML convention is lowerCamelCase for ids. Under_score
# or UPPER ids break convention and confuse tooling.
RE_STY_ID = re.compile(r'^\s*id\s*:\s*(\w+)')
RE_STY_CAMELCASE = re.compile(r'^[a-z][a-zA-Z0-9]*$')

# --- SIG (Signals & Connections) ---

# SIG-1: Connections without explicit target.
# Research: Default target is parent, which causes unintended signal
# handling if the parent type changes. The coding instructions mandate
# always setting target explicitly.
RE_SIG_CONNECTIONS = re.compile(r'Connections\s*\{')

# SIG-2: Old onFoo: handler syntax in Connections block (deprecated 5.15).
# Research: The old syntax produces deprecation warnings in Qt 6.
# Worse: mixing old onFoo: with new function onFoo() in the same
# Connections block silently ignores the function-based handlers.
RE_SIG_OLD_HANDLER = re.compile(r'^\s*on[A-Z]\w*\s*:')
RE_SIG_NEW_HANDLER = re.compile(r'^\s*function\s+on[A-Z]\w*\s*\(')

# --- JS (JavaScript Quality) ---

# JS-1: var instead of let/const.
# Research: var has function scope and hoisting, causing subtle bugs.
# let/const have block scope. Qt coding instructions mandate
# let/const. qmlsc can optimize const better than var.
RE_JS_VAR = re.compile(r'(?<!\w)var\s+\w+\s*[=;]')
RE_JS_VAR_EXCLUDE = re.compile(
    r'(property\s+var|^\s*//|^\s*/?\*)'
)

# JS-2: Loose equality (== / !=) instead of strict (=== / !==).
# Research: QML's JS engine follows ECMAScript; loose equality
# performs type coercion which is almost never desired in QML
# property comparisons. qmllint has equality-type-coercion warning.
RE_JS_LOOSE_EQ = re.compile(r'(?<!=)\s*[!=]=(?!=)\s*(?!=)')
RE_JS_LOOSE_EXCLUDE = re.compile(
    r'(^\s*//|^\s*/?\*|^\s*\*|import |property |signal )'
)

# --- ERR (Error Handling & Security) ---

# ERR-1: Hardcoded http:// instead of https://.
# Research: Unencrypted HTTP exposes data in plaintext. Same rule
# as the C++ linter's ERR-4. Excludes localhost/127.0.0.1/example.
RE_ERR_HTTP = re.compile(r'"http://[a-zA-Z]')
RE_ERR_HTTP_EXCLUDE = re.compile(
    r'(localhost|127\.0\.0\.1|::1|example\.test)'
)

# ERR-2: Hardcoded Unix paths (non-portable).
# Research: /tmp/ does not exist on Windows. Qt provides
# QStandardPaths for cross-platform temporary file access.
RE_ERR_UNIX_PATH = re.compile(r'"/tmp/')

# JS-3: forbidden dynamic code execution.
# Research: blocks JIT compilation in QV4, is a security risk,
# and qmllint flags it. There is never a valid use case in QML.
RE_JS_EVAL = re.compile(r'\beval\s*\(')


# ---------------------------------------------------------------------------
# Rule tables
# ---------------------------------------------------------------------------

# Tier A: Simple match + optional exclude.
RULES_SIMPLE: "List[Rule]" = [
    # --- IMP ---
    Rule("IMP-2", RE_IMP_VERSIONED,
         "Versioned import -- Qt 6 does not require version numbers on imports"),
    Rule("IMP-5", RE_IMP_QT_INCLUDE,
         "Qt.include() is deprecated -- use ES module imports or explicit QML imports"),

    # --- BND ---
    Rule("BND-1", RE_BND_PROP_VAR,
         "property var -- use a typed property (int, string, etc.) for "
         "qmlsc compilation and type safety"),
    Rule("BND-3", RE_BND_BINDING_OLDFUNC,
         "Qt.binding(function(){}) -- use arrow function: Qt.binding(() => expr)"),
    Rule("BND-5", RE_BND_LIST_PROP,
         "list<> property has no granular change signals -- "
         "add/move/remove won't notify; consider a ListModel or notify manually"),

    # --- LAY ---
    Rule("LAY-5", RE_LAY_PARENT_PARENT,
         "Anchoring to parent.parent -- fragile cross-branch reference; "
         "use an explicit id on the target"),

    # --- LDR ---
    Rule("LDR-2", RE_LDR_CREATE_COMPONENT,
         "Qt.createComponent(url) -- prefer inline Component{} for "
         "type safety and tooling support"),
    Rule("LDR-3", RE_LDR_CREATE_OBJECT,
         "Qt.createQmlObject() -- slow and uncacheable; "
         "use Loader or Component.createObject()"),

    # --- PRF ---
    Rule("PRF-3", RE_PRF_CLIP,
         "clip: true disables scene graph batching -- "
         "verify this is needed (acceptable on ListView)"),
    Rule("PRF-5", RE_PRF_RICHTEXT,
         "Text.RichText invokes full HTML/CSS parser -- "
         "use PlainText or StyledText if possible"),
    Rule("PRF-4", RE_PRF_FONT_ANIM,
         "Animating font.pixelSize triggers full text relayout each frame -- "
         "use scale transform on Text instead"),
    Rule("PRF-6", RE_PRF_LAYER,
         "layer.enabled forces offscreen FBO rendering -- "
         "enable only during effects/animations, then disable"),

    # --- STY ---
    # STY-6 is pattern-matched here but handled specially inline
    Rule("STY-6", RE_STY_ID,
         ""),  # placeholder; actual check is inline

    # --- ERR ---
    # ERR-1 uses raw lines (http:// contains // which comment
    # stripping removes) -- handled inline, not here.
    Rule("ERR-2", RE_ERR_UNIX_PATH,
         "Hardcoded /tmp/ path is not cross-platform -- "
         "use QStandardPaths.writableLocation from C++ or a "
         "platform-aware helper"),

    # --- JS ---
    Rule("JS-3", RE_JS_EVAL,
         "Dynamic code execution blocks JIT and is a security risk -- "
         "never use in QML"),
]

# Tier B: Match + context window check.
RULES_CONTEXT: "List[Rule]" = [
    # LDR-1: Loader.item without status guard
    Rule("LDR-1", RE_LDR_ITEM_ACCESS,
         "Loader.item accessed without status guard -- "
         "check Loader.status === Loader.Ready or use optional chaining (?.)",
         context_before=5, context_after=5,
         context_pattern=RE_LDR_STATUS_GUARD,
         context_must_match=False),

    # STA-2: Transition without from/to
    Rule("STA-2", RE_STA_TRANSITION,
         "Transition without from/to -- catch-all fires on every state change; "
         "specify explicit from/to pairs",
         context_after=3, context_pattern=RE_STA_FROM_TO,
         context_must_match=False),
]

# Tier C: Match + file-level flag guard.
RULES_FLAG: "List[Rule]" = [
    # DEL-1: model. access without required property
    Rule("DEL-1", RE_DEL_MODEL_ACCESS,
         "model.roleName without required property -- "
         "declare required properties for type safety and qmlsc compilation",
         requires_no_flag="has_required_prop"),

    # PRF-2: opacity: 0 without animation context
    Rule("PRF-2", RE_PRF_OPACITY_ZERO,
         "opacity: 0 without opacity animation -- prefer visible: false "
         "(skips rendering entirely and removes from input handling)",
         requires_no_flag="has_opacity_anim"),
]


# ---------------------------------------------------------------------------
# Per-line rule dispatch
# ---------------------------------------------------------------------------

def _check_line_rules(
    lineno: int,
    i: int,
    line: str,
    stripped: str,
    code_line: str,
    code_lines: "List[str]",
    code_text: str,
    num_lines: int,
    brace_depth: int,
    is_delegate_file: bool,
    file_flags: "Dict[str, bool]",
    emit,
) -> None:
    """Run all per-line rule checks (Tiers A, B, C, and special rules)."""

    # --- Tier A: simple rules ---
    for rule in RULES_SIMPLE:
        if not rule.pattern.search(code_line):
            continue
        if rule.exclude and rule.exclude.search(code_line):
            continue
        # STY-6 special handling: check the actual id value
        if rule.id == "STY-6":
            m = RE_STY_ID.match(stripped)
            if m:
                id_val = m.group(1)
                if not RE_STY_CAMELCASE.match(id_val):
                    emit(lineno, "STY-6",
                         f"id '{id_val}' -- use lowerCamelCase for QML ids")
            continue
        emit(lineno, rule.id, rule.message)

    # --- Tier A special: multi-pattern rules ---

    # BND-2: Imperative assignment destroying binding (heuristic)
    # Pattern 1: bare `prop = value` inside handler/function
    if (RE_BND_IMPERATIVE_ASSIGN.search(code_line)
            and not RE_BND_IMPERATIVE_EXCLUDE.search(code_line)
            and brace_depth >= 2):  # inside a handler or function
        m = RE_BND_IMPERATIVE_ASSIGN.search(code_line)
        if m:
            prop_name = m.group(1)
            # Check if this property was bound with : in the file
            # (use code_text to skip commented-out bindings)
            if re.search(
                rf'^\s*{re.escape(prop_name)}\s*:',
                code_text, re.M
            ):
                emit(lineno, "BND-2",
                     f"Imperative '=' on '{prop_name}' destroys "
                     "its binding -- use Qt.binding() to restore,"
                     " or verify this is intentional")

    # Pattern 2: qualified `id.prop = value` (e.g., in
    # Connections handler: `syncLabel.text = "done"`)
    # Only flag when the target id exists and its property has
    # a binding to a non-literal expression (not just
    # `text: "static"`), since overwriting a static value is
    # intentional and harmless.
    if (RE_BND_QUALIFIED_ASSIGN.search(code_line)
            and not RE_BND_IMPERATIVE_EXCLUDE.search(code_line)
            and brace_depth >= 1):
        m = RE_BND_QUALIFIED_ASSIGN.search(code_line)
        if m:
            target_id = m.group(1)
            prop_name = m.group(2)
            # Verify target id exists in file
            if re.search(
                rf'\bid\s*:\s*{re.escape(target_id)}\b',
                code_text
            ):
                # Check if the property has a dynamic binding
                # (references another id/property, not just a
                # string/number literal). Look for `prop: <expr>`
                # where expr starts with a word char (property
                # reference) rather than a literal (" ' digit).
                binding_match = re.search(
                    rf'^\s*{re.escape(prop_name)}\s*:\s*'
                    rf'(?!["\'\d])'
                    rf'(?!true\b|false\b|null\b|undefined\b)'
                    rf'[a-zA-Z_]',
                    code_text, re.M
                )
                if binding_match:
                    emit(lineno, "BND-2",
                         f"Imperative '=' on "
                         f"'{target_id}.{prop_name}' destroys "
                         "its binding -- use Qt.binding() to "
                         "restore, or verify this is intentional"
                         )

    # ERR-1: Hardcoded http:// URL (uses raw line because //
    # in URLs is stripped by comment removal)
    if (RE_ERR_HTTP.search(line)
            and not RE_ERR_HTTP_EXCLUDE.search(line)):
        emit(lineno, "ERR-1",
             "Hardcoded http:// URL -- "
             "use https:// for secure transport")

    # JS-1: var in JS (not property var)
    if (RE_JS_VAR.search(code_line)
            and not RE_JS_VAR_EXCLUDE.search(code_line)):
        emit(lineno, "JS-1",
             "Use let/const instead of var -- "
             "var has function scope and hoisting bugs")

    # JS-2: Loose equality
    if (RE_JS_LOOSE_EQ.search(code_line)
            and not RE_JS_LOOSE_EXCLUDE.search(code_line)):
        emit(lineno, "JS-2",
             "Loose equality (==/!=) -- use strict equality (===/!==)")

    # DEL-2: mutable var in delegate with reuseItems
    if (RE_DEL_VAR_IN_JS.search(code_line)
            and not RE_JS_VAR_EXCLUDE.search(code_line)
            and is_delegate_file
            and file_flags["has_reuse_items"]):
        emit(lineno, "DEL-2",
             "var declaration in delegate with reuseItems -- "
             "JS vars don't reset on reuse; use QML properties "
             "or reset in ListView.onReused")

    # --- Tier B: context-aware rules ---
    for rule in RULES_CONTEXT:
        if not rule.pattern.search(code_line):
            continue
        if rule.exclude and rule.exclude.search(code_line):
            continue
        if rule.context_pattern is None:
            emit(lineno, rule.id, rule.message)
            continue
        # Build context window
        ctx_parts = []
        if rule.context_before:
            start = max(i - rule.context_before, 0)
            ctx_parts.append("\n".join(code_lines[start:i]))
        if rule.context_after:
            end = min(i + 1 + rule.context_after, num_lines)
            ctx_parts.append("\n".join(code_lines[i + 1:end]))
        ctx = "\n".join(ctx_parts)
        has_ctx = bool(rule.context_pattern.search(ctx))
        if has_ctx == rule.context_must_match:
            emit(lineno, rule.id, rule.message)

    # --- Tier C: file-flag-gated rules ---
    for rule in RULES_FLAG:
        if rule.requires_flag and not file_flags.get(rule.requires_flag):
            continue
        if (rule.requires_no_flag
                and file_flags.get(rule.requires_no_flag)):
            continue
        if not rule.pattern.search(code_line):
            continue
        if rule.exclude and rule.exclude.search(code_line):
            continue
        emit(lineno, rule.id, rule.message)


# ---------------------------------------------------------------------------
# Post-scan file-level checks
# ---------------------------------------------------------------------------

def _post_scan_checks(
    filepath: str,
    lines: "List[str]",
    code_lines: "List[str]",
    file_flags: "Dict[str, bool]",
    tracker: BlockTracker,
    emit,
) -> None:
    """File-level checks + second-pass scans. Uses emit for all findings."""
    # STY-1: Root component missing id: root
    if tracker.root_block_seen and not tracker.root_block_has_id_root:
        for j, ln in enumerate(lines):
            if '{' in ln and not RE_COMMENT_LINE.match(ln.strip()):
                emit(j + 1, "STY-1",
                     "Top-level component should have id: root "
                     "(enables qualified lookup, future-proofs "
                     "for QML 3)")
                break

    # STA-3: Top-level states (not in StateGroup)
    # Only flag when the file appears to define a reusable component
    # (has required properties, indicating it's meant to be instantiated
    # by others). Plain application root items with states are fine.
    if file_flags["has_required_prop"]:
        state_depth = 0
        for j, ln in enumerate(lines):
            code_ln = code_lines[j]
            state_depth += code_ln.count('{') - code_ln.count('}')
            if state_depth == 1 and RE_STA_STATES.match(
                code_ln.strip()
            ):
                context_back = "\n".join(
                    code_lines[max(0, j - 3):j]
                )
                if "StateGroup" not in context_back:
                    emit(j + 1, "STA-3",
                         "Top-level states: in reusable component -- "
                         "use StateGroup (states is a QQmlListProperty,"
                         " appending from outside adds rather than "
                         "replaces)")
                    break

    # DEL-4: Component.onCompleted with reuseItems (file-level)
    if file_flags["has_reuse_items"]:
        for j, ln in enumerate(lines):
            if RE_DEL_ON_COMPLETED.search(code_lines[j]):
                emit(j + 1, "DEL-4",
                     "Component.onCompleted with reuseItems: true -- "
                     "onCompleted does NOT re-fire on reuse; "
                     "use ListView.onReused")
                break

    # PRF-1: transparent Rectangle (second pass with line content)
    for f in _scan_transparent_rects(filepath, lines, code_lines):
        emit(f.line, f.rule, f.message)

    # IMG-2: Image with network source without async
    for f in _scan_image_async(filepath, lines, code_lines):
        emit(f.line, f.rule, f.message)


# ---------------------------------------------------------------------------
# Per-file scanner
# ---------------------------------------------------------------------------

def scan_file(filepath: str) -> "List[Finding]":
    """Scan a single QML file and return all findings."""
    findings: "List[Finding]" = []
    path = Path(filepath)

    try:
        raw = path.read_text(encoding="utf-8", errors="replace")
        lines = raw.splitlines()
    except OSError:
        return findings

    num_lines = len(lines)

    def emit(line: int, rule: str, msg: str) -> None:
        findings.append(Finding(filepath, line, rule, msg))

    # --- Comment stripping for flag/context scans ---
    code_lines = [re.sub(r'//.*$', '', ln) for ln in lines]
    code_text = "\n".join(code_lines)

    # --- File-level pre-scan flags ---
    file_flags: "Dict[str, bool]" = {
        "has_required_prop":     bool(RE_DEL_REQUIRED_PROP.search(code_text)),
        "has_reuse_items":       bool(RE_DEL_REUSE_ITEMS.search(code_text)),
        "has_import_qtquick":    bool(RE_IMP_QTQUICK.search(code_text)),
        "has_controls_plain":    bool(RE_IMP_CONTROLS_PLAIN.search(code_text)),
        "has_control_custom":    bool(RE_CONTROL_CUSTOMIZATION.search(code_text)),
        "has_opacity_anim":      bool(RE_PRF_OPACITY_ANIM.search(code_text)),
    }

    imports = ImportTracker()

    is_delegate_file = bool(re.search(
        r'(delegate\s*:|ListView|GridView|Repeater|DelegateModel|'
        r'DelegateChooser|required\s+property\s+\w+\s+\w+.*model)',
        code_text
    ))
    tracker = BlockTracker(is_delegate_file=is_delegate_file)

    # --- Line-by-line scan ---
    for i, line in enumerate(lines):
        lineno = i + 1
        stripped = line.strip()
        code_line = code_lines[i]

        # Skip pure comment lines
        if RE_COMMENT_LINE.match(stripped):
            continue

        # --- Import analysis (falls through to rules + structure) ---
        imports.process_line(lineno, stripped, emit)

        # Rule dispatch -- reads tracker.brace_depth before it updates
        _check_line_rules(lineno, i, line, stripped, code_line,
                          code_lines, code_text, num_lines,
                          tracker.brace_depth, is_delegate_file,
                          file_flags, emit)

        # Structural tracking -- updates brace_depth, block_stack, and
        # emits structural findings. Called AFTER rule dispatch so that
        # rules see the pre-update brace_depth (BND-2 needs this).
        tracker.process_line(lineno, stripped, code_line, emit,
                             file_flags)

    # --- Post-scan file-level checks ---
    imports.post_scan_checks(lines, file_flags, emit)
    _post_scan_checks(filepath, lines, code_lines, file_flags,
                      tracker, emit)

    # Sort by line number
    findings.sort(key=lambda f: (f.file, f.line))
    return findings


def _check_closed_block(
    block: Block,
    emit,
    file_flags: "Dict[str, bool]",
) -> None:
    """Run block-level checks when a QML object block closes."""
    props = block.properties

    # --- LAY-1: anchors + Layout on same item ---
    has_anchors = any(k.startswith("anchors") for k in props)
    has_layout = any(k.startswith("Layout") for k in props)
    if has_anchors and has_layout:
        emit(block.start_line, "LAY-1",
             f"{block.type_name or 'Item'}: anchors and Layout.* "
             "on same item -- they conflict; pick one")

    # --- LAY-2: bare width/height inside Layout parent ---
    if block.parent_type in LAYOUT_TYPES:
        for prop_name in ("width", "height"):
            if prop_name in props:
                cap = prop_name.capitalize()
                emit(props[prop_name], "LAY-2",
                     f"{prop_name}: inside {block.parent_type} child "
                     f"-- use Layout.preferred{cap} or Layout.fill{cap}")
        # LAY-6: bare x/y inside Layout parent
        for prop_name in ("x", "y"):
            if prop_name in props:
                emit(props[prop_name], "LAY-6",
                     f"{prop_name}: inside {block.parent_type} child "
                     "-- Layout manages positioning; remove explicit "
                     f"{prop_name}")

    # --- LAY-3: Four anchor edges instead of fill ---
    anchor_edges = {
        "anchors.left", "anchors.right",
        "anchors.top", "anchors.bottom",
    }
    found_edges = anchor_edges.intersection(props.keys())
    if len(found_edges) == 4 and "anchors.fill" not in props:
        first_line = min(props[e] for e in found_edges)
        emit(first_line, "LAY-3",
             "Four separate anchor edges -- "
             "use anchors.fill: parent instead")

    # --- IMG-1: Image without sourceSize ---
    if block.type_name == "Image":
        has_ss = any(k.startswith("sourceSize") for k in props)
        if not has_ss:
            emit(block.start_line, "IMG-1",
                 "Image without sourceSize -- decodes full resolution "
                 "into GPU memory; set sourceSize to display dimensions")

    # IMG-2 is checked in _scan_image_async (needs line content).

    # --- LDR-5: Loader with both source and sourceComponent ---
    if block.type_name == "Loader":
        if "source" in props and "sourceComponent" in props:
            emit(block.start_line, "LDR-5",
                 "Loader has both source and sourceComponent -- "
                 "these are mutually exclusive; use one or the other")

    # --- ORD checks: attribute ordering within block ---
    # Skip blocks where the standard attribute ordering convention
    # does not apply (Connections has target: + function handlers;
    # Behavior has property-specific internal structure).
    if (len(block.categories) >= 2
            and block.type_name not in SKIP_ORD_TYPES):
        _check_ordering(block, emit)

    # --- STY-3: Multiple dot-notation properties from same group ---
    group_counts: "Dict[str, List[int]]" = {}
    for prop_name, prop_line in props.items():
        if "." in prop_name:
            group = prop_name.split(".")[0]
            # Skip attached property namespaces (these use dot by convention)
            if group not in (
                "Layout", "Component", "Drag", "Keys",
                "Accessible", "LayoutMirroring", "ListView",
                "GridView", "TableView", "SwipeView",
            ):
                group_counts.setdefault(group, []).append(prop_line)
    for group, group_lines in group_counts.items():
        if len(group_lines) >= 3:
            emit(min(group_lines), "STY-3",
                 f"{len(group_lines)} {group}.* properties using dot "
                 f"notation -- use group notation: {group} {{ ... }}")


def _check_ordering(block: Block, emit) -> None:
    """Check that QML attributes appear in the expected order."""
    prev_cat = -1
    for cat, lineno in block.categories:
        if cat < prev_cat:
            cat_names = {
                CAT_ID: "id",
                CAT_PROP_DECL: "property declaration",
                CAT_SIGNAL_DECL: "signal declaration",
                CAT_PROP_ASSIGN: "property assignment",
                CAT_ATTACHED: "attached property",
                CAT_STATES: "states",
                CAT_TRANSITIONS: "transitions",
                CAT_HANDLER: "signal handler",
                CAT_CHILD: "child object",
                CAT_FUNCTION: "function",
            }
            expected = cat_names.get(prev_cat, "previous attribute")
            actual = cat_names.get(cat, "this attribute")
            emit(lineno, "ORD-1",
                 f"{actual} appears after {expected} -- "
                 "expected order: id, properties, signals, "
                 "assignments, attached, states, transitions, "
                 "handlers, children, functions")
            return  # Only report first ordering violation per block
        prev_cat = cat


def _scan_transparent_rects(
    filepath: str,
    lines: "List[str]",
    code_lines: "List[str]",
) -> "List[Finding]":
    """Find Rectangle blocks with color: 'transparent'.

    Handles both single-line `color: "transparent"` and multiline
    ternary expressions where "transparent" appears on a continuation
    line after `color:`.
    """
    findings: "List[Finding]" = []
    depth = 0
    rect_depth = -1
    rect_start = 0
    in_color_expr = False

    for i, code_line in enumerate(code_lines):
        lineno = i + 1
        stripped = code_line.strip()

        open_b = code_line.count('{')
        close_b = code_line.count('}')

        if re.search(r'\bRectangle\s*\{', stripped):
            rect_depth = depth + open_b
            rect_start = lineno
            in_color_expr = False

        # Check only direct properties of the Rectangle (same depth)
        elif rect_depth > 0 and depth == rect_depth:
            # Single-line match: color: "transparent" or
            # color: cond ? "x" : "transparent"
            if RE_PRF_TRANSPARENT.search(code_line):
                findings.append(Finding(
                    filepath, lineno, "PRF-1",
                    "Rectangle with 'transparent' color -- "
                    "use Item instead, or toggle visible on the "
                    "Rectangle (creates geometry node even when "
                    "transparent)"
                ))
                rect_depth = -1
            # Track multiline color: expressions
            elif re.match(r'^\s*color\s*:', stripped):
                in_color_expr = True
            elif in_color_expr:
                # Continuation of color expression
                if RE_PRF_TRANSPARENT_CONT.search(code_line):
                    findings.append(Finding(
                        filepath, rect_start, "PRF-1",
                        "Rectangle with 'transparent' in color "
                        "expression -- use Item instead, or toggle "
                        "visible (creates geometry node even when "
                        "transparent)"
                    ))
                    rect_depth = -1
                    in_color_expr = False
                # End of continuation if line doesn't look like
                # a continued expression
                if not stripped.endswith(('?', ':', '||', '&&')):
                    in_color_expr = False
            else:
                in_color_expr = False

        depth += open_b - close_b
        if depth < rect_depth:
            rect_depth = -1

    return findings


RE_IMG_NETWORK_SRC = re.compile(r'source\s*:.*https?://')


def _scan_image_async(
    filepath: str,
    lines: "List[str]",
    code_lines: "List[str]",
) -> "List[Finding]":
    """Flag Image blocks with network/dynamic source but no async.

    Uses raw lines (not comment-stripped) because URLs contain //
    which the comment stripper would remove.
    """
    findings: "List[Finding]" = []
    depth = 0
    img_depth = -1
    img_start = 0
    has_async = False
    has_network_src = False

    for i, raw_line in enumerate(lines):
        lineno = i + 1
        code_line = code_lines[i]
        stripped = raw_line.strip()

        open_b = code_line.count('{')
        close_b = code_line.count('}')

        if re.match(r'Image\s*\{', stripped):
            img_depth = depth + open_b
            img_start = lineno
            has_async = False
            has_network_src = False
        elif img_depth > 0 and depth == img_depth:
            if "asynchronous" in raw_line:
                has_async = True
            if RE_IMG_NETWORK_SRC.search(raw_line):
                has_network_src = True

        depth += open_b - close_b
        if depth < img_depth:
            # Image block closed
            if not has_async and has_network_src:
                findings.append(Finding(
                    filepath, img_start, "IMG-2",
                    "Image with network source without "
                    "asynchronous: true -- download+decode "
                    "blocks the UI thread"
                ))
            img_depth = -1

    return findings


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

def main() -> int:
    if len(sys.argv) < 2:
        print(
            "Usage: qt_qml_lint.py [--json] <file> [file ...]",
            file=sys.stderr,
        )
        print(
            "       qt_qml_lint.py --files-from=- < list.txt",
            file=sys.stderr,
        )
        return 2

    json_output = False
    files: "List[str]" = []
    for arg in sys.argv[1:]:
        if arg == "--json":
            json_output = True
        elif arg == "--files-from=-":
            files.extend(
                line.strip() for line in sys.stdin if line.strip()
            )
        else:
            files.append(arg)

    if not files:
        print("Error: no input files specified", file=sys.stderr)
        return 2

    all_findings: "List[Finding]" = []
    for filepath in files:
        all_findings.extend(scan_file(filepath))

    # Sort by file, then line
    all_findings.sort(key=lambda f: (f.file, f.line))

    # Deduplicate (same file + line + rule)
    seen: "Set[Tuple[str, int, str]]" = set()
    deduped: "List[Finding]" = []
    for f in all_findings:
        key = (f.file, f.line, f.rule)
        if key not in seen:
            seen.add(key)
            deduped.append(f)

    if json_output:
        print(json.dumps(
            [asdict(f) for f in deduped],
            indent=2,
        ))
    else:
        for finding in deduped:
            print(finding)

    return 1 if deduped else 0


if __name__ == "__main__":
    sys.exit(main())
