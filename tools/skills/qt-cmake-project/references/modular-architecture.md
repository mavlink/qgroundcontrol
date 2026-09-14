# Subdirectories, Qt libraries, plugins

Covers a complex project across `add_subdirectory()` boundaries, with special or multiple targets,
including libraries, Qt plugins.

## When to add a subdirectory

Use `add_subdirectory()` when one or more of these is true:

- The directory produces its own target (executable, library, plugin, QML module).
- The directory has a meaningfully different dependency set from its parent
  (e.g. only the GUI subdir needs `Qt6::Quick`).
- The directory is reusable across projects.

Do **not** add a subdirectory just to group source files — for that,
use folders and `target_sources(... PRIVATE …)` from the parent's `CMakeLists.txt`.

## Subdirectory pattern

Top-level `CMakeLists.txt` after the project setup block:

```cmake
add_subdirectory(src/core)          # internal library
add_subdirectory(src/widgets)       # Qt library exposing widgets
add_subdirectory(src/app)           # qt_add_executable lives here
add_subdirectory(src/plugins/csvio) # Qt plugin
```

Each subdirectory's `CMakeLists.txt` declares its own target and linkages. The parent never sets
sources for a child target — that is the child's responsibility. This keeps targets self-contained
and lets you move directories without rewriting parent files.

## Qt library (`qt_add_library`)

Use `qt_add_library` for libraries in a Qt project. It is a wrapper around CMake's
`add_library()` that handles target creation and finalization. Tooling like AUTOMOC is set by
`qt_standard_project_setup()` at the project level, not by `qt_add_library` itself.

`src/core/CMakeLists.txt`:

```cmake
qt_add_library(myapp_core STATIC
    notebook.cpp
    notebook.h
    notestore.cpp
    notestore.h
)

target_include_directories(myapp_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(myapp_core
    PUBLIC
        Qt6::Core         # exposed in public headers
    PRIVATE
        Qt6::Sql          # used only in .cpp
)
```

Choosing the library kind:

| Kind | When to use |
|------|-------------|
| `STATIC` | Internal libraries linked into one executable. |
| `SHARED` | Library shipped separately or loaded by multiple binaries. Requires symbol export annotations (`Q_DECL_EXPORT`/`Q_DECL_IMPORT`). |
| `OBJECT` | Code reused into multiple final libraries without producing an `.a`/`.lib` of its own. Niche. |
| `INTERFACE` | Header-only libraries. No source files. |

`MODULE` exists but is for plugins — use `qt_add_plugin` instead.

If the kind is omitted, the library type follows Qt's own build:
static Qt → static library, shared Qt → shared library. (Qt 6.7+ optionally honours
`BUILD_SHARED_LIBS` when policy `QTP0003` is set to NEW.) For internal components,
prefer specifying `STATIC` explicitly and documenting the choice.

### Shared library

A `SHARED` library shipped to other projects (or installed) needs three additions beyond the
static pattern:
- a namespaced alias target,
- generator expressions for include paths,
- and soname versioning.

```cmake
qt_add_library(myapp_core SHARED
    src/notebook.cpp
    src/notestore.cpp
)

# Namespaced alias — consumers link MyApp::core, not bare myapp_core.
add_library(MyApp::core ALIAS myapp_core)

# Generator expressions: build-tree path during build,
# install path for installed-package consumers.
target_include_directories(myapp_core
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

target_link_libraries(myapp_core
    PUBLIC
        Qt6::Core
)

# soname versioning — prevents ABI-mismatch crashes when multiple
# versions of this library exist on the system.
set_target_properties(myapp_core PROPERTIES
    VERSION   ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)
```

Symbol export annotations (`Q_DECL_EXPORT` / `Q_DECL_IMPORT`) are still required for the C++ side.
See the kinds table above.

### Public-header convention

For libraries consumed by other targets, expose headers via a versioned include path:

```
src/core/
  CMakeLists.txt
  include/
    core/
      notebook.h        # consumers write: #include <core/notebook.h>
  notebook.cpp
  notebook_p.h          # private impl detail — not in include/
```

`PUBLIC` `target_include_directories` points at `include/`. Private `_p.h` headers stay alongside
the `.cpp` and are not listed publicly.

### Linking the library from the executable

`src/app/CMakeLists.txt`:

```cmake
qt_add_executable(myapp main.cpp)

target_link_libraries(myapp PRIVATE
    Qt6::Quick
    myapp_core            # the library target name from the other subdir
)
```

CMake resolves `myapp_core` by target name regardless of where it was declared,
as long as the subdirectory containing it has been added.

## Qt plugin (`qt_add_plugin`)

Plugins are dynamically loaded modules — most commonly Qt imageformats, sqldrivers,
qmltooling extensions, or application-specific extension points.
Use `qt_add_plugin` for any of these in Qt 6.

`src/plugins/csvio/CMakeLists.txt`:

Note: `CLASS_NAME` defaults to the target name. Specify it explicitly only when the C++ class name
doesn't match the target name (as below — target `csvio_plugin` vs class `CsvIoPlugin`).

```cmake
qt_add_plugin(csvio_plugin
    CLASS_NAME CsvIoPlugin
    csvioplugin.cpp
    csvioplugin.h
)

target_link_libraries(csvio_plugin PRIVATE
    Qt6::Core
    myapp_core            # plugins typically link the host library
)
```

`CLASS_NAME` must match the C++ class declared with
`Q_PLUGIN_METADATA`:

```cpp
class CsvIoPlugin : public QObject, public IoPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "com.example.MyApp.IoPlugin/1.0")
    Q_INTERFACES(IoPluginInterface)
    // ...
};
```

### Static vs. dynamic plugins

If Qt was built statically, `qt_add_plugin` produces a `STATIC` library by default.
If Qt was built as shared libraries, it produces a `MODULE` library instead.
Override with the `STATIC` keyword to force a static plugin even on a shared Qt build.
The `SHARED` keyword is also accepted, but per Qt's docs, Qt converts it to `MODULE` internally —
there is no way to force a true `SHARED` library here, by design (Visual Studio symbol-export).
`MODULE` libraries are loaded dynamically at runtime;
the load mechanism depends on the plugin's category (generic app plugins use `QPluginLoader`;
image-format / SQL-driver / style / QML plugins are loaded automatically by their respective Qt
subsystems).

To statically link the plugin into the host instead:

```cmake
qt_add_plugin(csvio_plugin STATIC
    CLASS_NAME CsvIoPlugin
    csvioplugin.cpp
    csvioplugin.h
)
```

Then in the host target add `Q_IMPORT_PLUGIN(CsvIoPlugin)` once,
or use the `target_link_libraries()` CMake command:

```cmake
target_link_libraries(myapp PRIVATE csvio_plugin)
```

Note: When installing a static `qt_add_plugin` for other projects to consume, you also need to
install the internal helper targets that `qt_add_plugin` creates. Pass `OUTPUT_TARGETS <var>` to the
call, then `install(TARGETS ${var} …)` alongside the plugin itself. Skipping this means
consuming projects can't successfully link the static plugin.

### QML extension plugin

A plugin that adds C++ types to a QML module is **not** built with `qt_add_plugin` directly.
It is declared as part of the QML module itself — see `qml-integration.md` for the
`qt_add_qml_module(... PLUGIN_TARGET ...)` pattern.

## Dependency direction

Targets must form a directed acyclic graph. A common Qt project shape:

```
        +---------+
        |  myapp  |  (executable)
        +----+----+
             |
   +---------+---------+
   |                   |
+--v----+         +----v-----+
| core  | <-----+ | widgets  |
+-------+       | +----------+
                |
                |
        +-------+--------+
        | csvio_plugin   |
        +----------------+
```

- `core` is the foundation; nothing in `core` depends on `widgets` or `csvio_plugin`.
- Plugins depend on the host or on a thin "interface" library, not the other way around.
  The host loads plugins; it never links them as direct dependencies (unless they are static).
- If you find yourself wanting `core` to link `widgets`, the abstraction belongs in `core` and the
  implementation in `widgets`.

## Naming conventions

- Library targets: `<project>_<role>` (`myapp_core`, `myapp_widgets`).
  Avoid bare names like `core` that collide across projects.
- Plugin targets: `<project>_<plugin>_plugin` or just `<plugin>_plugin`.
  The `_plugin` suffix is conventional in Qt itself.
- QML module backing-target names: same as the executable or library that owns the module,
  one backing target per module.
- Executable targets: lowercase project name (`myapp`).
  Avoid case-insensitive name clashes between executable targets and QML module URIs as they
  cause problems on Mac OS.
