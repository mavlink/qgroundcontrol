# Static resources — images, icons, fonts, translations

Covers adding static resources (images, icons, fonts, JSON, shaders, translations)
to a Qt 6+ CMake project.

## Three places resources can live

Pick the right one — using the wrong mechanism is the most common LLM mistake here.

| Asset is consumed by… | Add it via… |
|-----------------------|-------------|
| QML only | `qt_add_qml_module(... RESOURCES ...)` |
| C++ only (or both) | `qt_add_resources(target "name" FILES ...)` |
| Translations (.ts / .qm) | `qt_add_translations(target ...)` |

Hand-written `.qrc` files still work. The CMake commands above generate `.qrc` content
automatically and keep it in sync with the source list, which is usually preferable.
Fewer files to maintain and proper CMake dependency tracking.
Hand-written `.qrc` is fine for a pre-existing one inherited from an older project.

## Resources for QML (images referenced from `.qml`)

Use the `RESOURCES` argument of `qt_add_qml_module`. The assets are mounted under the
same module subtree as the QML files, so relative paths from QML "just work":

```cmake
qt_add_qml_module(myapp  # could be myqmlmodule if module is not embedded into executable
    URI MyQmlModule
    QML_FILES
        Main.qml
        AboutDialog.qml
    RESOURCES
        resources/images/logo.png
        resources/images/background.jpg
        resources/icons/app-icon.svg
)
```

In QML the assets are addressable two ways:

```qml
// Relative path resolved against the QML file's location:
Image { source: "resources/images/logo.png" }

// Absolute resource URL using the module's prefix:
Image { source: "qrc:/qt/qml/MyQmlModule/resources/images/logo.png" }
```

Prefer the relative form in QML. It survives module renames because it does not encode the URI.

## Resources for C++

Use `qt_add_resources` with an explicit prefix (since Qt 6.5 `PREFIX` is optional
and defaults to `/`, but specifying it is recommended for namespacing):

```cmake
qt_add_resources(myapp_core "myapp_core_resources"
    PREFIX "/myapp"
    FILES
        data/schema.json
        images/bg.jpg
)
```

Then read in C++ via the resource path:

```cpp
QFile schema(":/myapp/data/schema.json");
schema.open(QIODevice::ReadOnly);
```

Notes:

- The second argument (`"myapp_core_resources"`) names the generated resource.
  It must be unique **across all resources that end up in the final linked target**,
  not just within the library where you call `qt_add_resources`.
  This especially matters for static builds, where the same resource name in different static
  libraries collides in the consuming target.
- `PREFIX` becomes the leading path segment under `:/`.
  Pick something namespaced (`/myapp`, `/myapp_core`), bare `/` collides easily across libraries.
- Multiple `qt_add_resources` calls per target are allowed and often clearer than one giant call.
  Use one per logical asset group (icons, data, shaders).
- `PREFIX` via a leading namespace and `FILES` via the lead-in folder names together determine the
  full URI of resources. Using for example `images` for both leads to useless double grouping.

## Application icon

Application icons are *not* a resource, they are platform metadata.

- **Windows:** Create a `.rc` file describing the icon, then pass
  it as a source to `qt_add_executable`:

  ```cmake
  set(app_icon_windows "${CMAKE_CURRENT_SOURCE_DIR}/resources/myapp.rc")
  qt_add_executable(myapp main.cpp ${app_icon_windows})
  ```

- **macOS:** Set the bundle icon file name, copy the `.icns` into the
  bundle's `Resources` directory, then add it to the executable.
  All three pieces are required, without `MACOSX_PACKAGE_LOCATION "Resources"` the icon is
  not placed inside the `.app` bundle:

  ```cmake
  set(MACOSX_BUNDLE_ICON_FILE myapp.icns)
  set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resources/myapp.icns")
  set_source_files_properties(${app_icon_macos} PROPERTIES
      MACOSX_PACKAGE_LOCATION "Resources")
  qt_add_executable(myapp MACOSX_BUNDLE main.cpp ${app_icon_macos})
  ```

- **Linux:** Ship a `.desktop` file plus icons under `share/icons/hicolor/<size>/apps/`.
  These are install rules, not target sources.

For the Qt window icon (the one shown in the title bar / taskbar),
use `QGuiApplication::setWindowIcon(QIcon(":/myapp/icons/app-icon.png"))`
in `main.cpp`. This *is* a normal resource.

## Fonts

Bundle the font file as a resource, then load it at startup:

```cmake
qt_add_resources(myapp "myapp_fonts"
    PREFIX "/fonts"
    FILES
        Inter-Regular.ttf
        Inter-Bold.ttf
)
```

```cpp
int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QFontDatabase::addApplicationFont(":/fonts/Inter-Regular.ttf");
    QFontDatabase::addApplicationFont(":/fonts/Inter-Bold.ttf");

    QQmlApplicationEngine engine;
    engine.loadFromModule("MyQmlModule", "Main");
    return app.exec();
}
```

For QML-only consumption (no C++), bundle via the QML module's `RESOURCES` and
load with `FontLoader { source: "..." }` instead.

## Shaders, JSON, CSV, other binary blobs

Same pattern as data files above — `qt_add_resources` with a namespaced prefix.
For shaders compiled with `qsb` (the Qt Shader Tool) use the dedicated `qt_add_shaders` command.

The shader command lives in the `ShaderTools` component:

```cmake
find_package(Qt6 REQUIRED COMPONENTS ShaderTools)
```

Without this, `qt_add_shaders` is undefined.

```cmake
qt_add_shaders(myapp "myapp_shaders"
    PREFIX "/shaders"
    FILES
        blur.frag
        blur.vert
)
```

This compiles each shader to the QSB container format consumed by the Qt RHI before bundling.

## Translations

Translation commands live in the `LinguistTools` component, not `Core`. Load it with:

```cmake
find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
```

Without this, `qt_add_translations`, `qt_add_lupdate`, and `qt_add_lrelease` will not be defined.

Translation files use their own command:

```cmake
qt_add_translations(myapp
    TS_FILES
        i18n/qml_en.ts
        i18n/qml_de.ts
        i18n/qml_fi.ts
)
```

This command is a convenience wrapper around `qt_add_lupdate` and `qt_add_lrelease` and
has existed since Qt 6.2. It generates `.qm` files at build time and (by default)
embeds them into the target's resource subtree at `:/i18n/`.
Qt 6.7 introduced an extended form with more options (`TARGETS`, `SOURCE_TARGETS`,
`PLURALS_TS_FILE`, etc.); the original 6.2 form still works but is deprecated.

Note: For `QQmlApplicationEngine` auto-loading, translation file names must start with `qml_`
(e.g. `qml_de.qm`), files named otherwise still compile and embed but won't be auto-loaded by
the engine. When letting `qt_add_translations` auto-generate `.ts` files
(via `qt_standard_project_setup(I18N_TRANSLATED_LANGUAGES …)`), pass `TS_FILE_BASE qml` to
get the right naming. For widgets / console apps using `QTranslator` directly, any naming works.

## Hand-written `.qrc` (when you really need it)

The supported case is a pre-existing `.qrc` file inherited from an older project.
Add it via the variable-based variant of `qt_add_resources`:

```cmake
set(legacy_sources "")
qt_add_resources(legacy_sources legacy_resources.qrc)
target_sources(myapp PRIVATE ${legacy_sources})
```

In case of using hand-written `.qrc` files in static builds, use `Q_INIT_RESOURCE(<resource-name>)`
at the beginning of `main()` to not let the linker silently drop the generated registration code.
However, greenfield code is usually better off without hand-written `.qrc` files.

## Resource prefix conventions

- `/qt/...` and `/qt-project.org/...` — reserved by Qt for documented use cases
  (e.g. `qt_add_qml_module` mounts modules under `/qt/qml/<URI-path>`; `qt.conf` is read from
  `/qt/etc/qt.conf`). Application code must not write under these prefixes.
- `/<project>` or `/<project>/<group>` — application code. Pick one and use it consistently.
  Keep prefixes short — they appear in every resource path the rest of the codebase writes.
- Do not nest a project under `/` (root) without a namespace segment.
  Library + application sharing the root prefix is a common source of
  "file not found" bugs at runtime.
