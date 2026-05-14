# Qt Orchestration Module — Design Spec

**Date:** 2026-05-13
**Status:** Approved, pending implementation plan
**Authors:** Strattoon + Claude (brainstorming session)
**Context:** Sprig GCS fork of QGroundControl; Phase 2 issue authoring + execution

## Problem

Phase 1 slices repeatedly declared "done" while leaving Qt-specific work
incomplete. Specifically:

1. New QML / image assets added without registration in `custom/custom.qrc`,
   or registered in a way that silently dropped sibling qresource blocks on
   merge.
2. New `Q_OBJECT` classes / `Q_PROPERTY` declarations added without matching
   MOC requirements (missing `Q_OBJECT` macro, missing getters/setters/signals
   to back declared properties, or types not registered for QML).
3. Agents edited upstream `src/**` instead of shadowing into `custom/`,
   breaking fork hygiene and the weekly upstream rebase contract.
4. QML files using hardcoded sizes/colors instead of `ScreenTools` /
   `QGCPalette`, or with imports that don't resolve against the configured
   Qt + qrc-registered modules.

Root cause: the orchestrator brief's `verification_command` is typically a
narrow unit-test target. Agents satisfy that command and stop. There is no
single shared definition of "Qt-complete" that the issue, the brief, and a
post-execution gate all consult — so each layer has its own slightly
different idea of done, and slices drift through the gap.

## Decision

Build a single Python module at `tools/qt_orchestration/` that encodes
"Qt-complete" once and exposes it three ways:

- **Conversational issue drafting** — library functions used by the
  orchestrator session to scan the repo, propose touchpoints, and render a
  Phase-2 issue body before `gh issue create`.
- **Slice brief expansion** — CLI that turns an issue into a Codex-ready
  brief, filling Qt-specific verification commands per touchpoint.
- **Slice gate** — CLI run by `/orchestrate` after every Codex return,
  independently re-checking the slice diff against the rule set.

All three consult the same `rules.py`. The agent (Codex) cannot satisfy a
softer interpretation than the gate, because they read the same code.

## Architecture

```text
tools/qt_orchestration/
├── rules.py          # The Qt-completion rule set — single source of truth
├── scan.py           # Repo scanner: qrc blocks, AUTOMOC targets, shadows
├── draft_issue.py    # Library: scan + prose → issue draft (conversational)
├── expand_brief.py   # CLI: issue → slice brief with Qt-aware commands
├── verify_slice.py   # CLI gate: runs rules against slice diff, non-zero on miss
├── templates/
│   ├── phase2-issue.md.j2
│   └── slice-brief-qt.md.j2   # Extends refactor-brief-template.md
└── tests/            # Unit + golden-file + e2e fixtures
```

Console scripts (via `tools/pyproject.toml`):

- `qt-orchestration scan <paths...>` — JSON touchpoint manifest
- `qt-orchestration expand-brief --issue <path>` — emits slice brief
- `qt-orchestration verify-slice [--diff <ref>]` — the gate

`draft_issue` is **not** a CLI subcommand — it is a library imported by an
orchestrator session (me) so I can scan, propose, and iterate with the user
before writing the issue file.

**Integration points:**

- `.orchestrator/templates/refactor-brief-template.md` gains a
  `{{qt_completion_block}}` placeholder filled by `expand-brief`.
- `/orchestrate` invokes `verify-slice` after every Codex slice return,
  before flipping the slice green.
- Phase-2 issues, when authored conversationally, get scanned first so the
  issue body already lists qrc/CMake/custom/ touchpoints.

## The Rule Set (`rules.py`)

Each rule is a class with `applies_to(diff) -> bool` and
`check(diff, repo) -> list[Finding]`. `Finding` carries `severity`
(BLOCKER/MAJOR/MINOR/NIT, matching `.orchestrator/review-rubric.md`),
`path`, `line`, `message`, and `fix_hint`.

### Failure mode 1 — QRC / CMake wiring drift

- **R-QRC-01** *(BLOCKER)*: every added `.qml` / `.js` / image under
  `custom/` must appear in exactly one `<file>` entry inside
  `custom/custom.qrc`.
- **R-QRC-02** *(BLOCKER)*: `custom/custom.qrc` sibling-qresource integrity
  — parse all `<qresource prefix="...">` blocks, assert count and prefixes
  unchanged unless the issue explicitly authorizes a prefix change.
  (Encodes the silent-drop hazard documented in user memory.)
- **R-CMAKE-01** *(BLOCKER)*: every added `.cc` / `.cpp` / `.h` must appear
  in some `target_sources(...)` call reachable from the qgroundcontrol
  target.
- **R-CMAKE-02** *(MAJOR)*: any header containing `Q_OBJECT` must live under
  a target with `AUTOMOC ON`. Resolved via `cmake-file-api` query, cached
  per-build.

### Failure mode 2 — Q_OBJECT / MOC / signals

- **R-MOC-01** *(BLOCKER)*: any new class inheriting `QObject` (or
  `QAbstractListModel`, etc.) must have `Q_OBJECT` on the first line of the
  class body.
- **R-MOC-02** *(MAJOR)*: any class exposed to QML (referenced by
  `qmlRegisterType<T>` or returned from a `Q_INVOKABLE` / property) must
  have `QML_ELEMENT` or an explicit `qmlRegisterType` registration in the
  same TU's init path.
- **R-MOC-03** *(MAJOR)*: every `Q_PROPERTY` must have a matching getter;
  if `WRITE` is declared, a matching setter; if `NOTIFY` is declared, a
  signal with the named signature exists.

### Failure mode 3 — custom/ shadow not used

- **R-SHADOW-01** *(BLOCKER)*: diff touches `src/**` only if the issue's
  frontmatter contains `allow_upstream_edit: true`. Default is shadow-only
  — agent must place changes under `custom/src/...` mirroring the upstream
  path.
- **R-SHADOW-02** *(MAJOR)*: if a `custom/src/...` file exists that shadows
  an upstream file, the issue's file manifest must list the upstream
  original under `shadowed_originals` so the next weekly rebase audit can
  flag drift.

### Failure mode 4 — QML imports / ScreenTools / QGCPalette

- **R-QML-01** *(BLOCKER)*: every changed/added `.qml` passes `qmllint`
  (Qt-version-pinned via toolchain lookup in `build/`).
- **R-QML-02** *(MAJOR)*: no numeric literals in `width` / `height` /
  `font.pixelSize` / spacing inside changed QML — must reference
  `ScreenTools.defaultFontPixelHeight|Width`. AST check via
  `qmlformat --json`.
- **R-QML-03** *(MAJOR)*: no hex color literals or `Qt.rgba(...)` in
  changed QML — must come from `QGCPalette`. Same AST pass.
- **R-QML-04** *(MAJOR)*: every `import` statement in a changed QML
  resolves against the configured Qt + qrc-registered modules.

### Orchestration-level rules

- **R-BUILD-01** *(BLOCKER)*: `cmake --build build --target qgroundcontrol`
  succeeds. Incremental build; expected duration 5-30s per small slice. The
  brief's `verification_command` is necessary but never sufficient — this
  rule is the real Qt completion check.
- **R-TR-01** *(MAJOR)*: user-visible strings in changed C++/QML are
  wrapped in `tr(...)` / `qsTr(...)`. AST pass; allowlist for log strings.
- **R-NULL-01** *(BLOCKER)*: every new dereference path through
  `MultiVehicleManager::instance()->activeVehicle()` is null-checked.
  Re-runs the existing `vehicle-null-check` pre-commit hook on the slice
  diff so findings surface in the same format as everything else.

### Rule activation and overrides

Each rule's `applies_to` is cheap and pre-filters. A docs-only slice runs
zero rules in <1s. The dominant cost on a real Qt slice is R-BUILD-01,
which we want anyway.

`rules.toml` is the default severity table shipped with the module — it
lists every rule ID with its default severity. Per-issue overrides live in
the issue's frontmatter (e.g., `rule_overrides: { R-TR-01: MINOR }` for an
issue intentionally pre-translation) and merge over the defaults at
`verify-slice` runtime. Overrides **must** be in the issue frontmatter, not
declared by the agent, so an agent cannot silence a rule mid-slice.

## Data Flow

```text
[1] Conversational drafting (user + orchestrator session)
    User prose + qt_orchestration.scan(...) → draft_issue.render(...)
    → reviewed and approved by user
[2] gh issue create --repo Strattoon/qgroundcontrol
    Issue body contains: frontmatter (allow_upstream_edit,
      shadowed_originals, rule_overrides), file manifest, acceptance criteria
      tied to rule IDs.
[3] /orchestrate <issue#>
    Fetches issue → qt-orchestration expand-brief --issue <path>
    Renders slice-brief-qt.md.j2 on top of refactor-brief-template.md,
    fills {{qt_completion_block}} with the verification commands for the
    rules that will apply.
    Brief written to .orchestrator/briefs/<slice-slug>.md.
[4] jcode run -C <cwd> -p auto "$(cat .orchestrator/briefs/<slice-slug>.md)"
    Codex executes the slice. Brief instructs Codex to self-verify with
    qt-orchestration verify-slice before declaring done. Structured output
    includes a qt_verification section reporting which rules passed.
[5] /orchestrate gate (independent re-check, not trusting Codex self-report)
    Runs qt-orchestration verify-slice --diff <slice-ref>.
    Exit 0 → slice flips green, commit, next slice.
    Exit non-zero → findings written, REFACTOR brief generated,
      re-dispatched. Retry cap = 2; third failure halts + escalates.
[6] Issue close
    All slices green AND a final verify-slice --diff <issue-base>..<issue-head>
    passes across the merged range (catches inter-slice drift).
```

**Per-issue artifacts** (in-repo, under `.orchestrator/work/<issue-slug>/`):

- `manifest.json` — original scan result, immutable. Diffed against actual
  final manifest at issue close.
- `verify-log.jsonl` — append-only line per `verify-slice` invocation.
  Schema:

  ```json
  {"ts": "...", "slice": "...", "issue": 42, "exit": 0,
   "rules_run": ["R-QRC-01", "R-MOC-01"],
   "findings": [{"rule": "...", "severity": "...", "path": "...",
                 "line": 0, "message": "..."}],
   "retry_count": 0}
  ```

  This is the source of truth for "did this slice really pass." If
  `state.md` says green but the last `verify-log` entry was a failure,
  `/orchestrate` refuses to close the issue.

**Diff scoping:** each `verify-slice` call is scoped to that slice's diff
against the previous slice's commit — not against `master`. Otherwise
findings from earlier slices keep re-firing. `/orchestrate` already tracks
slice commits in `state.md`; `verify-slice` reads from there.

## Error Handling

### A. Rule-finding failures (expected case)

- BLOCKER → mandatory retry. `/orchestrate` writes
  `.orchestrator/work/<slice>/qt-findings.md` with `path:line`, `message`,
  `fix_hint`, rule ID. REFACTOR brief generated scoped to those files with
  findings inlined and re-dispatched.
- MAJOR → same retry behavior by default; issue can downgrade via
  `rule_overrides` in frontmatter.
- MINOR/NIT → recorded in `verify-log.jsonl` but slice flips green.
  Surfaces at issue close as an aggregate report.
- **Retry cap = 2.** Third failure halts the slice, comments findings on
  the GitHub issue, marks `state.md` as `blocked`, stops dispatching.

### B. Infrastructure failures (verify-slice itself can't run)

- Missing `build/` configure → exit 2 with instructions. `/orchestrate`
  runs configure once and retries; second failure halts + escalates.
  Missing build dir is **never** treated as "no findings."
- `qmllint` / `cmake-file-api` missing → exit 3, halt + escalate
  immediately. Environment bug, not slice bug.
- Compiler error from R-BUILD-01 attributable to the slice diff → BLOCKER
  finding, retry-eligible. Compiler error that survives reverting the slice
  diff → exit 4, halt + escalate (tree was broken before this slice).

### C. Definitional failures (rule set itself is wrong)

- Codex's structured output may include a `qt_verification_disputes`
  section with rule ID + reasoning instead of fixing. `/orchestrate` does
  **not** auto-honor disputes; surfaces them in
  `.orchestrator/work/<slice>/disputes.md` and halts. User triages: either
  confirm false positive (and add a fixture test against `rules.py` so the
  rule does not fire that way again) or override Codex.
- A rule firing on >50% of slices in a single issue gets auto-flagged at
  issue close as "candidate for refinement." Keeps `rules.py` honest over
  time without hand-wavy complaints.

Codex's self-verify-before-done is **advisory** — there so Codex catches
its own mistakes early. The orchestrator's independent re-run is what
actually gates green.

## Testing

### Layer 1 — Unit tests per rule

- Each rule paired with `<rule_id>_pass/` and `<rule_id>_fail/` fixture
  directories: synthetic mini-tree (few files, small CMakeLists, small qrc)
  - expected `findings.json`. Tests apply the rule, assert findings match.
- Fixtures under `tests/fixtures/rules/<rule_id>/`, small enough to read in
  one screen. Doubles as living documentation.

### Layer 2 — Golden-file tests on scanner + brief expander

- Fixture issues + fixture repo snapshots → assert produced manifest /
  brief is byte-identical to checked-in golden. `--update-goldens` flag.

### Layer 3 — End-to-end against the real repo (marked `slow`, opt-in)

- Replays a recorded Phase-1 slice diff (e.g.,
  `phase1-slice16-strings-audit` artifacts on disk) and asserts
  `verify-slice` produces expected findings.
- **Acceptance bar:** at least one fixture per failure mode (QRC drift,
  MOC issue, custom/ shadow miss, QML import rot) drawn from real or
  synthesized Phase-1-style slices. Without these, the module is hopeful
  infrastructure, not proven.
- Not in per-PR CI (Qt build is slow); runs locally + nightly. Per-PR CI
  runs Layer 1 + 2 only.

### Layer 4 — /orchestrate integration smoke

- Scripted run: synthetic issue → `expand-brief` → fake `jcode` shim
  (writes deliberately-broken diff) → `verify-slice` → assert findings
  file written, retry brief generated, escalation triggers at retry-cap.
  `--dry-run` jcode shim so no Codex credits are burned.

### Rule overrides + disputes

- Override path: fixture issue with `rule_overrides: { R-TR-01: MINOR }`
  triggering R-TR-01 → assert exit 0, finding logged at MINOR.
- Dispute path: fixture Codex output with `qt_verification_disputes` →
  assert `/orchestrate` halts with dispute surfaced, no retry attempted.

### Coverage targets

- `rules.py` ≥ 90% line coverage (load-bearing).
- `scan.py` / `expand_brief.py` ≥ 80%.
- `verify_slice.py` covered via Layer 4 integration rather than unit
  coverage chasing.

## Design Choices Flagged

- **Incremental builds for R-BUILD-01.** `build/` stays warm between
  slices; expected duration 5-30s. Alternative (clean build, 3-5 min per
  slice) gives stronger isolation but is rejected for now on cost grounds.
  Revisitable if incremental builds prove to mask real failures.
- **Codex self-verify is advisory, not mandatory.** Independent gate
  re-runs the rules regardless. Revisitable if Codex's self-reports prove
  reliably correlated with gate pass — would let us skip the gate for
  obvious-pass cases.

## Acceptance Criteria

This spec is considered implemented when:

1. `tools/qt_orchestration/` package exists with the three entry points,
   `rules.py` covering all 11 rules above, all unit and golden-file tests
   green.
2. Layer 3 e2e fixtures exist for all four failure modes and pass.
3. `/orchestrate` invokes `verify-slice` after every slice and refuses to
   close an issue with a failed last-`verify-log` entry.
4. At least one Phase-2 issue has been authored conversationally using
   `draft_issue` and successfully driven through to close end-to-end.
5. `.orchestrator/templates/refactor-brief-template.md` is extended with a
   `{{qt_completion_block}}` placeholder and the
   `<structured_output_contract>` is extended with `qt_verification` (the
   pass/fail self-report) and optional `qt_verification_disputes` (Codex's
   false-positive reasoning). Existing contract sections are unchanged.

## Out of Scope

- Rewriting `/orchestrate` itself — only adding the verify-slice hook
  point + retry/escalation logic around it.
- New review-rubric severities — we reuse BLOCKER/MAJOR/MINOR/NIT.
- LLM-driven rule generation — `rules.py` is hand-coded Python; rule
  changes go through PRs like any other code.
- Replacing the existing brief template — we extend it via
  `{{qt_completion_block}}`, not replace it.
