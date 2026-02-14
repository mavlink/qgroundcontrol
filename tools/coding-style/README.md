# QGroundControl Coding Style Examples

This directory contains example files demonstrating QGroundControl coding conventions.

## Files

| File | Description |
|------|-------------|
| `CodingStyle.h` | Header file conventions |
| `CodingStyle.cc` | Implementation file conventions |
| `CodingStyle.qml` | QML file conventions |

## Key Conventions Demonstrated

### C++ Header Files (`.h`)

- **Include Guards**: Use `#pragma once`
- **Include Order**: System → Qt → QGC headers (alphabetical within groups, blank lines between)
- **Qt Paths**: Use full paths (`QtCore/QObject` not `QObject`)
- **Forward Declarations**: For pointer/reference-only types
- **Logging Categories**: `Q_DECLARE_LOGGING_CATEGORY(ClassNameLog)`
- **Class Documentation**: Doxygen `///` comments
- **Naming**:
  - Classes: `PascalCase`
  - Members: `_leadingUnderscore`
  - Methods: `camelCase`
  - Q_PROPERTY: `camelCase`
- **Qt6 QML**: Use `QML_ELEMENT`, `QML_SINGLETON`, `QML_UNCREATABLE`
- **Modern C++**: `[[nodiscard]]`, `std::span`, `std::string_view`

### C++ Implementation Files (`.cc`)

- **Include Order**: Own header first, then system → Qt → QGC
- **Logging**: Use `QGC_LOGGING_CATEGORY` (not `Q_LOGGING_CATEGORY`)
- **C++20 Features**:
  - `std::ranges` algorithms
  - Designated initializers
  - Range-based for with init statement
  - Concepts and constraints
- **Error Handling**: Use `qCWarning`/`qCCritical` with logging category
- **Const Correctness**: Mark methods and parameters `const` appropriately

### QML Files (`.qml`)

- **Import Order**: QtQuick → Qt modules → QGC imports (versioned)
- **Property Order**: `id` → dimensions → anchors → other properties
- **Naming**:
  - IDs: `camelCase`
  - Properties: `camelCase`
  - JavaScript functions: `camelCase`
- **Signals**: Prefer declarative bindings over imperative `onXChanged`
- **Components**: Use `Loader` for conditional/heavy components

## Related Documentation

- **[CODING_STYLE.md](../../CODING_STYLE.md)** - Full coding style guide
- **[.clang-format](../../.clang-format)** - Automatic C++ formatting rules
- **[.clang-tidy](../../.clang-tidy)** - Static analysis configuration

## Formatting

Use clang-format to automatically format C++ code:

```bash
# Format changed files
./tools/format-check.sh

# Check formatting (CI mode)
./tools/format-check.sh --check

# Format all files
./tools/format-check.sh --all
```

## Static Analysis

Use clang-tidy for static analysis:

```bash
# Analyze changed files
./tools/analyze.py

# Analyze all files
./tools/analyze.py --all
```
