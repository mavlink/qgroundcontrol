---
name: qt-cpp-review
description: >-
  Invoke when the user asks to review, check, audit, or look
  over Qt6 C++ code — or suggest before committing. Runs
  deterministic linting (60+ rules) then six parallel deep-
  analysis agents covering model contracts, ownership, threading,
  API correctness, error handling, and performance. Reports only
  high-confidence issues (>80/100) with structured mitigations.
  Read-only — never modifies code.
license: LicenseRef-Qt-Commercial OR BSD-3-Clause
compatibility: Designed for Claude Code, GitHub Copilot, and similar agents.
disable-model-invocation: false
metadata:
  author: qt-ai-skills
  version: "2.0"
  qt-version: "6.x"
  category: review
argument-hint: "[framework]"
---

# Qt Code Review

A structured, read-only code review skill for Qt6 C++ code that
combines deterministic linting with parallel agent-driven deep
analysis across six focused domains.

## When to use this skill

- When the user mentions review-related tasks: "review", "check",
  "audit", "look over", "code review", "sanity check"
- Suggest running this skill **before committing** code
- When the user asks to validate Qt6 C++ code quality

## Arguments

- `/qt-cpp-review` — review using universal Qt6 C++ rules only
- `/qt-cpp-review framework` — also apply Qt framework/module
  development rules (BC, exports, d-pointers, qdoc, QML
  versioning)

## Framework mode detection

If `$ARGUMENTS` contains "framework", enable framework mode.

If the argument is not passed, auto-detect by scanning the first
few files in scope for framework signals. If **two or more** of
the following are found, suggest to the user:
"This looks like Qt framework/module code. Run
`/qt-cpp-review framework` to also apply framework-specific
rules (BC, exports, qdoc, QML versioning)?"

**Framework signals** (any two = likely framework code):
- `QT_BEGIN_NAMESPACE` / `QT_END_NAMESPACE`
- `Q_CORE_EXPORT`, `Q_GUI_EXPORT`, `Q_WIDGETS_EXPORT`, or any
  `Q_*_EXPORT` macro
- `#include <QtModule/private/*_p.h>` (private headers)
- `Q_DECLARE_PRIVATE`, `Q_D()`, `Q_Q()`
- `qt_internal_add_module` or `qt_add_module` in CMakeLists.txt
- `sync.profile` or `.qmake.conf` in the repository root

Do **not** auto-enable framework mode — only suggest it. Let the
user confirm.

When framework mode is enabled:
1. Pass `--framework` to the linter (if supported)
2. Load `references/qt-framework-checklist.md` alongside the
   universal checklist
3. Include framework rules in each agent's mission context

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
Only report issues found in the changed lines — do not report
issues in unchanged surrounding context.

### Codebase scope (wide)
Triggered by language like: "review the codebase", "audit the
project", "check the repository", "review src/", or when a specific
file/directory path is given without commit language.

**Action**: Glob for `*.cpp`, `*.h`, `*.hpp` files in the
specified scope. Review all matched files.

## Execution order

The review proceeds in three phases. **Never skip a phase.**

### Phase 1: Deterministic linting (scripts)

Run the unified Python linter against the target files. Requires
Python 3.6+ (no external dependencies). If Python is not
available, warn the user and skip to Phase 2.

```bash
python3 references/lint-scripts/qt_review_lint.py <files...>
# If python3 is not found, fall back to:
python references/lint-scripts/qt_review_lint.py <files...>
```

This single-pass scanner encodes all mechanically-checkable rules
from the Qt review guidelines. It reads each file once and
evaluates all rules per line. Output is deterministic and
repeatable. The linter is authoritative — do not second-guess
its output.

Collect all output before proceeding to Phase 2.

**Rule categories** (60+ checks):
- **INC** (Includes) — ordering, qglobal.h, qNN duplication
- **DEP** (Deprecated) — obsolete Qt/std class usage
- **PAT** (Patterns) — anti-patterns (min/max, std::optional,
  NRVO, COW detach, etc.)
- **MDL** (Model) — QAbstractItemModel contract (begin/end
  balance, dataChanged roles, flags, default: in data())
- **ERR** (Error Handling) — QFile::open, QJsonDocument::isNull,
  QNetworkReply::error, SSL, timeouts, arg() mismatch
- **LCY** (Lifecycle) — deleteLater, Q_ASSERT side effects,
  null guards, unbounded containers, qDeleteAll depth
- **API** (Naming) — get-prefix, enum hygiene, QList<QString>
- **HDR/TMO/CND/VAL/TRN** — headers, timeouts, conditionals,
  value classes, ternary operator

### Phase 2: Agent-driven deep analysis (6 parallel agents)

Launch six focused review agents in parallel. Name each agent
descriptively when launching (e.g. "Agent 1: Model Contracts")
to provide progress visibility. Each agent has a tight scope
and a specific checklist. Agents are READ-ONLY — they must
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
  60–79 = investigation target (max 10 total across all
  agents), <60 = suppress
- Does NOT duplicate findings from Phase 1 lint output
  (pass lint output as context to each agent)

See **Agent missions** below for the six agents.

### Phase 3: Consolidation and reporting

Merge lint script output and all agent findings. Deduplicate
(same file+line+issue = one finding). Apply confidence scoring.
Format the final report using the output format below.

## Agent missions

Launch all six agents in parallel. Pass each agent:
1. The list of files in scope
2. The Phase 1 lint output (so they skip already-flagged issues)
3. Their specific mission below

Each agent should read all files in scope, then focus on its
assigned categories.

---

### Agent 1: Model Contracts

**Scope**: QAbstractItemModel signal protocol, role system,
index validity, proxy model correctness.

**Check for**:
- `beginInsertRows`/`endInsertRows` balance — every structural
  model change (add/remove/move) must use the correct begin/end
  pairs. `layoutChanged` is NOT a substitute for insert/remove.
- `roleNames()` returning roles that `data()` does not handle
  (missing switch cases, fall-through to default)
- `dataChanged` emitted with empty roles vector (forces full
  refresh instead of targeted update)
- `beginRemoveRows` called with `first > last` (edge case when
  container is empty — QAIM contract violation)
- `flags()` returning inappropriate flags (e.g. `ItemIsEditable`
  for non-editable items)
- `setData()` returning true without emitting `dataChanged`
- Proxy models accessing source model internals instead of going
  through `data()`/`index()` API
- Filter/proxy models using source-model indices to index into
  filtered containers (wrong index space)

**References**: `references/qt-review-checklist.md` § Model
Contracts

---

### Agent 2: Ownership & Lifecycle

**Scope**: Memory ownership, parent-child, resource cleanup,
Rule of Five, RAII correctness.

**Check for**:
- Structs/classes with raw pointers where `new` is visible and
  no corresponding `delete`/`deleteLater`/smart-pointer wrapping
  exists (Rule of Five violation)
- Missing `deleteLater()` on QNetworkReply in finished handlers
- `Q_ASSERT` wrapping side-effectful expressions (compiled out
  in release builds — the side effect disappears)
- `Q_ASSERT` as the sole null guard (crashes in release)
- Polymorphic QObject subclasses missing `Q_DISABLE_COPY_MOVE`
- Polymorphic classes missing virtual destructor
- QTimer/QObject created with `new` but no parent and no other
  lifecycle management (scope, smart pointer, explicit delete)
- `QObject::connect()` called with potentially null
  sender/receiver outside a null guard (runtime warning)
- `m_recentlyAccessed`-style tracking lists that maintain
  pointers to objects that may be deleted elsewhere (dangling)
- Unbounded container growth (append without cap or trim)
- Destructor not cleaning up owned children recursively
- Abstract interfaces with no implementations beyond one class
  (YAGNI violation — codebase scope only)

**References**: `references/qt-review-checklist.md` § Ownership
& Lifecycle, § Polymorphic Classes, § RAII Classes

---

### Agent 3: Thread Safety

**Scope**: Cross-thread QObject access, mutex consistency,
signal emission from worker threads.

**Check for**:
- QObject member variables written from `QtConcurrent::run()`
  or `QThread` worker without synchronization (mutex, atomic,
  queued connection, or other thread-safe primitive)
- Signals emitted from worker threads connected with
  `Qt::DirectConnection` (or explicit non-queued connections)
  to main-thread receivers
- Model mutations (`addNote`, `removeRows`, etc.) from
  background threads
- Shared containers (`QList`, `QHash`) modified from multiple
  threads without consistent synchronization
- Non-atomic increment/decrement of shared counters
  (`m_operationCount++` from multiple threads)
- QTimer or other QObject operations from non-owner thread

**References**: `references/qt-review-checklist.md` § Thread
Safety

---

### Agent 4: API, Naming & C++ Correctness

**Scope**: Qt naming conventions, const-correctness, move
semantics, enum hygiene, noexcept correctness.

**Check for**:
- `get`-prefix on mere getters (Qt reserves `get` for user
  interaction or out-parameter decomposition)
- Non-const getter methods (especially Q_PROPERTY READ
  accessors — UB via meta-object system)
- Missing `std::forward<T>()` on forwarding/universal references
- `return std::move(localVar)` preventing NRVO
- `const` local variable preventing implicit move on return
  (e.g. `const QJsonDocument doc(...); return doc;` forces copy)
- `const` method returning mutable pointer through raw pointer
  indirection (`findById() const` returning `T*` lets callers
  mutate via a const accessor — const doesn't propagate through
  raw pointers)
- `noexcept` on functions containing `Q_ASSERT` (incompatible —
  Q_ASSERT may throw for testing, noexcept terminates)
- Unscoped enums without explicit underlying type
- Missing trailing comma on last enumerator
- `switch` over enum with `default:` label (suppresses -Wswitch)
- `QList<QString>` instead of `QStringList`
- Missing `const` on methods that don't modify state
- Case-sensitive string comparison for user-facing sort
- Duplicated validation logic across classes
- `const QMetaObject::Connection` preventing handle cleanup

**References**: `references/qt-review-checklist.md` § API &
Naming, § Enums, § Methods, § Move Semantics, § Operators

---

### Agent 5: Error Handling & Validation

**Scope**: Missing error checks, input validation, security.

**Check for**:
- `QFile::open()` return value ignored
- `QJsonDocument::fromJson()` result not checked for
  `isNull()`/`isObject()` before use
- `QNetworkReply::error()` not checked before `readAll()`
- XML writer `hasError()` not checked after writing
- Hardcoded `http://` instead of `https://` in URLs
- No SSL error handling (`QNetworkAccessManager::sslErrors`)
- No timeout on network requests (`setTransferTimeout`)
- Negative values accepted where only positive are valid
  (e.g. timer intervals, font sizes)
- No schema/version validation on imported data
- No input length validation on imported/downloaded data
  (unbounded strings from untrusted sources)
- `QString::arg()` with wrong placeholder count
- `saveToFile()` returning true regardless of I/O errors
- Inconsistent error reporting patterns across methods

**References**: `references/qt-review-checklist.md` § Error
Handling & Validation

---

### Agent 6: Performance & Code Quality

**Scope**: Performance anti-patterns, dead code, unnecessary
copies, code smells.

**Check for**:
- `QRegularExpression` constructed inside a loop (expensive
  compilation on every iteration)
- `roleNames()` rebuilding QHash on every call (should cache)
- Non-const range-for over COW-shared QList/QHash triggering
  unnecessary detach/deep-copy
- Non-const `operator[]` on shared QHash (triggers detach) —
  use `.value()` for reads
- Expensive operation before cheap early-exit check (wasted
  allocation)
- Dead/unreachable code (functions never called, branches
  that are always true/false given preconditions)
- Magic numbers without named constants
- God classes violating Single Responsibility
- Copy-pasted validation/logic across classes
- Stale member caches not invalidated on model changes
  (e.g. search cache surviving data edits)
- `QMap`/`QHash` iteration order nondeterminism when selecting
  a "best" or "first" entry (`.first()` changes if keys are
  added; use deterministic tie-breaking)
- `QMap` for small fixed-size constant data (use array/switch)
- Returning QList/container by value from frequently-called
  methods (implicit deep copy on every call — return const ref
  or cache)
- Member variables maintained (appended, capped) but never
  read by any method (dead state — wasted CPU and memory)
- Missing re-entrancy guard on methods that emit signals
  which could trigger re-entry
- Setter silently resetting unrelated state without signal
- Early return skipping status/signal updates

**References**: `references/qt-review-checklist.md` §
Performance & Code Quality

---

## Confidence scoring guidelines

| Confidence | Meaning | Action |
|------------|---------|--------|
| 90–100 | Certain: direct rule violation with full symbol trace | Report as finding |
| 80–89 | High: rule violation confirmed but edge case possible | Report as finding |
| 60–79 | Medium: likely issue but cannot fully verify | Report as investigation target |
| <60 | Low: suspicion only | Suppress entirely |

**Investigation targets** are findings the agent believes are real
but cannot fully verify — e.g. noexcept correctness requiring
whole-program analysis, dead code that may have callers outside
scope, or design-intent judgments like virtual access levels.
These are presented in a separate section for human verification.
Maximum 10 investigation targets per report, prioritized by
confidence within the 60–79 band.

## Output format

Present the final report as follows. Use exactly this structure.

```
## Qt Code Review Report

**Scope**: [diff: `git diff HEAD~1..HEAD` | files: <paths>]
**Files reviewed**: N
**Issues found**: N (M from lint, K from deep analysis)

---

### Lint findings

For each lint finding:

#### [L-NNN] <Short title>
- **File**: `path/to/file.cpp:42`
- **Rule**: <rule ID from checklist>
- **Finding**: <what the script detected>
- **Mitigation**: <what to do, in prose — no code patches>

---

### Deep analysis findings

For each agent finding:

#### [D-NNN] <Short title>
- **File**: `path/to/file.cpp:42`
- **Category**: <agent name: Model Contracts | Ownership &
  Lifecycle | Thread Safety | API & C++ Correctness | Error
  Handling | Performance & Quality>
- **Confidence**: NN/100
- **Finding**: <description of the issue>
- **Trace**: <how the issue was confirmed — which symbols were
  followed, what was checked>
- **Mitigation**: <what to do, in prose — no code patches>

---

### Investigation targets (human verification needed)

Findings the agent identified but could not fully verify.
Maximum 10, sorted by confidence. These require human judgment.

For each investigation target:

#### [I-NNN] <Short title>
- **File**: `path/to/file.cpp:42`
- **Category**: <agent name>
- **Confidence**: NN/100
- **Finding**: <what the agent suspects>
- **Unverified because**: <what the agent could not confirm —
  e.g. "cannot trace all callees for throw potential",
  "only one implementation visible in scope">
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

The following reference files contain detailed checklists
extracted from the Qt wiki "Things To Look Out For In Reviews":

- `references/qt-review-checklist.md` — Universal Qt6 C++ review
  rules (always loaded)
- `references/qt-framework-checklist.md` — Qt framework/module
  development rules (loaded only in framework mode)
- `references/qt-deprecated-classes.md` — Classes and patterns
  that should no longer be used in Qt implementation
- `references/lint-scripts/qt_review_lint.py` — Single-pass
  Python linter (runs all 60+ checks in <1s)
