# Common LLM mistakes — pre-flight checklist

This is the bias-correction layer. Mainstream LLMs emit the mistakes below by default when
generating Qt CMake. Before returning the final output, check the draft against every item here.

Each entry has the wrong pattern, the right pattern, and the underlying reason.
Knowing the reason matters. It lets the agent generalise to cases this list does not enumerate.

## 1. `add_executable` instead of `qt_add_executable`

```cmake
# WRONG
add_executable(myapp main.cpp)

# RIGHT
qt_add_executable(myapp main.cpp)
```

**Why:** `qt_add_executable` auto-links `Qt::Core`, handles target finalization,
and on Android creates a `MODULE` library suitable for APK packaging.
The `WIN32` and `MACOSX_BUNDLE` options are user-supplied opt-ins, not automatic.
`add_executable` skips all of this.

## 2. `qt5_*` macros in a Qt 6 project

```cmake
# WRONG
qt5_add_resources(myapp_RESOURCES resources.qrc)
qt5_wrap_ui(myapp_UIS mainwindow.ui)

# RIGHT
qt_add_resources(myapp "myapp_data" PREFIX "/" FILES …)
# UI files are picked up by AUTOUIC; no manual wrap call needed.
```

**Why:** Qt's compatibility guide recommends the versionless `qt_*` commands for new code.
`qt5_*` macros are intended for projects that need to support Qt 5 versions older than 5.15;
they use the legacy variable-list API instead of the target-based CMake API.

## 3. Missing `qt_standard_project_setup()`

```cmake
# WRONG — manually enabling what the helper does (and missing what it doesn't)
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
qt_add_executable(myapp main.cpp)

# RIGHT
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
qt_standard_project_setup(REQUIRES 6.8)
qt_add_executable(myapp main.cpp)
```

**Why:** `qt_standard_project_setup()` enables `CMAKE_AUTOMOC` and `CMAKE_AUTOUIC`,
includes `GNUInstallDirs`, configures Windows runtime output and RPATH defaults,
and (with `REQUIRES`) opts you into modern Qt CMake policies.
Hand-rolling `set(CMAKE_AUTOMOC ON)` etc. misses the policy opt-in and the install-layout defaults.
Note: it does *not* set `AUTORCC` or the C++ standard, those still need explicit `set()` calls.

## 4. Hand-written `.qrc` for QML files

```cmake
# WRONG
qt_add_executable(myapp main.cpp)
qt_add_resources(myapp "qml" PREFIX "/" FILES
    Main.qml
    AboutDialog.qml
)

# RIGHT
qt_add_executable(myapp main.cpp)
qt_add_qml_module(myapp
    URI MyApp
    QML_FILES
        Main.qml
        AboutDialog.qml
)
```

**Why:** QML files placed via `qt_add_resources` are invisible to the QML compiler,
the type registrar, and the QML language server.
They will load at runtime but lose static analysis, ahead-of-time compilation, and IDE autocomplete.

## 5. `find_package(Qt6 REQUIRED)` with no minimum version

```cmake
# WRONG
find_package(Qt6 REQUIRED COMPONENTS Quick)

# RIGHT
find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)
```

**Why:** Many `qt_*` commands have evolved between minor 6.x releases
(`qt_add_qml_module` in particular). Without a minimum, the project may build on the
developer machine and fail in CI on an older Qt. The `REQUIRES` argument to
`qt_standard_project_setup()` and the version pin to `find_package` should agree.

## 6. `QT += quick` or other `.pro` syntax

```
# WRONG (qmake leftover)
QT += quick widgets
CONFIG += c++17
SOURCES += main.cpp
RESOURCES += assets.qrc
```

**Why:** This is qmake `.pro` syntax, not CMake syntax. CMake fails to parse it.
There is no `+=` operator, and bare names like `QT` start a function-call parse
that expects `(` next. The configure step errors out at the `+=` token; nothing is built.
Never emit `.pro` syntax inside a `CMakeLists.txt`.

## 7. Mismatched module URI and directory

```
# WRONG
qml/components/MyButton.qml          # directory: lowercase "components"
qt_add_qml_module(myqmlmodule_components
    URI MyQmlModule.Components             # URI: uppercase "Components"
)

# RIGHT
qml/MyQmlModule/Components/MyButton.qml
qt_add_qml_module(myqmlmodule_components
    URI MyQmlModule.Components
)
```

**Why:** The QML engine resolves `import MyQmlModule.Components` by appending the dotted path
to its import path. If the directory on disk doesn't match, `qt_add_qml_module()` may
fail to import at runtime if the import path setup doesn't compensate.
Using the `TARGET <cmake-target>` version of `IMPORTS` or `DEPENDENCIES` of this command
can eliminate this. So the ultimate solution to not having to care about folder structure is
importing MyQmlModule.Components from cmake via the `TARGET` import format:

```cmake
qt_add_qml_module(myqmlmodule  # or myapp if it is an executable
    URI MyQmlModule
    IMPORTS TARGET myqmlmodule_components
)
```

## 8. Lowercase QML file names

```
# WRONG
qml/MyQmlModule/mybutton.qml

# RIGHT
qml/MyQmlModule/MyButton.qml
```

**Why:** QML treats UpperCamelCase file names as types. A file named `mybutton.qml` is
not usable as `MyButton { … }` in another QML file, only via `Qt.createComponent("mybutton.qml")`.

## 9. Manually listing AUTOMOC outputs

```cmake
# WRONG
qt_add_executable(myapp
    main.cpp
    moc_mainwindow.cpp        # generated — never list manually
    qrc_resources.cpp         # generated — never list manually
)

# RIGHT
qt_add_executable(myapp
    main.cpp
    mainwindow.cpp
    mainwindow.h
)
```

**Why:** `moc_*.cpp`, `ui_*.h`, and `qrc_*.cpp` are produced by AUTOMOC/AUTOUIC/AUTORCC
during the build. Listing them as source files causes "file not found" at configure time
or duplicate symbol errors at link time.

## 10. Wrong `target_link_libraries` visibility

```cmake
# WRONG — exposes Quick to consumers of myapp_core unnecessarily
target_link_libraries(myapp_core PUBLIC Qt6::Quick)

# RIGHT — Quick is an implementation detail of myapp_core
target_link_libraries(myapp_core PRIVATE Qt6::Quick)
```

**Why:** `PUBLIC` propagates the dependency to every consumer of the target.
Use `PUBLIC` only when the dependency appears in the target's *public headers*.
Otherwise use `PRIVATE`. `INTERFACE` is consumer-only — for `target_link_libraries`,
mainly header-only or alias targets; for `target_include_directories`,
also normal on compiled libraries whose include paths are consumer-only (see SKILL.md Rule 6).

## 11. `RESOURCE_PREFIX /` in a QML module

```cmake
# WRONG
qt_add_qml_module(myapp
    URI MyQmlModule
    RESOURCE_PREFIX /              # collides with non-QML resources
    QML_FILES Main.qml
)

# RIGHT
qt_add_qml_module(myapp
    URI MyQmlModule
    RESOURCE_PREFIX /qt/qml        # Qt 6 default, leave it
    QML_FILES Main.qml
)
```

**Why:** When the `QTP0001` policy is NEW (e.g. via `qt_standard_project_setup(REQUIRES 6.8)`),
`/qt/qml` is the default prefix where the QML engine looks when importing a module by URI.
Overriding it to `/` collides with hand-added `qt_add_resources` calls and
makes `engine.loadFromModule(...)` fail. Without QTP0001 NEW, the default stays `/`.

## 12. Inventing `qt_add_qml_module` argument names

The actual arguments are listed in `qml-integration.md`.
The following names *do not exist* despite being plausible:

- `SOURCE_FILES` (use `QML_FILES` for QML, `SOURCES` for C++)
- `QML_SOURCES` (use `QML_FILES`)
- `QRC_PREFIX` (use `RESOURCE_PREFIX`)
- `MODULE_NAME` (use `URI`)
- `MODULE_VERSION` (use `VERSION`)
- `IMAGES` / `ASSETS` (use `RESOURCES`)

If unsure of a name, query the Qt docs MCP tool or
fetch the official command reference page rather than guessing.

## 13. `qt_add_plugin` for QML extension plugins

```cmake
# WRONG — bypasses QML module registration
qt_add_plugin(myapp_qml_plugin
    CLASS_NAME MyAppPlugin
    plugin.cpp
)

# RIGHT — plugin is part of the QML module
qt_add_qml_module(myapp_quick
    URI MyApp.Quick
    PLUGIN_TARGET myapp_quick_plugin
    SOURCES
        myextension.cpp
        myextension.h
)
```

**Why:** A QML extension plugin must be wired into the QML module's `qmldir` so the engine
loads it on `import`. Only `qt_add_qml_module(... PLUGIN_TARGET ...)` does that wiring.

**Note on `PLUGIN_TARGET`:** the argument is *optional*. If omitted, `qt_add_qml_module`
auto-generates a plugin target named `<backing-target>plugin`
(e.g. `myapp_quick` → `myapp_quickplugin`).
Specify `PLUGIN_TARGET` only to customize the name or when using `NO_CREATE_PLUGIN_TARGET`.
The auto-generated plugin is almost always adequate unless adding image providers.
Do not declare `PLUGIN_TARGET` for executable-backed (application-owned) modules,
there is no separate plugin in that case.

## 14. Generating a `.pro` file alongside `CMakeLists.txt`

If the user asks to "support both build systems," **ask which one they want**
before generating either. Maintaining a `.pro` and a `CMakeLists.txt` for the same project
is almost always a mistake. The two drift apart, builds diverge between contributors, and the
QML compiler / language server only fully work with the CMake build.

The exception is a Qt-internal module where dual-builds can be supported for legacy reasons.
