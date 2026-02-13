# QGroundControl Coding Style Examples

Example files demonstrating QGroundControl coding conventions.

| File | Shows |
|------|-------|
| `CodingStyle.h` | Header conventions (includes, naming, Qt6 macros) |
| `CodingStyle.cc` | Implementation conventions (logging, C++20, error handling) |
| `CodingStyle.qml` | QML conventions (imports, property order, signals) |

**Full guide:** [CODING_STYLE.md](../../CODING_STYLE.md)

**Auto-formatting:** `clang-format -i <file>` or `python3 ./tools/pre_commit.py` (uses [.clang-format](../../.clang-format))
