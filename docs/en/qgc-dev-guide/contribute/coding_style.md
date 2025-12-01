# Coding Style

See the full **[Coding Style Guide](https://github.com/mavlink/qgroundcontrol/blob/master/CODING_STYLE.md)** for comprehensive documentation.

## Quick Reference

- **Indentation**: 4 spaces (no tabs)
- **Naming**: PascalCase for classes, camelCase for methods/variables
- **Private members**: Prefix with underscore (`_myVariable`)
- **C++ Standard**: C++20
- **Qt Version**: Qt 6.10+

## Example Files

The coding style is demonstrated in these reference files:

- [CodingStyle.h](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.h) - C++ header patterns
- [CodingStyle.cc](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.cc) - C++ implementation patterns
- [CodingStyle.qml](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.qml) - QML patterns

## Key Guidelines

### C++
- Use `[[nodiscard]]` for functions with important return values
- Use `std::span` instead of pointer + size parameters
- Use defensive checks instead of `Q_ASSERT` (removed in release builds)
- Use `QGC_LOGGING_CATEGORY` for logging

### QML
- No hardcoded sizes - use `ScreenTools`
- No hardcoded colors - use `QGCPalette`
- Use QGC controls (`QGCButton`, `QGCLabel`, etc.)
- Use `qsTr()` for all user-visible strings
- Use function syntax for Connections: `function onSignal() { }`

### Common Pitfalls
1. Assuming single vehicle - always null-check `activeVehicle()`
2. Accessing Facts before `parametersReady` signal
3. Using `Q_ASSERT` in production code
4. Hardcoded sizes/colors in QML
