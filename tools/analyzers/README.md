# QGC Static Analyzers

Static analysis tools for QGroundControl C++ code.

Compiler analysis records per-file durations as each file completes in `*-progress.jsonl`,
so interrupted CI jobs retain partial timing evidence. The final `*-timings.json` includes
elapsed time, accumulated worker time, and shard coverage. PR clang-tidy runs split
selected translation units across four jobs, after header dependency expansion. Each unit is
checked once; Clazy and manual runs use one job. Only the first shard saves shared build caches.

Per-check profiling adds substantial overhead and is disabled by default. Use
`python tools/analyze.py --tool clang-tidy --profile-checks src/LogManager/LogEntryTableModel.cc`
or the manual workflow's `profile_checks` input with a narrow path and `analyze_all: false`.
Profiled runs print the most expensive checks and archive the original profiles. Check costs
are summed across translation units and overlap with parallel work. Ordinary runs retain
per-file timing and progress without instrumenting individual checks. Changing `.clang-tidy`
requires a full clang-tidy scan but does not independently force a full Clazy scan.

## Vehicle Null-Check Analyzer

Detects unsafe patterns where `activeVehicle()` or `getParameter()` results are used without null checks.

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
| `unsafe_get_parameter` | Medium | `getParameter(...)->rawValue()` |

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
pre-commit run vehicle-null-check --all-files
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

## Shared Utilities

The analyzers use shared patterns from `tools/common/`:

- `patterns.py` - Regex patterns for QGC code constructs
- `file_traversal.py` - File discovery with proper filtering

## Adding New Analyzers

1. Create new Python script in `tools/analyzers/`
2. Import shared patterns from `tools.common.patterns`
3. Use `find_cpp_files()` from `tools.common.file_traversal`
4. Support `--json` output for editor integration
5. Add to `.pre-commit-config.yaml` if appropriate
