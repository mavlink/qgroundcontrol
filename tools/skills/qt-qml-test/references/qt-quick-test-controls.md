# Qt Quick Test — control-specific patterns

One section per Qt Quick Control. Each section covers what to
test, how to interact with the control, and how to verify its
signals. Apply the rules from `SKILL.md` (and the cross-cutting
items in `qt-quick-test-pitfalls.md`) on top of these patterns.

The examples assume the canonical template from
`qt-quick-test-template.md` and a `findChild`-based lookup of
the control by `objectName`.

## Common shape

Every example below begins with this setup inside each test
function. It is shown here once and **elided** from the
per-control examples that follow (those start at the line
after `findChild`):

```qml
let app = createTemporaryObject(myFormComponent, root)
verify(!!app, "Component exists")
let <name> = findChild(app, "<objectName>")
verify(!!<name>, "Object exists")
```

The only allowed `verify` messages are `"Component exists"`,
`"Object exists"`, and a `Component.errorString()` (rule 30).

`tryCompare` polls (default 5 s timeout) and runs the event
loop between checks, so there is **no need** for `wait()`
between an action and the matching `tryCompare` assertion.
`mouseClick` and related helpers deliver events synchronously
before returning.

## Interactive-only signals

Some signals fire only on real user input — touch, mouse,
wheel, or keys. Programmatic property assignment fires
`valueChanged` (or the equivalent property-change signal) but
**not** the interactive signal. Drive these via the right
column; never assert them after a direct property assignment.

| Control      | Interactive signal      | How to drive                                                                                                                                              |
|--------------|-------------------------|-----------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Slider`     | `moved`                 | `mousePress` / `mouseMove` / `mouseRelease` on the handle, or `s.forceActiveFocus()` then `keyClick(s, Qt.Key_Right)`                                     |
| `Dial`       | `moved`                 | `mousePress` / `mouseMove` / `mouseRelease` on the dial (most reliable), or `d.forceActiveFocus()` then `keyClick(d, Qt.Key_Up)`                          |
| `SpinBox`    | `valueModified`         | `mouseClick(sb.up.indicator)` or `mouseClick(sb.down.indicator)`                                                                                          |
| `SpinBox`    | `accepted`              | `sb.contentItem.forceActiveFocus()` then `keyClick(sb.contentItem, Qt.Key_Return)`                                                                        |
| `Button`     | `clicked`               | `mouseClick(button)`                                                                                                          |
| `Button`     | `canceled`              | `mousePress(button, x, y)` then `mouseMove(button, -10, -10)` then `mouseRelease(button, -10, -10)` (rule 24)                 |
| `MenuItem`   | `triggered`             | `menu.open()` then `mouseClick(item)`                                                                                         |
| `Dialog`     | `accepted` / `rejected` | `dialog.open()` then `mouseClick(dialog.standardButton(Dialog.Ok))` (rule 39)                                                 |
| `MouseArea`  | `clicked` / `pressed` … | `mouseClick(area)` / `mousePress(area)` / etc.                                                                                |
| `TapHandler` | `tapped`                | `mouseClick(<host item>)` — dispatch to the handler's host, not the handler itself (rule 43)                                  |

If a test only needs to react to a value change regardless of
how it was triggered, spy on `valueChanged` and assign `value`
directly — but per rule 12 only when the source declares an
`onValueChanged` handler.

## Button

Test `text`, the `clicked` signal, and any custom signals
declared on the source.

```qml
SignalSpy {
    id: clickSpy
    signalName: "clicked"
}

SignalSpy {
    id: canceledSpy
    signalName: "canceled"
}

function test_buttonClicked() {
    // setup elided; binds the button to `button`
    clickSpy.target = button
    clickSpy.clear()
    mouseClick(button)
    tryCompare(clickSpy, "count", 1)
}

function test_buttonCanceled() {
    // setup elided
    canceledSpy.target = button
    canceledSpy.clear()
    mousePress(button, button.width / 2, button.height / 2)
    mouseMove(button, -10, -10)
    mouseRelease(button, -10, -10)
    tryCompare(canceledSpy, "count", 1)
}
```

## TextField, TextArea, TextEdit, TextInput

Cover several input categories per rule 34: alphabetic, numeric,
and special characters. Set `focus = true` before assigning
`text`. Do **not** use `keyClick` for text input (rule 25) and
do **not** test `cursorVisible` (rule 11).

```qml
function test_textFieldAcceptsCharacters() {
    // setup elided; binds the field to `nameField`
    nameField.focus = true
    nameField.text = qsTr("Alice")
    compare(nameField.text, qsTr("Alice"))
}

function test_textFieldAcceptsNumbers() {
    // setup elided
    nameField.focus = true
    nameField.text = qsTr("12345")
    compare(nameField.text, qsTr("12345"))
}

function test_textFieldAcceptsSpecialCharacters() {
    // setup elided
    nameField.focus = true
    nameField.text = qsTr("Name & Co. (1985)")
    compare(nameField.text, qsTr("Name & Co. (1985)"))
}
```

If the field has a `RegularExpressionValidator` attached, test
both an accepted and a rejected input — see
"RegularExpressionValidator" below.

## Slider

`moved` is interactive-only — see "Interactive-only signals".

```qml
SignalSpy {
    id: sliderMovedSpy
    signalName: "moved"
}

function test_sliderValue() {
    // setup elided; binds the slider to `slider`
    slider.value = 0.75
    compare(slider.value, 0.75)
}

function test_sliderMoved() {
    // setup elided
    sliderMovedSpy.target = slider
    sliderMovedSpy.clear()
    let before = slider.value
    slider.forceActiveFocus()
    keyClick(slider, Qt.Key_Right)
    tryCompare(sliderMovedSpy, "count", 1)
    verify(slider.value > before)
}
```

## SpinBox

`valueModified` and `accepted` are interactive-only — see
"Interactive-only signals". For `valueModified` specifically,
do **not** use `wait` on the spy (rule 15) — use `tryCompare`
against `count`. Use plain JavaScript number literals
(`99.99`), never locale-specific separators (rule 32).

```qml
SignalSpy {
    id: valueModifiedSpy
    signalName: "valueModified"
}

function test_spinBoxValueModified() {
    // setup elided; binds the spinbox to `ageBox`
    valueModifiedSpy.target = ageBox
    valueModifiedSpy.clear()
    let before = ageBox.value
    mouseClick(ageBox.up.indicator)
    tryCompare(valueModifiedSpy, "count", 1)
    verify(ageBox.value > before)
}
```

## Dial

`moved` is interactive-only — see "Interactive-only signals".
For background tests, use the `.background` accessor (rule 6).

Prefer **mouse-drag** over `keyClick` for driving the dial. A
single drag fires `moved` more than once (once during
`mouseMove`, again on `mouseRelease`), so assert the spy
count with `>= 1` — not `tryCompare(spy, "count", 1)`. The
exact value the drag lands on depends on the dial's geometry
and `inputMode`; compare `value !== before` rather than to a
specific number.

```qml
SignalSpy {
    id: dialMovedSpy
    signalName: "moved"
}

function test_dialValueChange() {
    // setup elided; binds the dial to `dial`
    dialMovedSpy.target = dial
    dialMovedSpy.clear()
    let before = dial.value
    mousePress(dial, dial.width / 2, dial.height / 2)
    mouseMove(dial, dial.width - 5, dial.height / 2)
    mouseRelease(dial, dial.width - 5, dial.height / 2)
    verify(dialMovedSpy.count >= 1)
    verify(dial.value !== before)
}

function test_dialBackgroundColor() {
    // setup elided
    compare(dial.background.color, '#ff0000')
}
```

`keyClick(dial, Qt.Key_Right)` after
`dial.forceActiveFocus()` is the listed alternative and
fires `moved` exactly once, but it depends on the dial's
containing window being the active window. For sources
rooted in `Window` / `ApplicationWindow` (template Variant 7),
the spawned window often isn't active under Wayland / macOS
/ offscreen — synthetic keys land in the runner's window and
the assertion fails. Mouse events bypass window activation
because `QTest::mousePress` posts directly to the target
item. Use the key form only for single-component sources
(template Variant 1) where the runner's `TestCase` window
holds focus.

## Dialog, FileDialog, FolderDialog, ColorDialog, MessageDialog

The dialog **is** the component under test, so it is created
directly — the Common-shape `findChild` step does not apply.
Spy on `accepted`, `rejected`, or another signal declared on
the dialog (rule 19). See "Interactive-only signals" for the
drive pattern.

```qml
SignalSpy {
    id: acceptedSpy
    signalName: "accepted"
}

function test_dialogAcceptsOnOk() {
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
```

`FileDialog`, `FolderDialog`, `ColorDialog`, `MessageDialog`
follow the same shape — spy on the relevant signal, drive the
dialog, assert the count.

## MenuItem

`triggered` is interactive-only — see "Interactive-only
signals". Open the menu before clicking the item (rule 16).

```qml
SignalSpy {
    id: triggeredSpy
    signalName: "triggered"
}

function test_menuItemTriggered() {
    // setup elided; binds the menu to `menu`, item to `openItem`
    triggeredSpy.target = openItem
    triggeredSpy.clear()
    menu.open()
    mouseClick(openItem)
    tryCompare(triggeredSpy, "count", 1)
}
```

## Image

Verify the image loads successfully (rule 37). The
`Image.Ready` status indicates a successful load.

```qml
function test_imageLoadsSuccessfully() {
    // setup elided; binds the image to `logo`
    tryCompare(logo, "status", Image.Ready)
}
```

If the source declares an explicit `source` property,
additionally verify it resolves to the expected URL.

## MouseArea

Test signals (`clicked`, `pressed`, `released`,
`positionChanged`, `entered`, `exited`) by attaching a
`SignalSpy` and setting `target` in the test function (rule 20).
See "Interactive-only signals" for the drive pattern. For drag
tests, use the press / move / release sequence (rule 24).

```qml
SignalSpy {
    id: mouseClickedSpy
    signalName: "clicked"
}

function test_mouseAreaClicked() {
    // setup elided; binds the area to `area`
    mouseClickedSpy.target = area
    mouseClickedSpy.clear()
    mouseClick(area)
    tryCompare(mouseClickedSpy, "count", 1)
}
```

## TapHandler, HoverHandler

Pointer handlers use the same `SignalSpy` + target shape as
`MouseArea` (rule 17), but events are dispatched to the
handler's **host item**, not the handler itself (rule 43). See
"Interactive-only signals" for the drive pattern.

```qml
SignalSpy {
    id: tappedSpy
    signalName: "tapped"
}

function test_tapHandler() {
    // setup elided; binds the handler to `handler`, host to `host`
    tappedSpy.target = handler
    tappedSpy.clear()
    mouseClick(host)
    tryCompare(tappedSpy, "count", 1)
}
```

A `HoverHandler`'s `hoveredChanged` is driven by `mouseMove`
into and out of the host item.

## Accessible signals

Signals declared by `Accessible` (e.g. `onPressAction`) follow
the same `SignalSpy` + target pattern (rule 18). Drive the
underlying control normally; the accessibility signal fires as
a side effect.

```qml
SignalSpy {
    id: pressActionSpy
    signalName: "pressAction"
}

function test_accessiblePressAction() {
    // setup elided; binds the button to `button`
    pressActionSpy.target = button.Accessible
    pressActionSpy.clear()
    mouseClick(button)
    tryCompare(pressActionSpy, "count", 1)
}
```

## NumberAnimation

Animations are asynchronous; use `tryCompare` against the
target property to wait for completion (rule 36). Do not assert
the property immediately after starting the animation.

```qml
function test_fadeOutCompletes() {
    // setup elided; binds the animated item to `item`
    item.opacity = 0
    tryCompare(item, "opacity", 0)
}
```

If the animation drives a chained property, assert the final
value with `tryCompare` and a timeout that comfortably exceeds
the animation duration.

## RegularExpressionValidator

Test both correct and incorrect inputs (rule 38). The input's
`acceptableInput` reflects whether the current text passes.

```qml
function test_emailValidatorAccepts() {
    // setup elided; binds the field to `emailField`
    emailField.focus = true
    emailField.text = qsTr("user@example.com")
    verify(emailField.acceptableInput)
}

function test_emailValidatorRejects() {
    // setup elided
    emailField.focus = true
    emailField.text = qsTr("not-an-email")
    verify(!emailField.acceptableInput)
}
```

## Multi-instance / similar controls

When a layout contains multiple instances of the same control
type (two `SpinBox`es, three `Slider`s), give each its own spy
with a descriptive ID and set targets independently (rules 21,
22). See variant 4 in `qt-quick-test-template.md` for a full
example.

## What this reference does NOT cover

- Build-system wiring (CMake `qt_add_test`, runner harness).
- Test review or refactoring.
- Custom controls outside the `QtQuick.Controls` module — apply
  the same `SignalSpy` + `findChild` patterns and consult the
  control's own QML source for available signals.
