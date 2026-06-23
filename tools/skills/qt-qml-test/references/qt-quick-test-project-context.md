# Qt Quick Test — project-context bounded reads

The minimum set of project files the skill should read to
generate a correct test. Read each only when present in the
project tree, and stop:

1. **The source QML file under test** (always).
2. **Custom components the source QML directly imports** —
   e.g. the `MyButton.qml` in `import "../widgets" as W` used
   as `W.MyButton`, or referenced by type name from a sibling
   path. Read each such file **once** to learn its declared
   properties and signals. Do **not** recurse into their own
   imports.
3. **The module's `qmldir`** if it exists in the same
   directory as the source. Extract the `module <URI>` line
   if present — it is the canonical module URI for the source
   import.
4. **The nearest `CMakeLists.txt`** walking up from the
   source directory, but only to grep for a
   `qt_add_qml_module(... URI <uri> ...)` call that covers
   the source. Read just enough to capture the `URI`
   argument; do not interpret anything else in the file.

Do not read framework files (Qt installation, third-party QML
modules).

If a property or signal cannot be resolved from these
bounded reads, follow rule 40 in SKILL.md (exclude tests for
those properties). The reading is opportunistic: if a file
is not present or not accessible, generate a test for what
is known and proceed.
