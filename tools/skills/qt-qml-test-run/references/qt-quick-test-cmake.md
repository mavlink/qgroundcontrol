# Qt Quick Test — CMake wiring recipe

The recipe the skill writes when a project has no test
infrastructure, plus the detection rules and common failure
modes.

The skill always writes new files (`tests/CMakeLists.txt` and
`tests/main.cpp`). It **never** mutates the root
`CMakeLists.txt` silently — the proposed three-line addition
is printed for review and applied only with the user's
explicit OK.

## tests/CMakeLists.txt — `QUICK_TEST_MAIN` harness

The skill uses the C++ harness for all CMake projects. It
works whether the project ships its own QML module via
`qt_add_qml_module` or not, so a separate "direct mode" is not
needed.

Two variants are emitted depending on the project's modules.
**Pick the Widgets variant** if any of the following matches in
the project's root `CMakeLists.txt` `find_package(Qt6 ...
COMPONENTS …)` list, or in any `target_link_libraries` for
project targets:

- `Widgets` / `Qt6::Widgets`
- `Charts` / `Qt6::Charts` — QtCharts privately links Widgets
  and spawns `QWidgetTextControl` internally; without a
  `QApplication` the test binary segfaults at first chart draw.
- `WebEngineWidgets`, `WebEngineQuick` — same reason.
- `Multimedia` (when paired with widgets-based renderers).
- `PrintSupport`, `Pdf`, `PdfWidgets`.

Otherwise emit the **GuiApplication variant**.

### GuiApplication variant (default)

`tests/CMakeLists.txt`:

```cmake
qt_add_executable(tst_qmltests main.cpp)

target_compile_definitions(tst_qmltests PRIVATE
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(tst_qmltests PRIVATE
    Qt6::Gui
    Qt6::QuickTest
    # Add the project's backing module library here, e.g.:
    # MyAppLib
    # ${PROJECT_NAME}plugin
)

add_test(NAME tst_qmltests COMMAND tst_qmltests)
```

`tests/main.cpp`:

```cpp
#include <QtQuickTest>
#include <QCoreApplication>
#include <QObject>

class Setup : public QObject
{
    Q_OBJECT
public slots:
    void applicationAvailable()
    {
        // Required for QML Settings / QSettings to initialise
        // cleanly. Replace the strings with the project's identity
        // if it ships its own.
        QCoreApplication::setOrganizationName("QtProject");
        QCoreApplication::setOrganizationDomain("qt.io");
        QCoreApplication::setApplicationName("qmltests");
    }
};

QUICK_TEST_MAIN_WITH_SETUP(qmltests, Setup)

#include "main.moc"
```

### Widgets variant (Charts / Widgets / WebEngineWidgets / …)

Differs from the GuiApplication variant in two places:

1. `tests/CMakeLists.txt` links `Qt6::Widgets`:

   ```cmake
   target_link_libraries(tst_qmltests PRIVATE
       Qt6::Gui
       Qt6::Widgets
       Qt6::QuickTest
       # …project libraries…
   )
   ```

2. `tests/main.cpp` constructs `QApplication` explicitly before
   handing control to the runner — `QUICK_TEST_MAIN_WITH_SETUP`
   creates a `QGuiApplication` by default, which is not enough
   for code paths that touch `QWidget*`:

   ```cpp
   #include <QtQuickTest>
   #include <QApplication>
   #include <QObject>

   class Setup : public QObject
   {
       Q_OBJECT
   public slots:
       void applicationAvailable()
       {
           QCoreApplication::setOrganizationName("QtProject");
           QCoreApplication::setOrganizationDomain("qt.io");
           QCoreApplication::setApplicationName("qmltests");
       }
   };

   int main(int argc, char *argv[])
   {
       QApplication app(argc, argv);
       Setup setup;
       return quick_test_main_with_setup(argc, argv, "qmltests",
                                         QUICK_TEST_SOURCE_DIR, &setup);
   }

   #include "main.moc"
   ```

When emitting the Widgets variant, also add `Widgets` to the
project's root `find_package(Qt6 ... COMPONENTS …)` list at
Step 5c if it is not already present.

### Both variants

`QUICK_TEST_MAIN_WITH_SETUP` takes a class with
`applicationAvailable()` (and optionally `qmlEngineAvailable()`)
slots; the runner invokes them after the `QCoreApplication`
exists but before the first QML file loads. The org/domain/app
names are required by `QSettings` and the QML `Settings`
element; without them, every `Settings` instance prints "Failed
to initialize QSettings instance" at construction.

`QUICK_TEST_SOURCE_DIR` points at the directory containing
`tst_*.qml` files at configure time and is baked into the
binary, so moving the test files later requires a re-configure.

The commented `target_link_libraries` lines must be filled in
by the project owner **if the project has a backing C++/QML
module library** (anything declared via `qt_add_qml_module`
that tests need to instantiate). The skill cannot reliably
guess the backing-library target name; it surfaces this gap in
the console output and the run report. Projects without a
backing library can leave those lines commented and rely on
Qt-shipped QML modules only.

To run via the test executable instead of CTest:

```bash
./build/tests/tst_qmltests -o report.xml,junitxml
```

The flags accepted by the test executable are the same as
`qmltestrunner` (it embeds the runner).

## Root `CMakeLists.txt` — proposed addition

The skill never silently mutates the root file. With
`--wire-up`, it prints these three lines for confirmation:

```cmake
find_package(Qt6 REQUIRED COMPONENTS QuickTest)
enable_testing()
add_subdirectory(tests)
```

Add after any existing `find_package(Qt6 ...)` call, or merge
the `QuickTest` component into the existing call's `COMPONENTS`
list. The `add_subdirectory(tests)` line goes near the bottom,
after the project's main targets are defined.

## Wire-up procedure

When the runner's Step 5 needs to wire up missing test
infrastructure, apply the following sub-steps in order:

**5a — Write `tests/CMakeLists.txt`.** Pick the variant per
the GuiApplication vs Widgets criteria above and write the
matching template. Auto-fill the `target_link_libraries`
project-plugin lines per the next section; leave the
commented placeholder only when no library-backed
`qt_add_qml_module` calls are found. Create the `tests/`
directory if missing. **Never overwrite** an existing
`tests/CMakeLists.txt` — if the file is already there,
surface its content and stop with a "merge manually"
message.

**5b — Write `tests/main.cpp`.** Use the template matching
the variant chosen at 5a. Both variants use
`QUICK_TEST_MAIN_WITH_SETUP` with a `Setup` class that sets
organization / domain / application names (required by QML
`Settings` and `QSettings`). Do **not** emit the bare
`QUICK_TEST_MAIN(qmltests)` form. Same no-overwrite policy
as 5a.

**5c — Propose root `CMakeLists.txt` edits.** Print the
three-line addition above (and, for the Widgets variant,
also merge `Widgets` into the same `find_package`
`COMPONENTS` list). Show the existing root `CMakeLists.txt`
so the user can locate the right insertion points. Apply
**only after the user confirms with an explicit "yes" or
"apply"**.

**5d — Recipe-only path.** When the user reached Step 5 via
Step 4b explicitly asking for wire-up but did not pass
`--wire-up`, do not write or modify any files. Print the
`tests/CMakeLists.txt` template, the `tests/main.cpp`
template, the three-line root addition, and the instruction
"Re-run with `--wire-up` after reviewing, or apply
manually." Stop; do not proceed to Step 6.

## Binary-direct JUnit invocation (not `ctest --output-junit`)

For CMake projects, Step 7 invokes the built test binary
directly to get JUnit XML at per-QML-function granularity:

```bash
"./build/tests/tst_qmltests" -o "<report.xml>,junitxml"
```

Do **not** use `ctest --output-junit` as the parser source.
CTest aggregates JUnit output at the CTest-target level: a
test binary that runs 100+ QML test functions appears in the
XML as a single `<testcase name="tst_qmltests" ...>` entry.
The parser in Step 8 would then report "1 test passed" — or,
worse, "1 test failed" with no per-function breakdown — even
on a fully-passing suite.

The test binary's `-o report.xml,junitxml` form produces one
`<testcase>` per QML `function test_*()`, which is what the
parser and the Markdown report need. CTest is still useful
for a smoke pass (`ctest --test-dir build --output-on-failure`),
just not as the JUnit source.

## Detection patterns — is wiring already present?

Grep the project's CMakeLists.txt files (root and one level
deep). If **any** of these match, treat the test
infrastructure as present and skip wiring.

| Pattern (regex) | What it indicates |
|---|---|
| `find_package\([^)]*QuickTest` | `Qt6::QuickTest` is available |
| `quick_test_main\b` or `QUICK_TEST_MAIN` | C++ harness present |
| `QUICK_TEST_SOURCE_DIR` | Test source directory configured |

All three are QuickTest-specific — none of them fire on a
C++ QTest-only project. Generic CTest signals like
`enable_testing()` or `add_test(... tst_...)` are not used
for detection because a C++ QTest project sets both without
involving QML Quick Test.

Avoid matching `qt_internal_add_test` — that macro is **Qt
internal API** (private to Qt itself); user projects should
not use it. Its presence usually means the project is a Qt
module, not a typical user codebase, and the skill should
defer to the existing setup.

### Additional detection — backing target type

Separately from the wiring-already-present check, grep for
`qt_add_qml_module(<target>` and pair `<target>` with its
declaration:

- `qt_add_executable(<target> ...)` — module is built into the
  executable; **no linkable plugin exists** for tests to use.
  See "Module-on-executable refactor" below.
- `qt_add_library(<target> ... STATIC ...)` or
  `add_library(<target> STATIC ...)` — module backs a static
  library; the auto-generated `<target>plugin` is also static.
  **Direct `qmltestrunner` cannot load these modules**: at
  runtime it tries to `dlopen` the plugin, but a static plugin
  has nothing to load and the import fails with `module "<URI>"
  is not installed`. A custom test executable that links the
  static plugin target is the only working path. The skill must
  therefore skip the direct-mode offer for any test that uses
  `import <URI>` against a STATIC-backed module and route
  straight to Step 5 wire-up.
- `qt_add_library(<target> ... SHARED ...)` or unqualified
  `qt_add_library(<target> ...)` resolving to the default
  `BUILD_SHARED_LIBS` value — module backs a shared library; the
  auto-generated `<target>plugin` is loadable via `qmltestrunner
  -import <plugin-dir>`, but the test executable can also link
  it directly for less environmental setup.

This pairing only matters when the test is expected to use
`import <URI>` to reach the project's own QML types. If the
test only exercises Qt-shipped modules (`QtQuick.Controls`,
`Qt.labs.*`, etc.), no backing library is needed.

### Auto-filling project plugin links

When the project has one or more `qt_add_qml_module(<lib>
...)` calls backed by libraries, the skill should not leave the
`target_link_libraries` lines commented — it can enumerate every
such target from the project's CMakeLists.txt files and emit
both `<lib>` and `<lib>plugin` for each, e.g.:

```cmake
target_link_libraries(tst_qmltests PRIVATE
    Qt6::Gui
    Qt6::QuickTest
    AppCore
    AppCoreplugin
    AppWidgets
    AppWidgetsplugin
)
```

Leave the commented placeholder only when no
`qt_add_qml_module` libraries were found.

## Module-on-executable failure modes

When `qt_add_qml_module(<exe> URI ...)` is called on a
`qt_add_executable` target, no separate plugin is generated
and three downstream failures appear in test wiring:

1. **Linking a guessed `<exe-target>plugin`** — the target
   does not exist; the linker reports "cannot find
   -l<name>plugin".
2. **Falling back to `qmltestrunner -import <build-dir>`** —
   the auto-generated `build/<module>/qmldir` contains
   `prefer :/`, directing Qt to load module files from qrc.
   Those qrc copies live only inside the original executable,
   not the test binary; loads fail with "Type X unavailable:
   No such file or directory".
3. **Editing `prefer :/` out of the generated qmldir** — page
   files that reference sibling types (e.g. a `ButtonPage`
   inheriting `ScrollablePage`) by bare name still fail to
   resolve, because sibling-type resolution within a module
   relies on the module being registered in the *linking*
   binary, not located via on-disk qmldir from a sibling
   process.

The fix for all three is the same refactor below.

## Module-on-executable refactor

When the project declares `qt_add_qml_module(<exe> URI ...)`
with `<exe>` being a `qt_add_executable` target, no separate
plugin library is generated. The module registration is baked
into the executable. A test binary cannot link to this — there
is nothing to link to.

The fix is to split the QML module out of the executable into
a `STATIC` library that both the original executable and the
test binary link against.

**Before** (typical example layout):

```cmake
qt_add_executable(myapp main.cpp)

qt_add_qml_module(myapp
    URI MyApp
    NO_RESOURCE_TARGET_PATH      # only valid on executables
    QML_FILES Main.qml SubPage.qml
    RESOURCES icons/logo.png
)

target_link_libraries(myapp PUBLIC Qt6::Core Qt6::Quick)
```

**After**:

```cmake
qt_add_executable(myapp main.cpp)

qt_add_library(myappmodule STATIC)

qt_add_qml_module(myappmodule
    URI MyApp
    # NO_RESOURCE_TARGET_PATH removed — only valid on executables
    QML_FILES Main.qml SubPage.qml
    RESOURCES icons/logo.png
)

target_link_libraries(myappmodule PUBLIC Qt6::Core Qt6::Quick)

target_link_libraries(myapp PRIVATE
    myappmodule
    myappmoduleplugin           # auto-generated by qt_add_qml_module
)
```

In `tests/CMakeLists.txt`, link the same pair:

```cmake
target_link_libraries(tst_qmltests PRIVATE
    Qt6::Gui
    Qt6::QuickTest
    myappmodule
    myappmoduleplugin
)
```

Notes:

- `NO_RESOURCE_TARGET_PATH` is only valid when the backing
  target is an executable; remove it (or replace with
  `RESOURCE_PREFIX "/"`) when moving to a library.
- Singleton declarations (`set_source_files_properties(...
  QT_QML_SINGLETON_TYPE TRUE)`) must be set *before* the
  `qt_add_qml_module` call and now apply to the library
  target's sources.
- The auto-generated plugin name is `<library-target>plugin`.
  If the library is `myappmodule`, the plugin is `myappmoduleplugin`.

## Common failure modes after wiring

- **`find_package(Qt6 ... QuickTest)` missing** — `qt_add_executable`
  succeeds but `target_link_libraries(... Qt6::QuickTest)`
  fails with "Target Qt6::QuickTest not found". Fix: add
  `QuickTest` to the root `find_package` components list.
- **`main.moc: No such file or directory`** at compile time —
  the test binary's `main.cpp` declares `class Setup : public
  QObject { Q_OBJECT … };` and ends with `#include "main.moc"`,
  which requires AUTOMOC to generate the `.moc` file.
  `qt_add_executable` does not enable AUTOMOC on its own.
  Fix: ensure the project's root `CMakeLists.txt` calls
  `qt_standard_project_setup()` (it turns AUTOMOC on for the
  project), or add `set(CMAKE_AUTOMOC ON)` at the root, or
  `set_target_properties(tst_qmltests PROPERTIES AUTOMOC ON)`
  on the test target.
- **`enable_testing()` missing** — `add_test` calls are silently
  ignored; CTest reports "no tests found". Fix: add the line
  to the root `CMakeLists.txt` *before* any `add_subdirectory`
  that contains `add_test` calls.
- **`QUICK_TEST_SOURCE_DIR` mismatch** — the harness reports
  "no tests" because the configured directory is empty or
  wrong. The skill writes the macro pointing at
  `${CMAKE_CURRENT_SOURCE_DIR}`, which is the directory
  containing the generated `tests/CMakeLists.txt`. Move
  `tst_*.qml` files into that directory or update the macro.
- **Custom QML module not found at runtime** — the test fails
  with "module not installed" because the test binary is not
  linked against the project's backing library. Fix: uncomment
  and edit the project's backing library target name in
  `target_link_libraries` (e.g. `MyAppLib`).
- **`cannot find -l<name>plugin`** at link time — the named
  plugin target does not exist. Most often because
  `qt_add_qml_module` was called on an executable (no plugin
  is generated in that case). See the "Module-on-executable
  refactor" section above.
- **`"Type X unavailable"` / `"No such file or directory"`
  pointing at `qrc:/...` paths**, even though the QML files
  exist on disk — the auto-generated `build/<module>/qmldir`
  contains `prefer :/`. The qrc copies live in the original
  executable, not the test binary, so resolution fails. The
  cure is the refactor above, not editing the qmldir (which
  is regenerated every configure).
- **`"<SiblingType> is not a type"`** when loading a file that
  references a same-module sibling without an explicit import —
  same root cause as the previous bullet. Sibling-type
  resolution within a QML module requires the module to be
  registered in the *linking* binary. Loaded via on-disk qmldir
  from a sibling process, this resolution does not fire
  reliably.

## Why this is "starter" wiring

The template above is the minimum that gets a test running.
Production setups commonly add:

- **Per-test `add_test` granularity** (one CTest target per
  `tst_*.qml`) for parallelism and failure isolation.
- **Test data directories** copied into the build tree at
  configure time.
- **Environment overrides** (`QT_QPA_PLATFORM=offscreen`,
  `QT_LOGGING_RULES=*.debug=false`) on `add_test` via
  `set_tests_properties(... ENVIRONMENT ...)`.
- **CTest labels** for selective execution (`unit`, `slow`,
  `gui`).

The skill does not generate these by default; the project
owner can add them after the initial wiring is verified to
work.
