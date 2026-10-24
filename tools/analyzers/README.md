# QGC Static Analyzers

Backends for [`tools/analyze.py`](../analyze.py) and QGC-specific static analyzers for C++ code.
For `analyze.py` usage and compiler-analysis prerequisites, see
[tools/README.md](../README.md#analyzepy); for CI planning, sharding, and result caching, see
[ci-overview.md](../../.github/ci-overview.md).

## Compiler Analysis Output

Compiler analysis (clang-tidy, Clazy) writes these files to the build directory:

- `<tool>-progress.jsonl` records each file's duration and status as it completes, so
  interrupted CI jobs retain partial timing evidence.
- `<tool>-timings.json` adds selection, elapsed and accumulated worker time, shard coverage,
  result-cache statistics, and (when profiling) per-check costs.
- `<tool>-diagnostics.json` lists each unique diagnostic once (line, column, severity, and check,
  by file relative to the checkout, or `build/...` for generated files), so reports can count a
  header's warnings once across translation units and shards. `commit` names the checkout whose
  line numbers they use (`GITHUB_SHA`, which on a pull request is GitHub's merge commit rather
  than the PR head, else `git rev-parse HEAD`; absent when neither is known). With `PR_HEAD_SHA`
  set (Code Analysis on a pull request), `changed` repeats those on lines the pull request changed
  (`common.git.changed_ranges` against the PR's diff base).
- `<tool>-raw.txt` keeps every translation unit's full output; the console shows each identical
  diagnostic block once.

`--cache-dir` (`result_cache.py`) replays results whose tool, configuration, compile command, and
preprocessed inputs are unchanged; tool failures, timeouts, and crashes always run again.

Per-check profiling adds substantial overhead and is disabled by default. Use
`python tools/analyze.py --tool clang-tidy --profile-checks src/LogManager/LogEntryTableModel.cc`
or the manual workflow's `profile_checks` input with a narrow path and `analyze_all: false`.
Profiled runs print the most expensive checks and archive the original profiles under
`clang-tidy-profiles/`. Check costs are summed across translation units and overlap with parallel
work. Ordinary runs retain per-file timing and progress without instrumenting individual checks.

## clang-tidy Fixes

`python tools/analyze.py --tool clang-tidy --fix [PATH...]` runs clang-tidy with
`--export-fixes` for each selected translation unit, writing `clang-tidy-fixes/*.yaml` in the build
directory, then merges them with `clang-apply-replacements --ignore-insert-conflict`. Headers shared
by several translation units therefore receive each fix once, which running `clang-tidy --fix` in
parallel cannot guarantee. The `clang-apply-replacements` beside `clang-tidy` is preferred (both ship
in LLVM packages and the pinned `clang-tidy` PyPI wheel), then `PATH`.

- Fix runs bypass `--cache-dir`: a replayed result would export no fixes.
- Header fixes are limited to this checkout's `src/`, `test/`, and `tools/`, because CPM sources and
  generated headers in other `src/` directories also match `.clang-tidy`'s `HeaderFilterRegex`.
- Fixes from translation units that failed to compile are discarded, as `clang-tidy --fix` does.
- Fixed code is not reformatted; run `just format-fix`, review with `git diff`, and rerun without
  `--fix` for findings that have no automatic fix.
- `analyze.py` refuses `--tool clang-tidy --fix` when `CI` or `GITHUB_ACTIONS` is set; CI only
  reports findings.

## clang-format Hook

`clang_format.py` also runs as the `clang-format` pre-commit hook
(`python3 tools/analyzers/clang_format.py [--fix] FILE...`). It checks, or with `--fix` formats, only
the lines that differ from the PR merge base (pre-commit's `PRE_COMMIT_FROM_REF` and
`PRE_COMMIT_TO_REF`) or, locally, from `HEAD`. New files are checked in full and deletion-only
changes are skipped, as in `analyze.py --tool clang-format`.

## Vehicle Null-Check Analyzer

Detects unsafe patterns where `activeVehicle()` results are used without null checks.

### Usage

```bash
# Analyze specific files
python3 tools/analyzers/vehicle_null_check.py src/Vehicle/*.cc

# Analyze directory recursively
python3 tools/analyzers/vehicle_null_check.py src/

# JSON output for CI/editor integration
python3 tools/analyzers/vehicle_null_check.py --json src/

# Show help
python3 tools/analyzers/vehicle_null_check.py --help
```

### Detected Patterns

| Pattern | Risk | Example |
| --- | --- | --- |
| `unsafe_active_vehicle_direct` | High | `activeVehicle()->method()` |
| `unsafe_active_vehicle_use` | High | Variable used after `activeVehicle()` without check |

### Example Output

```text
src/Example.cc:42:15: warning: unsafe_active_vehicle_direct
  activeVehicle()->parameterManager()->getParameter(...);
  Suggestion: Add null check before using activeVehicle():
  Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
  if (!vehicle) return;
```

### JSON Output Format

```json
[
  {
    "file": "src/Example.cc",
    "line": 42,
    "column": 15,
    "pattern": "unsafe_active_vehicle_direct",
    "code": "activeVehicle()->parameterManager()->...",
    "suggestion": "Add null check before using activeVehicle():..."
  }
]
```

### Pre-commit Integration

Already configured in `.pre-commit-config.yaml`. Runs automatically on C++ files.

```bash
# Run manually on all files
prek run vehicle-null-check --all-files
```

### Suppressing False Positives

The analyzer looks back 10 lines for null checks. If you have a valid null check that isn't detected, restructure your code to have the check closer to the usage, or add a comment explaining why it's safe.

```cpp
// Safe: null check is visible to analyzer
Vehicle *vehicle = MultiVehicleManager::instance()->activeVehicle();
if (!vehicle) {
    return;
}
vehicle->doSomething();  // OK - analyzer sees the check above
```

## QT_TRANSLATE_NOOP Check

`qt_translate_noop_check.py` reports `QT_TRANSLATE_NOOP`/`QT_TR_NOOP` results that are returned
or used as runtime strings (for example in `.arg()` or `QString(...)`). These macros only mark
strings for extraction; translate at the call site with `QCoreApplication::translate()` or `tr()`.
It runs as the `qt-translate-noop-check` pre-commit hook and, like the null-check analyzer, takes
files or directories and `--json`.

## qmlformat Check

`qmlformat.py` runs Qt's `qmlformat` (from `PATH`, like qmllint) on each file in stdout mode with the
nearest `.qmlformat.ini` and prints a unified diff where the output differs; it never rewrites files.
It backs `analyze.py --tool qmlformat` and the manual-stage `qmlformat` hook
(`prek run qmlformat --hook-stage manual --files <file.qml>`). Apply a result with `qmlformat -i`.

## Shared Utilities

`base.py` defines `AnalysisResult` and `AnalyzerBase` for `analyze.py` backends, and
`run_line_check_cli()`, the shared command line (paths and `--json`) of the two line-based checks.
File discovery uses `file_traversal.py` from [`tools/common/`](../common/README.md).

## Adding New Analyzers

1. Create a new Python script in `tools/analyzers/` and bootstrap imports as
   `vehicle_null_check.py` does (see [Bootstrapping Imports](../common/README.md#bootstrapping-imports))
2. Use `find_cpp_files()` from `common.file_traversal`
3. For an `analyze.py --tool`, subclass `analyzers.base.AnalyzerBase` and add one entry to
   `ANALYZERS` in `tools/analyze.py`
4. For a line-based check, fill `AnalysisResult.findings` and define
   `main(argv)` as `run_line_check_cli(YourAnalyzer, argv)`
5. Add tests under `tools/tests/`, and add to `.pre-commit-config.yaml` if appropriate, running it
   through `uv run --frozen --project tools --group scripts python` like the other hooks
