# QGroundControl Coding Style Examples

Reference files demonstrating QGroundControl's coding conventions. For the authoritative rules
(naming, include order, C++20, QML, logging), see **[CODING_STYLE.md](../../CODING_STYLE.md)** — these
files are worked examples of it.

## Files

| File | Demonstrates |
| --- | --- |
| `CodingStyle.h` | Header conventions: `#pragma once`, include order, forward declarations, `Q_DECLARE_LOGGING_CATEGORY`, QML macros, naming |
| `CodingStyle.cc` | Implementation conventions: include order, `QGC_LOGGING_CATEGORY`, C++20 features, const correctness |
| `CodingStyle.qml` | QML conventions: import/property order, naming, declarative bindings |

## Related Documentation

- **[CODING_STYLE.md](../../CODING_STYLE.md)** - Full coding style guide
- **[.clang-format](../../.clang-format)** - Automatic C++ formatting rules
- **[.clang-tidy](../../.clang-tidy)** - Static analysis configuration

## Formatting & Static Analysis

```bash
./tools/analyze.py --tool clang-format --fix        # Format changed files
./tools/analyze.py --tool clang-format              # Check formatting (CI mode)
./tools/analyze.py --tool clang-format --fix --all  # Format all files
./tools/analyze.py                                   # clang-tidy on changed files
./tools/analyze.py --all                             # clang-tidy on all files
```

Or use the `just` wrappers (`just format-fix`, `just analyze`) — see [tools/README.md](../README.md).
