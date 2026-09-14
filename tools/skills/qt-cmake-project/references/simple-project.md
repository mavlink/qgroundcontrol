# Simple project, executable target, flat layout

Covers a simple Qt 6 project, producing a runnable application binary and organising a
flat source tree.

## Simple project layout

A minimal Qt Quick app needs almost no structure.
Qt's getting-started shows a single-directory project:

```
MyApp/
  CMakeLists.txt
  main.cpp
  Main.qml
```

For projects with multiple targets (an app + a library, an app + tests, multiple QML modules),
see `references/modular-architecture.md`.
Default to the flat layout above until you actually need the extra structure.

## CMakeLists.txt template for a simple Qt Quick app

The minimum a fresh Qt 6 Quick app should emit. Adapt the project name, Qt version,
and component list to the actual request, never include components the user did not ask for.

```cmake
cmake_minimum_required(VERSION 3.16)

project(MyProject
    VERSION 0.1.0
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(myapp main.cpp)

qt_add_qml_module(myapp
    URI MyQmlModule
    QML_FILES
        Main.qml
    RESOURCE_PREFIX /qt/qml  # is the default when QTP0001 is NEW, can be left out
)

target_link_libraries(myapp PRIVATE Qt6::Quick)
```

Notes:

- `cmake_minimum_required(VERSION 3.16)` matches the version used in Qt's official
  getting-started template. Use a higher value if the user asks.
- `qt_standard_project_setup(REQUIRES 6.8)` enables Qt CMake policies introduced up to
  that Qt version (set to NEW), and causes a configuration-time error if Qt is older than the
  specified version. The argument is optional but recommended:
  it opts you into modern defaults like QTP0001 (which sets
  `:/qt/qml/` as the default QML resource prefix).
- `LANGUAGES CXX` is explicit. Without it CMake also enables C, which is harmless but
  adds compiler-detection overhead.
- For Android projects on CMake versions earlier than 3.19, also call `qt_finalize_project()`
  at the end of the top-level `CMakeLists.txt`. On CMake 3.19+ this happens automatically.
- Our QML module's target name is `myapp` above just because the QML module is embedded into the
  executable. Otherwise, it would be better to name it after its URI to be `myqmlmodule`.
- A QML module's URI name is recommended to not clash with a target executable name because of
  case-insensitive file systems. Giving URI `MyApp` to the QML module above would lead to such a
  clash on Mac OS. This does not happen on Windows because executables get `.exe` suffix there.

## qt_add_executable specifics

Use `qt_add_executable`, not `add_executable`. It auto-links `Qt::Core`, handles
target finalization (auto-deferred to the end of the current directory scope on CMake 3.19+),
and on Android creates a `MODULE` library suitable for APK packaging instead of a
regular executable. The `WIN32` and `MACOSX_BUNDLE` options are user-supplied and
pass through to `add_executable()` for GUI / bundle apps.

Both spellings work for GUI / bundle apps: passing `WIN32`/`MACOSX_BUNDLE` as positional args
to `qt_add_executable`, or setting the `WIN32_EXECUTABLE`/`MACOSX_BUNDLE` target properties
via `set_target_properties` after target creation.
Qt's official getting-started uses the property form.

## main.cpp template (Qt Quick application)

```cpp
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    engine.loadFromModule("MyQmlModule", "Main");

    return app.exec();
}
```

`engine.loadFromModule("MyQmlModule", "Main")` (Qt 6.5+) is preferred over
`engine.load(QUrl("qrc:/qt/qml/MyQmlModule/Main.qml"))`. It uses the QML module URI directly
and works with both file-system imports during development and resource-system imports
in shipped binaries.

Note: `QQmlApplicationEngine` does not auto-create a root window.
The loaded type (`Main` here) must be a `Window` or `ApplicationWindow`,
visual Qt Quick items must be placed inside one.
Loading a plain `Item` or `Rectangle` produces a running app with no visible UI.

Do not generate a `qmake` `.pro` file and do not emit instructions for `qmake -r`.
If the user asks for both build systems, pick one and ask which they prefer.
Supporting both in one commit is almost never what they want.

## When to grow the structure

When a project grows past a single target - adding a library, a plugin, tests,
or multiple QML modules — that's when subdirectories earn their keep.
See `references/modular-architecture.md` for the `add_subdirectory` patterns
and `references/qml-integration.md` for splitting QML modules into their own directories.
