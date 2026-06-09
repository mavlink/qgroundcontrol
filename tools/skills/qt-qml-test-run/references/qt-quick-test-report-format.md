# Qt Quick Test — Markdown report format

Specification for the Markdown report the runner skill writes
at Step 9. The skill produces one file per run named
`build/tests/reports/test-report-<YYYY-MM-DD-HHMMSS>.md`, using
the same timestamp as the corresponding JUnit XML at
`build/tests/reports/junit/qmltests-<timestamp>.xml`. Both
land under the build folder so they ride along with other
build artifacts (already excluded from version control by
convention) instead of polluting the source tree.

## Framing

The report is a **standalone diagnostic** of this run. Do
not frame it as a *quality comparison* with prior runs
("better than last time", "regressed by N tests") even if
older reports are present in the directory. *Change
detection* is a separate matter: when there are failures,
prior-run timestamps are a useful signal for distinguishing
real regressions from environmental flakiness, and the report
includes a dedicated section for that (Section 4 below).

**Write the report for a reader who has no access to the
skill.** Do not refer to "the skill", "this runner", or any
similar meta-reference. State guidelines as facts where they
need to reach the reader.

## Sections

1. **Header**
   - Project name (from the root CMakeLists.txt `project()`
     call, or the directory name as a fallback).
   - Qt version (extracted from the resolved `<qt-path>` —
     e.g., `6.11.0` from `/opt/Qt/6.11.0/gcc_64`).
   - Run mode (CMake / Standalone / Direct).
   - Invocation timestamp.
   - Path to the JUnit XML report.

2. **Run setup** — what to copy/paste to reproduce this run:
   - **Invocation** — the exact command line in a fenced
     block, including any environment variables prepended
     (e.g. `QT_QPA_PLATFORM=offscreen ./build/tests/tst_qmltests
     -o <report.xml>,junitxml`, or `<qt-path>/bin/qmltestrunner
     -input <tests-dir> -o <report.xml>,junitxml` for the
     Direct / Standalone paths).
   - **Test root** — directory passed to `-input` or
     configured via `QUICK_TEST_SOURCE_DIR`.
   - **Skipped subdirectories** (omit when none) — any
     `tests/skipped/`, `tests/disabled/`, etc. excluded via
     `-input <leaf-dir>` scoping at Step 7, with a one-line
     note on why (e.g. "contains a hanging file").
   - **Extra `-import` paths** (omit when none) — any
     `-import <path>` flags needed for the tests' imports to
     resolve.

3. **Summary table** — total / passed / failed / skipped /
   duration in seconds. Lead with a one-line verdict:
   - 0 failed, 0 skipped → "All N tests passed."
   - F failed → "F of N tests failed."
   - S skipped only → "All non-skipped tests passed (S
     skipped)."

4. **Source changes since prior run** (omit when no failures,
   or when no prior JUnit XML report exists) — discover via:

   - Find the most recent prior `qmltests-*.xml` under
     `build/tests/reports/junit/` (excluding the current run's
     file). Use its mtime as the baseline.
   - List project source files (e.g. `*.qml`, `*.cpp`, `*.h`,
     `*.hpp`, `CMakeLists.txt`) under the project root with
     mtime newer than the baseline. Exclude `build*/` (covers
     the report directory itself) and `.git/`.
   - Render the matches as a bulleted list of relative paths
     under a one-line lead, e.g. "Source files modified since
     the prior run at HH:MM:SS:".
   - If a Git repository is detected (`.git` exists), also
     include `git diff --stat` since the baseline commit when
     resolvable (e.g. `git log -1 --before=<baseline-mtime>
     --format=%H`); otherwise fall back to `git status
     --short`.

   When this section has any entries, frame the failure
   analysis (Section 5) as **"likely regression in the listed
   files"** before exploring environmental causes. Read the
   diff and look for changes that plausibly explain each
   failed assertion. Only fall back to environmental /
   flakiness hypotheses when no source change can explain the
   failure.

5. **Failed tests** (omit section when no failures) — for
   each failed case:
   - Full name (`classname::name`)
   - `failure_message` verbatim, in a fenced block
   - `source` (file:line[:col]) if present, formatted as a
     Markdown link with the line number visible
   - One-line suggested next step: "Inspect the test
     function in `<source>`" or "Re-run with
     `QT_LOGGING_RULES='*=true'` to capture more context". If
     Section 4 lists changed files, prefer "Inspect `<file>`
     at the lines changed since the prior run".

6. **Slowest tests** — top 10 by `time_ms`, with a column
   header note: "`time_ms` includes test setup and teardown,
   not just the assertion." Flag any case above 1000 ms with
   a `⏱` (or `[slow]` if avoiding emoji) and one-line hint:
   "candidate for `tryCompare` audit — see the
   `qt-qml-test` skill's pitfalls reference."

7. **Skipped tests** (omit section when none) — name +
   reason if the runner emitted one.

8. **AI-assistance footer** — end the report with the exact
   line:

   > AI assistance has been used to create this output.

   This must always be present, regardless of result.

## Parser output

The runner skill's Step 8 invokes
`references/scripts/parse-qmltestrunner-output.py` on the
JUnit XML and consumes the JSON summary it prints. The
script's own docstring is the source of truth for the
schema (`total`, `passed`, `failed`, `skipped`,
`duration_ms`, `cases[]`, `slowest[]`).

On Windows the interpreter may be `python` instead of
`python3`; retry with `python` if the first attempt fails.

When the parser exits non-zero it writes `{"error": "..."}`
to stdout. Map the message to a cause:

- `"Report file not found"` → wrong path; re-check Step 7.
- `"Failed to parse XML"` → runner crashed mid-write; rerun
  with the JUnit format flag.
- `"No <testcase> elements found"` → test directory empty or
  not discovered; check `QUICK_TEST_SOURCE_DIR` or `-input`.
- Every case fails with `"Type X unavailable"`,
  `"No such file or directory"` for `qrc:/...`, or
  `"<SiblingType> is not a type"` → URI imports against the
  project module hit the module-on-executable case; refactor
  per [qt-quick-test-cmake.md § Module-on-executable refactor](qt-quick-test-cmake.md#module-on-executable-refactor).
  Relative-import variant → verify paths and on-disk
  `qmldir`.

Do not proceed to the Markdown report with an empty parser
result.

## Console summary

After writing the Markdown report (or skipping it under
`--no-report`), display to the user:

- Verdict line (passed / failed / skipped count, run
  duration).
- First 3 failures with one-line summary each (full detail
  is in the report).
- When failures exist AND the report's Section 4 listed
  changed source files, prefix the failures block with one
  short line: "Source files modified since prior run:
  `<file1>`, `<file2>` — failures are likely regressions;
  inspect the diff first."
- Path to the Markdown report — or, when `--no-report` was
  passed, the line "Markdown report skipped (`--no-report`);
  JUnit XML retained at `<junit-path>`."

Keep console output concise. The detailed analysis lives in
the report file. Report **outcomes only** — verdict,
failures, paths — not the workflow that produced them
("per Step 4b", "applying wire-up", etc.). Answer directly
if the user asks why a path was chosen.

Apply the Framing rule from this file to the console
summary too: no overall quality comparison with prior runs,
even if asked "is it better now?".
