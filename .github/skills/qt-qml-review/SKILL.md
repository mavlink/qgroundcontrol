---
name: qt-qml-review
description: >-
  Invoke when the user asks to review, check, audit, or look
  over Qt6 QML code -- or suggest before committing. Runs
  deterministic linting (47+ rules) then six parallel deep-
  analysis agents covering bindings, layout, loaders, delegates,
  states, and performance. Optionally invokes system qmllint
  for type-level checks. Reports only high-confidence issues
  (>80/100) with structured mitigations. Read-only -- never
  modifies code.
license: LicenseRef-Qt-Commercial OR BSD-3-Clause
compatibility: Designed for Claude Code, GitHub Copilot, and similar agents.
disable-model-invocation: false
metadata:
  author: qt-ai-skills
  version: "1.0"
  qt-version: "6.x"
  category: review
---

# Qt QML Code Review

A structured, read-only code review skill for Qt6 QML code that
combines deterministic linting with parallel agent-driven deep
analysis across six focused domains.

## When to use this skill

- When the user mentions review-related tasks: "review", "check",
  "audit", "look over", "code review", "sanity check"
- Suggest running this skill **before committing** QML code
- When the user asks to validate Qt6 QML code quality

## Scope detection

Detect the user's intended scope from their language:

### Diff/commit scope (narrow)
Triggered by language like: "this commit", "these changes",
"the diff", "what I changed", "my changes", "staged changes",
"outstanding changes", "before I commit"

**Action**: Run `git diff` (unstaged) and `git diff --cached`
(staged) to obtain the changeset. If the user says "this commit",
use `git diff HEAD~1..HEAD`. Review only the changed lines plus
sufficient surrounding context (±50 lines) for understanding.
Only report issues found in the changed lines -- do not report
issues in unchanged surrounding context.

### Codebase scope (wide)
Triggered by language like: "review the codebase", "audit the
project", "check the repository", "review src/", or when a specific
file/directory path is given without commit language.

**Action**: Glob for `*.qml` files in the specified scope. Review
all matched files.

## Execution order

The review proceeds in three phases. **Never skip a phase.**

### Phase 1: Deterministic linting (Python script)

Run the unified Python linter against the target files. Requires
Python 3.6+ (no external dependencies). If Python is not available,
warn the user and skip to Phase 1b.

```bash
python3 references/lint-scripts/qt_qml_lint.py <files...>
# If python3 is not found, fall back to:
python references/lint-scripts/qt_qml_lint.py <files...>
```

This single-pass scanner encodes all mechanically-checkable rules
from the QML review checklist. It reads each file once and evaluates
all rules per line, plus block-level structural checks. Output is
deterministic and repeatable. The linter is authoritative -- do not
second-guess its output.

Collect all output before proceeding.

**Rule categories** (47+ checks):
- **IMP** (Imports) -- ordering, versioning, redundancy, deprecation
- **ORD** (Ordering) -- QML attribute ordering convention
- **BND** (Bindings) -- property var, imperative =, Qt.binding style
- **LAY** (Layout) -- anchors/Layout mixing, sizing in layouts
- **LDR** (Loader) -- status guards, createComponent, createQmlObject
- **DEL** (Delegates) -- required properties, reuse safety, connect()
- **STA** (States) -- PropertyChanges syntax, transitions, StateGroup
- **IMG** (Images) -- sourceSize, asynchronous loading
- **PRF** (Performance) -- transparent rect, opacity, clip, layer
- **STY** (Style) -- id:root, camelCase, group notation
- **SIG** (Signals) -- Connections target, handler syntax
- **ERR** (Error/Security) -- hardcoded http://, non-portable paths
- **JS** (JavaScript) -- var/let/const, loose equality

### Phase 1b: System qmllint (optional)

Attempt to run `qmllint` if available on the system. Detection
order:

1. `$QT_HOST_PATH/bin/qmllint`
2. `which qmllint` / `where qmllint`
3. Skip if not found (warn user)

If found, run with JSON output:

```bash
qmllint --json - -I <import-paths> <files...>
```

Parse the JSON output and merge with Python linter findings.
Deduplicate by file+line+issue. qmllint is authoritative for type-
level checks (unresolved types, incompatible assignments, alias
cycles). The Python linter is authoritative for style, ordering,
and performance patterns that qmllint does not cover.

### Phase 2: Agent-driven deep analysis (6 parallel agents)

Launch six focused review agents in parallel. Name each agent
descriptively when launching (e.g. "Agent 1: Bindings & Properties")
to provide progress visibility. Each agent has a tight scope
and a specific checklist. Agents are READ-ONLY -- they must
never edit or write files.

**Tool-agnostic agent contract**: Each agent described below is
a self-contained review mission. In Claude Code, launch them as
general-purpose subagents. In other tools, implement each as
whatever subprocess, prompt chain, or analysis pass the tool
supports. The key requirement is that each agent:
- Has read access to all source files in scope
- Can search/grep the codebase to trace symbols
- Reports findings in the structured format below
- Applies confidence thresholds: >80 = confirmed finding,
  60-79 = investigation target (max 10 total across all
  agents), <60 = suppress
- Does NOT duplicate findings from Phase 1 lint output
  (pass lint output as context to each agent)

See **Agent missions** below for the six agents.

### Phase 3: Consolidation and reporting

Merge lint script output, qmllint output (if available), and all
agent findings. Deduplicate (same file+line+issue = one finding).
Apply confidence scoring. Format the final report using the output
format below.

## Agent missions

Launch all six agents in parallel. Pass each agent:
1. The list of files in scope
2. The Phase 1 lint output (so they skip already-flagged issues)
3. The Phase 1b qmllint output if available
4. Their specific mission below

Each agent should read all files in scope, then focus on its
assigned categories.

---

### Agent 1: Bindings & Properties

**Scope**: Binding correctness, property types, alias chains,
qualified lookup, binding loops.

**Check for**:
- Multi-cycle binding loops (A changes B via handler, B's binding
  updates A) -- runtime only detects single-cycle
- Property alias chains (alias to alias) where intermediate
  components may not be initialized
- Unqualified property access (bare `someProperty` instead of
  `root.someProperty`) -- complements qmllint `unqualified` warning
  with semantic context
- `Qt.binding()` closures capturing loop variables by reference
  (use `let` not `var`)
- `pragma ComponentBehavior: Bound` missing on files with delegates
  that access outer-scope ids
- Missing `readonly` on properties that are bound but never
  imperatively assigned

**References**: `references/qt-qml-review-checklist.md`
sections 3 (Bindings & Properties)

---

### Agent 2: Layout & Anchoring

**Scope**: Anchoring correctness, layout sizing, visual tree
structure.

**Check for**:
- Anchoring to items with `visible: false` (resolve the target id,
  check its `visible` property)
- Anchoring across unrelated visual tree branches (not sharing a
  common parent)
- Items in Layouts using `implicitWidth`/`implicitHeight` bindings
  that could create feedback loops
- Missing `Layout.fillWidth`/`Layout.fillHeight` on items that
  should stretch
- Nested Layouts without clear sizing policy (ambiguous size
  negotiation)

**References**: `references/qt-qml-review-checklist.md`
section 4 (Layout & Anchoring)

---

### Agent 3: Component Loading & Lifecycle

**Scope**: Loader patterns, dynamic object creation, Connections
lifecycle, C++ integration.

**Check for**:
- `Component.createObject()` return values not tracked or destroyed
  (memory leak)
- Loader switching between `source` and `sourceComponent` at runtime
  (unsupported)
- Image with dynamic/network source missing `Image.status` error
  handling
- `Connections` with dynamically-changing `target` not handling
  `null` target state
- Context properties (`rootContext()->setContextProperty()`) in C++
  integration code
- Object ownership issues at QML/C++ boundary (parentless objects
  returned from invokable functions)

**References**: `references/qt-qml-review-checklist.md`
sections 5 (Loader), 8 (Images), 13 (C++ Integration)

---

### Agent 4: ListView & Delegate Correctness

**Scope**: Model-view patterns, delegate lifecycle, reuse safety,
required properties.

**Check for**:
- Missing `required property int index` when `index` is used in a
  delegate that declares other required properties
- Delegate accessing `model.roleName` for roles not defined in the
  model's `roleNames()`
- Complex delegates (nested Repeaters, multiple Loaders, heavy
  bindings) that will degrade scroll performance
- `currentIndex` usage without guards for known Qt bugs
  (QTBUG-48633, QTBUG-93293)
- `DelegateChooser` patterns that could fail on non-QAbstractItemModel
  (choice made once at creation, not re-evaluated)
- Pooled delegates remaining visible (missing
  `onPooled: visible = false` pattern)

**References**: `references/qt-qml-review-checklist.md`
section 6 (ListView & Delegates)

---

### Agent 5: States, Transitions & Structure

**Scope**: State machine correctness, migration patterns, component
structure.

**Check for**:
- `PropertyChanges.restoreEntryValues` surprises (properties
  reverting on state exit when developer expects them to persist)
- `Binding.restoreMode` mismatch from Qt 5 migration (default
  changed from `RestoreNone` to `RestoreBindingOrValue`)
- Deprecated `Connections` handler syntax (`onFoo:`) vs
  modern `function onFoo()` in migrated code
- `QtGraphicalEffects` imports that should be migrated to
  `MultiEffect` (Qt 6.5+)
- Top-level component states that should use `StateGroup` for
  reusability
- Missing `from`/`to` on transitions that could fire unexpectedly
  when new states are added

**References**: `references/qt-qml-review-checklist.md`
sections 7 (States), 14 (Migration)

---

### Agent 6: Performance & Code Quality

**Scope**: Performance anti-patterns, rendering cost, JavaScript
quality, style consistency.

**Check for**:
- Expensive expressions in property bindings (function calls that
  should be cached as `readonly property`)
- `QRegularExpression` or complex computation inside loops
- Missing `Text.PlainText` when rich text is not needed (default
  `textFormat` incurs parsing overhead)
- `font.preferShaping: false` opportunity (when text shaping
  features are unused)
- Signals that communicate down (should be functions) or functions
  that communicate up (should be signals)
- Unnecessary `id` assignments on objects never referenced
- Custom properties scattered across items instead of consolidated
  in `QtObject { id: privates }`
- Singletons used for data (should use property injection for
  testability)
- Pointer handler opportunities (MouseArea that should be
  TapHandler/DragHandler for multi-touch)
- Reusable components with explicit `width`/`height` instead of
  `implicitWidth`/`implicitHeight` (prevents consumer resizing)
- `parent` used without null-check in delegates or Loader items
  (can be null during creation/destruction)
- Missing `pragma ComponentBehavior: Bound` on files with delegates
  that access outer-scope ids

**References**: `references/qt-qml-review-checklist.md`
sections 9 (Performance), 10 (Style), 11 (Signals),
12 (JavaScript), 13 (C++ Integration)

---

## Confidence scoring guidelines

| Confidence | Meaning | Action |
|------------|---------|--------|
| 90-100 | Certain: direct rule violation with full trace | Report as finding |
| 80-89 | High: rule violation confirmed but edge case possible | Report as finding |
| 60-79 | Medium: likely issue but cannot fully verify | Report as investigation target |
| <60 | Low: suspicion only | Suppress entirely |

**Investigation targets** are findings the agent believes are real
but cannot fully verify. These are presented in a separate section
for human verification. Maximum 10 investigation targets per report,
prioritized by confidence within the 60-79 band.

## Output format

Present the final report as follows. Use exactly this structure.

```
## QML Code Review Report

**Scope**: [diff: `git diff HEAD~1..HEAD` | files: <paths>]
**Files reviewed**: N
**Issues found**: N (M from lint, K from deep analysis)
**qmllint**: [ran / not available]

---

### Lint findings

For each lint finding:

#### [L-NNN] <Short title>
- **File**: `path/to/file.qml:42`
- **Rule**: <rule ID from checklist>
- **Finding**: <what the script detected>
- **Mitigation**: <what to do, in prose -- no code patches>

---

### Deep analysis findings

For each agent finding:

#### [D-NNN] <Short title>
- **File**: `path/to/file.qml:42`
- **Category**: <agent name: Bindings & Properties | Layout &
  Anchoring | Component Loading & Lifecycle | ListView &
  Delegates | States & Structure | Performance & Quality>
- **Confidence**: NN/100
- **Finding**: <description of the issue>
- **Trace**: <how the issue was confirmed -- which symbols were
  followed, what was checked>
- **Mitigation**: <what to do, in prose -- no code patches>

---

### Investigation targets (human verification needed)

Findings the agent identified but could not fully verify.
Maximum 10, sorted by confidence. These require human judgment.

For each investigation target:

#### [I-NNN] <Short title>
- **File**: `path/to/file.qml:42`
- **Category**: <agent name>
- **Confidence**: NN/100
- **Finding**: <what the agent suspects>
- **Unverified because**: <what the agent could not confirm>
- **How to verify**: <specific action for the reviewer>

---

### Summary

| Category | Lint | Deep | Investigate | Total |
|----------|------|------|-------------|-------|
| ...      | N    | N    | N           | N     |
| **Total**| **M**| **K**| **I**       | **N** |

Findings below confidence 60 are suppressed entirely.
```

## References

The following reference files contain detailed checklists:

- `references/qt-qml-review-checklist.md` -- Complete QML review
  rules (lint + agent rules, always loaded)
- `references/lint-scripts/qt_qml_lint.py` -- Single-pass Python
  linter (runs all 47+ checks in <1s)

---

Copyright (C) 2026 The Qt Company.
