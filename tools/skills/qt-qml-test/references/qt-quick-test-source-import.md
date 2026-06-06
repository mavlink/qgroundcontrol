# Qt Quick Test — source import resolution

How to emit the `<source-import>` placeholder in the canonical
template — the import line that makes the component under test
visible to the test file.

Pick exactly one of the following forms and resolve in order;
stop at the first match.

## Form 1 — Declared module URI (library backing target only)

If the nearest `CMakeLists.txt` walking up from the source
directory contains `qt_add_qml_module(<target> ... URI <URI> ...)`,
check how `<target>` was declared:

- **`qt_add_library(<target> ...)`** (or plain `add_library`) —
  the module produces a linkable `<target>plugin` that a test
  binary can link to. Emit:

  ```qml
  import <URI>
  ```

- **`qt_add_executable(<target> ...)`** — the module is built
  into the executable; the auto-generated `<target>plugin`
  does not exist as a linkable library, so URI-based imports
  cannot be resolved from a sibling test binary. Do not emit
  `import <URI>` in this case; use Form 2 (relative directory
  import) instead. The QML engine reads source files from
  disk through the relative path and resolves sibling types
  via the on-disk `qmldir` (when present). For most
  executable-backed projects this works **without any
  refactor** — do not mention the "module-on-executable
  refactor" in the final reply.

  The refactor (splitting the module into a `STATIC` library)
  is only required when a test specifically needs
  `import <URI>` — for example, to exercise a singleton
  registered via `QT_QML_SINGLETON_TYPE TRUE` in CMake, or
  types that are not listed in any on-disk `qmldir`. In those
  rare cases, surface the refactor in the final reply and
  point at the `qt-qml-test-run` skill which can apply it.

A standalone `qmldir` containing `module <URI>` without a
matching `qt_add_qml_module(... URI <URI>)` is rare in modern
Qt 6 projects and usually leftover from an older layout;
treat it as advisory only.

## Form 2 — Relative directory import

Otherwise (or as the fallback from Form 1's executable case),
the source is reached via file path. Compute the relative
path from the test file's directory to the source file's
directory and emit:

```qml
import "<relative-path>"
```

For the default destination (`tests/tst_<X>.qml` next to a
source at the project root), this is `import ".."`. For a
source under `app/` it is `import "../app"`.

## Hard rule

**Never emit `import my_module` literally** — it is a
documentation placeholder, not a valid import. If neither
resolution form is determinable (e.g., the test target path
overrides the default and the source path can't be reduced
to a relative directory), stop and ask the user for the
correct import line rather than guessing.
