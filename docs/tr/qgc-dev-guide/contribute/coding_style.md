# Coding Style

The canonical style guide is [CODING_STYLE.md](https://github.com/mavlink/qgroundcontrol/blob/master/CODING_STYLE.md) in the source tree. High level style information:

- Indent with 4 spaces (no tabs); lines are at most 120 columns.
- `PascalCase` class names, `camelCase` methods and variables, `_leadingUnderscore` private members.
- C++20 and Qt 6 idioms: always null-check `Vehicle*`/`activeVehicle()`, use categorized logging (`qCDebug`/`qCWarning`), and no `Q_ASSERT` in production code.
- QML uses QGC controls, `ScreenTools` sizes and `QGCPalette` colors instead of hardcoded values, and wraps user-visible strings in `qsTr()`.

The style itself is demonstrated in the following example files:

- [CodingStyle.cc](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.cc)
- [CodingStyle.h](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.h)
- [CodingStyle.qml](https://github.com/mavlink/qgroundcontrol/blob/master/tools/coding-style/CodingStyle.qml)

Formatting and linting are enforced by pre-commit hooks (clang-format, qmllint, and QGC-specific checks); run `just lint` before you push.
Only format the lines you change: whole-file reformatting of existing files creates unrelated diffs that make pull requests harder to review.
