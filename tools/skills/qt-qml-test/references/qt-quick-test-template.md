# Qt Quick Test — template variants

Paste-ready test skeletons. Pick the variant whose shape matches
the source QML being tested and apply the rules from `SKILL.md`.

## Variant 1 — Single component

Use when the test exercises one component directly (no children
to reach into). The component is instantiated inside each test
function.

```qml
import QtQuick
import QtTest

<source-import>

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: myButtonComponent
        MyButton {}
    }

    TestCase {
        name: "MyButtonTests"
        when: windowShown

        function test_componentExists() {
            let mybutton = createTemporaryObject(myButtonComponent, root)
            verify(!!mybutton, "Component exists")
        }

        function test_defaultText() {
            let mybutton = createTemporaryObject(myButtonComponent, root)
            verify(!!mybutton, "Component exists")
            compare(mybutton.text, qsTr(""))
        }
    }
}
```

Every `createTemporaryObject` call passes `root` (the outer
`Item`) as the parent. The default — the `TestCase` — is
`visible: false`, so children created under it never enter the
visual tree, and `mouseClick` / focus / rendering-dependent
checks fail silently.

## Variant 2 — Nested components

Use when the source QML wraps several inner items (a form, a
dialog, a layout). Load the parent once and reach into named
children via `findChild`.

```qml
import QtQuick
import QtTest

<source-import>

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: myFormComponent
        MyForm {}
    }

    TestCase {
        name: "MyFormTests"
        when: windowShown

        function test_submitButtonExists() {
            let app = createTemporaryObject(myFormComponent, root)
            verify(!!app, "Component exists")
            let submitButton = findChild(app, "submitButton")
            verify(!!submitButton, "Object exists")
        }

        function test_emailFieldDefault() {
            let app = createTemporaryObject(myFormComponent, root)
            verify(!!app, "Component exists")
            let emailField = findChild(app, "emailField")
            verify(!!emailField, "Object exists")
            compare(emailField.text, qsTr(""))
        }
    }
}
```

The source QML must declare `objectName: "..."` on every
descendant the test reaches. Never call `findChild` with an
empty `objectName`.

## Variant 3 — Components requiring focus

Input controls (`TextField`, `TextArea`, `TextInput`,
`TextEdit`) need focus before they accept input. Set `focus =
true` immediately after looking the child up, before any input
events.

```qml
import QtQuick
import QtTest

<source-import>

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: myFormComponent
        MyForm {}
    }

    TestCase {
        name: "EmailFieldTests"
        when: windowShown

        function test_emailFieldAcceptsText() {
            let app = createTemporaryObject(myFormComponent, root)
            verify(!!app, "Component exists")
            let emailField = findChild(app, "emailField")
            verify(!!emailField, "Object exists")
            emailField.focus = true
            emailField.text = qsTr("user@example.com")
            compare(emailField.text, qsTr("user@example.com"))
        }
    }
}
```

For tests that depend on a focus-change-triggered property
update, set `focus = false` or `focus = true` explicitly before
the assertion (rule 28).

## Variant 4 — Multi-instance / similar controls

When the source QML contains several controls of the same type
(two `Slider`s, a row of `Button`s) and the test must spy on
each separately, declare one `SignalSpy` per target with
descriptive IDs (rules 21, 22). Set each target inside the test
function.

The signal shown here (`valueModified`) requires interactive
input to fire — drive each spy via a real input event
(e.g. `mouseClick` on `up.indicator` / `down.indicator`),
not a property assignment and not the `increase()` /
`decrease()` methods. Per Qt 6 docs, `valueModified` is
emitted only on touch, mouse, wheel, or key interaction;
the methods bypass that path and fire `valueChanged` only.
See the per-control sections of
`qt-quick-test-controls.md` for which signals are
interactive-only.

```qml
import QtQuick
import QtTest

<source-import>

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: myFormComponent
        MyForm {}
    }

    SignalSpy {
        id: ageValueModifiedSpy
        signalName: "valueModified"
    }

    SignalSpy {
        id: priceValueModifiedSpy
        signalName: "valueModified"
    }

    TestCase {
        name: "MultiSpinBoxTests"
        when: windowShown

        function test_ageSpinBoxValueModified() {
            let app = createTemporaryObject(myFormComponent, root)
            verify(!!app, "Component exists")
            let ageBox = findChild(app, "ageSpinBox")
            verify(!!ageBox, "Object exists")
            ageValueModifiedSpy.target = ageBox
            ageValueModifiedSpy.clear()
            let before = ageBox.value
            mouseClick(ageBox.up.indicator)
            tryCompare(ageValueModifiedSpy, "count", 1)
            verify(ageBox.value > before)
        }

        function test_priceSpinBoxValueModified() {
            let app = createTemporaryObject(myFormComponent, root)
            verify(!!app, "Component exists")
            let priceBox = findChild(app, "priceSpinBox")
            verify(!!priceBox, "Object exists")
            priceValueModifiedSpy.target = priceBox
            priceValueModifiedSpy.clear()
            let before = priceBox.value
            mouseClick(priceBox.up.indicator)
            tryCompare(priceValueModifiedSpy, "count", 1)
            verify(priceBox.value > before)
        }
    }
}
```

`clear()` is required when the same spy is reused across test
functions; on a fresh spy in a fresh test it is still safe and
preferred for explicitness.

## Variant 5 — Dialog standard buttons

`Dialog`, `FileDialog`, `FolderDialog`, `ColorDialog`, and
`MessageDialog` expose standard buttons (Ok, Cancel, Apply,
etc.) via `dialog.standardButton(Dialog.Ok)`.

```qml
import QtQuick
import QtTest
import QtQuick.Controls

<source-import>

Item {
    id: root
    width: 800
    height: 600

    Component {
        id: confirmDialogComponent
        ConfirmDialog {}
    }

    SignalSpy {
        id: acceptedSpy
        signalName: "accepted"
    }

    TestCase {
        name: "ConfirmDialogTests"
        when: windowShown

        function test_okButtonAccepts() {
            let dialog = createTemporaryObject(confirmDialogComponent, root)
            verify(!!dialog, "Component exists")
            dialog.open()
            acceptedSpy.target = dialog
            acceptedSpy.clear()
            let okButton = dialog.standardButton(Dialog.Ok)
            verify(!!okButton, "Object exists")
            mouseClick(okButton)
            tryCompare(acceptedSpy, "count", 1)
        }
    }
}
```

For `MenuItem` signals (rule 16), open the menu before clicking:

```qml
function test_menuItemTriggered() {
    let app = createTemporaryObject(myMenuComponent, root)
    verify(!!app, "Component exists")
    let menu = findChild(app, "fileMenu")
    verify(!!menu, "Object exists")
    let openItem = findChild(menu, "openMenuItem")
    verify(!!openItem, "Object exists")
    triggeredSpy.target = openItem
    triggeredSpy.clear()
    menu.open()
    mouseClick(openItem)
    tryCompare(triggeredSpy, "count", 1)
}
```

## Variant 6 — Press / move / release for cancel and drag

For `Button` cancel signals and `MouseArea` `onPositionChanged`
(rule 24), simulate a press, a move outside the item bounds, and
a release.

```qml
function test_buttonCancel() {
    let app = createTemporaryObject(myFormComponent, root)
    verify(!!app, "Component exists")
    let button = findChild(app, "submitButton")
    verify(!!button, "Object exists")
    canceledSpy.target = button
    canceledSpy.clear()
    mousePress(button, button.width / 2, button.height / 2)
    mouseMove(button, -10, -10)
    mouseRelease(button, -10, -10)
    tryCompare(canceledSpy, "count", 1)
}
```

The `-10, -10` coordinates are deliberately outside the button
bounds so the press is interpreted as canceled rather than
clicked.

## Variant 7 — Window / ApplicationWindow

Use when the source QML's top-level type is `Window`,
`ApplicationWindow`, or a derivative. `createTemporaryObject`
under an `Item` parent does not work for windows — they
always become top-level, sidestepping the visual tree the
runner expects and breaking `when: windowShown` semantics for
the contained scene. Load the source as a `Component` via
`Qt.createComponent` and create the window object directly.

**Pick the URL form by source location** (see table below).
Library-backed modules use the qrc form; executable-backed and
no-CMake-info cases use `Qt.resolvedUrl("../<File>.qml")`.
Absolute `file:///` is an escape hatch for symlinked test trees.

```qml
import QtQuick
import QtTest

Item {
    id: root
    width: 800
    height: 600

    readonly property string windowUrl: "qrc:/qt/qml/MyAppUi/MyWindow.qml"

    TestCase {
        name: "MyWindowTests"
        when: windowShown

        function test_componentCompiles() {
            let comp = Qt.createComponent(root.windowUrl)
            tryCompare(comp, "status", Component.Ready)
            verify(comp.status === Component.Ready, comp.errorString())
            comp.destroy()
        }

        function test_defaultTitle() {
            let comp = Qt.createComponent(root.windowUrl)
            tryCompare(comp, "status", Component.Ready)
            let win = comp.createObject(null, { availableStyles: ["Basic"] })
            verify(win !== null)
            compare(win.title, qsTr("My App"))
            win.destroy()
            comp.destroy()
        }
    }
}
```

### URL form by source location

| Source's QML module backing | URL to use |
|---|---|
| `qt_add_library(... STATIC\|SHARED)` + `qt_add_qml_module(<lib> URI "<URI>" ...)` | `"qrc:/qt/qml/<URI>/<File>.qml"` |
| `qt_add_executable(<exe>)` + `qt_add_qml_module(<exe> URI "<URI>" ...)` | `Qt.resolvedUrl("../<File>.qml")` |
| Source on disk, no CMake info available | `Qt.resolvedUrl("../<File>.qml")` |

URL form notes:

- qrc form survives symlinks, `-input <other-dir>` runs, and
  test-tree relocation — but is unreachable from a sibling
  test binary when the source belongs to an executable-backed
  module (the qrc lives in the app, not the test).
- `Qt.resolvedUrl("../<File>.qml")` is portable and machine-
  independent; it breaks only under symlinked test trees,
  where the URL resolves against the symlink rather than its
  target.
- Use absolute `file:///<absolute>/<File>.qml` **only** as an
  escape hatch for that symlink case. Never as the default —
  it hardcodes machine layout and embeds the developer's home
  directory.

Variant notes:

- Pass `null` as the `createObject` parent so the window is
  not reparented under the test `Item`.
- The second argument to `createObject` must supply every
  `required property` the source declares; without them the
  load fails silently and `win` is `null`.
- Always destroy both `win` and `comp` at the end of the
  function — the test framework will not GC the window for
  you.
- `tryCompare(comp, "status", Component.Ready)` handles
  Components that are loaded asynchronously; for sources
  loaded synchronously it returns immediately.
- **Prefer mouse events over `keyClick` for input
  assertions in this variant.** The spawned `win` is a
  second top-level window alongside the runner's own
  `TestCase` window. On Wayland, macOS, and offscreen
  platforms it does not automatically become the active
  window — synthetic keys go to the runner's window, not
  `win`. `forceActiveFocus()` only forces focus *within*
  the active window, so it cannot rescue this. Mouse helpers
  (`mouseClick`, `mousePress`/`mouseMove`/`mouseRelease`)
  post directly to the target item and bypass the
  window-activation path; use them instead whenever a key
  event has a mouse equivalent.

## Variant 8 — Singleton access

Use when the source QML starts with `pragma Singleton` (or is
declared a singleton via CMake's `QT_QML_SINGLETON_TYPE TRUE`).
Singletons cannot be instantiated by user code — the test
accesses the singleton through the module name and exercises
its read/write properties.

```qml
import QtQuick
import QtTest

<source-import>

Item {
    id: root
    width: 200
    height: 200

    TestCase {
        name: "MyConfigTests"
        when: windowShown

        function test_singletonAccessible() {
            verify(MyConfig !== null)
        }

        function test_disabledDefault() {
            let original = MyConfig.disabled
            MyConfig.disabled = false
            compare(MyConfig.disabled, false)
            MyConfig.disabled = original   // restore for other tests
        }

        function test_disabledWritable() {
            let original = MyConfig.disabled
            MyConfig.disabled = true
            compare(MyConfig.disabled, true)
            MyConfig.disabled = false
            compare(MyConfig.disabled, false)
            MyConfig.disabled = original
        }
    }
}
```

Notes:

- Do **not** declare `Component { MyConfig {} }`. The QML
  engine refuses to instantiate singleton types and the test
  file will fail to compile.
- Singletons are process-wide and persist across test
  functions. Always capture the original value before mutating
  a writable property, and restore it at the end of the
  function so subsequent tests are not order-sensitive.
- If the singleton is registered via CMake's
  `QT_QML_SINGLETON_TYPE TRUE` and the module's backing
  target is an executable (not a library), the singleton is
  not reachable from a sibling test binary via on-disk
  imports — the project needs the module-on-executable
  refactor (see the `qt-qml-test-run` skill's CMake
  reference).

## Notes

- Always include `when: windowShown` on `TestCase`. Without it,
  rendering- and focus-dependent tests run before the scene is
  visible and produce false negatives.
- The base `Item` width and height are illustrative; choose
  values that comfortably contain the component being tested
  (rule 2).
- Replace `<source-import>` and the component type names with
  the actual module URI (or relative directory import) and
  types from the source QML file path. See SKILL.md
  § Resolving the source import for the resolution rules.
