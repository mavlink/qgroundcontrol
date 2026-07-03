# QML modules, adding QML files, reusable controls

Covers defining and growing custom QML modules, adding `.qml` files to an existing project,
and adding reusable UI controls. Also covers the related task of exposing C++ types to QML.

## The QML module is the unit of organisation

In Qt 6, **every QML file lives in a QML module**. There is no supported way to add a `.qml` file
to a Qt project except by listing it in a `qt_add_qml_module(...)` call.
The legacy patterns from Qt 5 — adding `.qml` files to a `.qrc` resource file by hand,
or loading them from an arbitrary file path — are still tolerated but bypass the QML compiler,
the type registrar, and the QML language server.

A QML module is identified by a dotted **URI** (`MyQmlModule`, `MyQmlModule.Controls`, `Acme.Charts`)
and is bound to a CMake **backing target** — usually an executable or a library.
One backing target may host exactly one QML module.

## Defining a QML module

Minimal declaration alongside a target:

```cmake
qt_add_qml_module(myapp  # should be myqmlmodule if it is a separate library, not embedded into the executable
    URI MyQmlModule
    QML_FILES
        Main.qml
        AboutDialog.qml
    RESOURCE_PREFIX /qt/qml  # is the default when QTP0001 is NEW, can be left out
)
```

Argument cheat-sheet (the names LLMs frequently get wrong):

| Argument | Purpose |
|----------|---------|
| `URI` | Dotted module name. Recommended to match the directory path under the QML import root. |
| `VERSION` | Optional. Major.minor module version. Defaults to the highest possible version. Qt recommends omitting it unless you actively use module versioning (it has fundamental flaws — prefer external package management). |
| `QML_FILES` | `.qml`, `.js`, and `.mjs` files compiled into the module. |
| `SOURCES` | C++ sources compiled into the backing target as part of the module (optionally use `target_sources`). |
| `RESOURCES` | Non-QML assets bundled into the module's resource subtree (e.g. images referenced *only* from QML). |
| `RESOURCE_PREFIX` | Resource-system prefix the module is mounted under. Defaults to `/qt/qml` when the `QTP0001` policy is NEW (e.g. via `qt_standard_project_setup(REQUIRES 6.8)`). Old default is `/`. |
| `PLUGIN_TARGET` | Optional. Name of the plugin target. Defaults to `<backing-target>plugin` (e.g. backing `myqmlmodule` → plugin `myqmlmoduleplugin`). Setting `PLUGIN_TARGET` equal to the backing target produces a plugin-as-its-own-backing-target arrangement (the module then must be loaded dynamically). Use `NO_PLUGIN` instead if you don't want any plugin at all. |
| `IMPORTS` / `OPTIONAL_IMPORTS` | Other QML modules this one imports. Written into the generated `qmldir` file; QML files that import this module also transitively get these imports. |
| `DEFAULT_IMPORTS` | Subset of `OPTIONAL_IMPORTS`: which optional imports tooling like `qmllint` should resolve as the default. Niche. |
| `DEPENDENCIES` | Other QML modules this module depends on at the C++ level (e.g. when registering a class that inherits a C++ type from another module) but does not import in QML. Where a dependency works as either `IMPORTS` or `DEPENDENCIES`, Qt recommends `DEPENDENCIES` — it is lighter (doesn't propagate to consumers that import this module). |

**The ultimate solution for not having to keep folder structure and QML URIs in sync**:
`IMPORTS`, `OPTIONAL_IMPORTS`, `DEFAULT_IMPORTS`, and `DEPENDENCIES` accept a
`TARGET <cmake-target>` form when the `QTP0005` policy is NEW (requires Qt 6.8+ via
`qt_standard_project_setup(REQUIRES 6.8)`). This **does not auto-link** the target,
only derives QML metadata, import paths, and build-order dependencies.
If your code calls into the target, also call `target_link_libraries()` explicitly.
The named target must be built by your project (not an imported `find_package` target).
Transitive QML-module deps are not added automatically — declare each one explicitly.

**There is no** `SOURCE_FILES`, `QML_SOURCES`, `QRC_PREFIX`, or `MODULE_NAME` argument.
Do not invent option names.

## Module URI is recommended to match directory path

For a module with `URI MyQmlModule.Controls`, the QML files are recommended to live in a
directory ending with `MyQmlModule/Controls/` on disk:

```
qml/
  MyQmlModule/
    Controls/
      CMakeLists.txt
      MyButton.qml
      MyToggle.qml
```

The QML engine resolves `import MyQmlModule.Controls 1.0` by appending the dotted path to each entry
on its import path. If your directory is named `controls/` (lowercase) or `Components/`,
before Qt 6.8, `qt_add_qml_module` will issue a configure-time warning and the import may fail
at runtime, because the module is registered under one URI and the engine expects to find it at a
path that mirrors that URI.
Beginning with Qt 6.8, the default `QTP0005` policy allows the `TARGET <cmake-target>` format
to be used for QML imports and dependencies which automatically finds out import path and URI
from the module's metadata. Always use this solution if possible.

## Adding a QML file to an existing module

Two steps, in order:

1. **Create the file in the module's directory.** Use UpperCamelCase for the file name.
   The QML engine treats `MyButton.qml` as a type named `MyButton`; lowercase filenames
   produce types that cannot be used as elements in QML, only via `Qt.createComponent`.

2. **Add it to the `QML_FILES` list** in the same directory's `CMakeLists.txt`:

   ```cmake
   qt_add_qml_module(myqmlmodule_controls
       URI MyQmlModule.Controls
       QML_FILES
           MyButton.qml
           MyToggle.qml
           MySlider.qml          # <-- newly added
   )
   ```

Do **not** add the file to a separate `.qrc` file. Do **not** add it via `qt_add_resources`.
Do **not** load it via an absolute file path in C++ —
`engine.loadFromModule("MyQmlModule.Controls", "MySlider")` is the supported access pattern once the
file is in `QML_FILES`.

To add `.qml` files to a module after `qt_add_qml_module()` was called (conditional adds,
subdirectory-walking, build-time generated files, etc.),
use `qt_target_qml_sources(target QML_FILES …)`.
For files that don't yet exist at configure time, also set the `GENERATED TRUE` source property.

## Adding a reusable UI control

A "reusable UI control" in QML terms is a `.qml` file whose root type is
*not* `Window`/`ApplicationWindow`, declares one or more public properties or signals,
and is meant to be composed into other components.

The mechanics are identical to *adding any QML file* (above). The file goes in `QML_FILES`
of the appropriate module. The naming and packaging conventions matter:

- **Filename = type name.** `MyButton.qml` becomes `MyButton`.
  No suffixes like `MyButtonControl.qml` unless `Control` is genuinely part of the public name.
- **One control per file.** Helper components used only by the control go inline as nested objects,
  or in a sibling file prefixed with an underscore (`_MyButtonInternals.qml`).
  Underscore-prefixed types are conventionally treated as private.
- **Place reusable controls in their own QML module.** A module named `MyQmlModule.Controls` (URI)
  with backing target `myqmlmodule_controls` (a `qt_add_library`) keeps the reusable
  surface separate from application code:

  ```cmake
  qt_add_library(myqmlmodule_controls STATIC)

  qt_add_qml_module(myqmlmodule_controls
      URI MyQmlModule.Controls
      QML_FILES
          MyButton.qml
          MyToggle.qml
  )

  target_link_libraries(myqmlmodule_controls PUBLIC Qt6::Quick)
  ```

  The application target then links `myqmlmodule_controls` and writes
  `import MyQmlModule.Controls 1.0` in its QML.

  ```cmake
  qt_add_qml_module(myapp  # or myqmlmodule if it is a library
      URI MyQmlModule
      IMPORTS TARGET myqmlmodule_controls
  )

  target_link_libraries(myapp PUBLIC myqmlmodule_controls)
  ```

## Singletons

A QML singleton is a single shared instance accessible by type name.
Mark the file with `pragma Singleton` at the top, *and* flag it via the `QT_QML_SINGLETON_TYPE`
source property **before** the `qt_add_qml_module` call:

```cmake
set_source_files_properties(Theme.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

qt_add_qml_module(myapp  # better to be myqmlmodule if it is not embedded into an executable
    URI MyQmlModule
    QML_FILES
        Main.qml
        Theme.qml
)
```

The `QT_QML_SINGLETON_TYPE` source property is required. Without it the QML compiler
will not register the file as a singleton even if the `pragma Singleton` is present.

**Order matters**: per Qt's docs,
*"the source property must be set before creating the module the singleton belongs to."*
Setting it after `qt_add_qml_module` results in the `singleton` directive not being written
into the generated `qmldir`, so the type is registered as a regular type instead of a singleton.

## C++ types exposed to QML

When a QML module needs C++-defined types (a model, a backend controller, a custom item),
declare the C++ class with the QML registration macros and let `qt_add_qml_module` pick them up via
`SOURCES` or via the backing target's `target_sources`:

```cpp
// noteviewmodel.h
#include <QObject>
#include <QtQmlIntegration/qqmlintegration.h>

class NoteViewModel : public QObject
{
    Q_OBJECT
    QML_ELEMENT          // exposes as `NoteViewModel` in the module
    // ...
};
```

Available registration macros:

| Macro | Effect |
|-------|--------|
| `QML_ELEMENT` | Register type with name = class name |
| `QML_NAMED_ELEMENT(Name)` | Register with a custom name |
| `QML_SINGLETON` | Register as a singleton |
| `QML_UNCREATABLE("reason")` | Type is queryable but not instantiable from QML |
| `QML_ANONYMOUS` | Register the type but do not expose a name (used for base classes) |
| `QML_INTERFACE` | Register an interface for use with `as` |

The macros require the `Qt6::QmlIntegration` link dependency.
`qt_add_qml_module` handles type registration generation automatically.
Do not write `qmlRegisterType<NoteViewModel>(...)` calls by hand for files in the module.

`qt_add_qml_module` requires `AUTOMOC` to be enabled on the backing target.
`qt_standard_project_setup()` enables it by default. Without `AUTOMOC`,
automatic type registration silently doesn't happen and your C++ types won't appear in QML.

## Loading a QML module from C++

```cpp
QQmlApplicationEngine engine;
engine.loadFromModule("MyQmlModule", "Main");
```

`loadFromModule(uri, typeName)` is the Qt 6.5+ idiomatic loader.
Avoid `engine.load(QUrl("qrc:/qt/qml/MyQmlModule/Main.qml"))` in new code.
It works but couples the loader to the resource prefix and the file name.

## Static vs. dynamic QML modules

By default the QML module's plugin (if any) is a shared library loaded at runtime.
To statically link a QML module into the host binary
(common for desktop apps with a single executable) either:

- Have the application's *own* `qt_add_qml_module` call (the module is part of the executable's
  backing target — no plugin needed), or
- For a library-backed module, pass `STATIC` to the library and let `qt_import_qml_plugins(myapp)`
  register the module's plugin into the host.

  Note: `qt_import_qml_plugins(target)` has no effect in non-static build configurations.

Do not declare `PLUGIN_TARGET` for application-owned modules.
The executable *is* the backing target, and there is no separate plugin to load.

## Mistakes specific to QML modules

- Listing `.qml` files inside `qt_add_resources(...)` instead of `QML_FILES`.
  The compiler skips them, the language server cannot see them, and `loadFromModule` fails.
- Using the same name for the QML module name (URI) which is embedded into the same named
  executable will cause name clashing on the case-insensitive file system of Mac OS because there
  is no `.exe` suffix for executables unlike on Windows.
- Lowercase QML filenames (`mybutton.qml`). The type is unusable as an element.
- Adding a `module` line to a hand-written `qmldir` file. The `qmldir` is generated by
  `qt_add_qml_module`; never edit it by hand.
- One `qt_add_qml_module` per source folder *and* a parent module that re-exports the children.
  Modules cannot re-export. Either flatten or compose at import-time in QML.
- Declaring two `qt_add_qml_module` calls with the same backing target.
  One backing target hosts one module — split the target if you need two modules.
