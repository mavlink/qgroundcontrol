# Qt Quick Test — testing rules (full text)

The 47 rules that form the contract of the `qt-qml-test`
skill. `SKILL.md` carries a one-line index per rule under the
same category headings; load this reference for the
normative text, examples, and rationale.

Apply every rule relevant to the component under test.

## Imports & structure

1. Use `QtQuick` and `QtTest` modules without specifying versions.

   Add Qt-shipped imports the test actually references:

   - **`import QtQuick.Controls`** — only when the test code
     itself references a Controls identifier by name
     (`Slider.SnapAlways`, `Dialog.Ok`, `Popup.CloseOnEscape`,
     `Button.AutoRepeat`, etc.). The source's use of a
     Controls type does **not**, by itself, require the
     import in the test: `findChild` returns a generic
     QObject reference whose properties resolve dynamically,
     so `button.text` and `button.background.radius` work
     without `import QtQuick.Controls`. The source's
     style-qualified import (`QtQuick.Controls.Basic`) brings
     the type in for the source, not for the test's script
     scope.
   - **`import QtQuick.Layouts`** — when test code reads or
     writes `Layout.fillWidth`, `Layout.preferredHeight`, etc.

   Do not add unreferenced framework imports. When in doubt,
   omit — a missing import surfaces as a `ReferenceError` at
   runtime and is easy to add then.

2. Set `Item` width and height appropriate to the tested component.

## Single vs nested components

3. When testing a single component, use direct reference:

   ```
   let mycomponent = createTemporaryObject(myComponent, root)
   verify(!!mycomponent, "Component exists")
   ```

   Always pass `root` (the outer `Item`) as the parent — the
   default parent is the `TestCase`, which is `visible: false`
   and silently breaks input events.

4. When testing nested components, start test functions with
   `let app = createTemporaryObject(myComponent, root)`. Access
   children through the parent: `let object = findChild(app,
   objectName)`. Do not use empty `objectName`.
5. After `findChild`, always verify: `verify(!!object, "Object
   exists")`.

## Properties

6. For the `background` property, use `.background` accessor
   (e.g., `dial.background`).
7. Propose tests for explicitly defined properties only.
8. Do NOT test the `appControl` size.
9. Do NOT test `anchors` properties.
10. Do NOT test `currentIndex`.
11. Do NOT test `cursorVisible`.

## Signals & SignalSpy

12. Add `SignalSpy` only for signals defined in the tested code.
    Test signals in SEPARATE test functions. Define spy target
    before testing: `clickSpy.target = button`. Clear the spy
    before changing targets: `clickSpy.clear()`.
13. `Slider` signals — see rule 12.
14. `SpinBox` signals — see rule 12.
15. Do NOT use `wait` method for `SignalSpy` on a `valueModified`
    signal.
16. `MenuItem` signals — open the menu before clicking, then
    compare spy value (otherwise rule 12).
17. `TapHandler` / `HoverHandler` signals — rule 12, plus
    *trigger* via `mouseClick(<hostItem>)`; do not call the
    signal directly (rule 43).
18. `Accessible` type signals (e.g. `onPressAction`) — see rule 12.
19. `Dialog`, `FileDialog`, `FolderDialog`, `ColorDialog`,
    `MessageDialog` signals — see rule 12.
20. `MouseArea` signals — see rule 12.
21. Use separate `SignalSpy` instances per target with
    descriptive IDs (e.g. `ageAcceptedSpy`, `priceAcceptedSpy`).
22. Same as rule 21 when testing multiple similar controls.

## Mouse & key events

23. Set focus before testing input components: `emailField.focus
    = true`.
24. For Button cancel signals and Mouse `onPositionChanged`,
    simulate with:

    ```
    mousePress(button, button.width / 2, button.height / 2)
    mouseMove(button, -10, -10)
    mouseRelease(button, -10, -10)
    ```

    The sequence above is the action, not the test. Follow it
    with an assertion against the outcome — either
    `tryCompare(canceledSpy, "count", 1)` on a spy targeting
    the source's `canceled` signal, or `tryCompare` on a
    property the press mutated to confirm the
    release-handler's restore path did **not** run (the source
    emits `canceled` instead of `released` when the release is
    outside the hit area, so state set by `onPressed` is
    expected to stay). A press/move/release with no following
    assertion is a no-op — see rule 47.

25. Do NOT use `keyClick()` for input text testing.
26. Use `mouseDoubleClickSequence` instead of `mouseDoubleClick`.
27. **Use `tryCompare()` for any property or spy assertion
    that follows a mouse event** — `mousePress`, `mouseMove`,
    `mouseRelease`, `mouseClick`, `mouseDoubleClickSequence`.
    Not just release / doubleclick.

    ```
    mousePress(button, button.width / 2, button.height / 2)
    tryCompare(shadowEffect, "shadowVerticalOffset", 2)
    tryCompare(pressedSpy, "count", 1)
    ```

    Why widen this beyond release: a synchronous JS handler
    (`onPressed: shadowEffect.shadowVerticalOffset = 2`)
    works with immediate `compare`, but the moment the source
    adds a `Behavior on shadowVerticalOffset` or a
    `NumberAnimation` the binding becomes asynchronous and
    immediate `compare` races it. `tryCompare` polls up to
    5 s and survives the future addition. `wait(100)` is a
    weaker heuristic — prefer `tryCompare` in every position
    where it can stand in for `wait` + `compare`. Reserve
    plain `compare` for state read **before** any input
    event.

28. For focus-change-triggered property changes: explicitly set
    `focus = false` or `focus = true` before testing the
    outcome.
29. Do NOT use `Qt.Key_At`, `Qt.Key_Dollar`, `Qt.Key_Percent`,
    `Qt.Key_Hash`.

## Conventions

30. **Custom messages in `compare` and `verify` are not
    allowed, except for these three canonical forms** (which
    every template variant uses):

    - `verify(!!x, "Object exists")` — after `findChild` and
      for any handle returned by `createTemporaryObject` that
      isn't the top-level component.
    - `verify(!!x, "Component exists")` — for the top-level
      component instantiation in single-component tests.
    - `verify(comp.status === Component.Ready, comp.errorString())`
      — for `Qt.createComponent` status checks in Variant 7.
      `errorString()` is the diagnostic value of the message;
      keep it.

    Every other `verify` and `compare` call takes no message.
    In Variant 7, the post-`createObject` check is
    `verify(win !== null)` with no string — do not extend it
    to `verify(win !== null, "Window exists")` or similar.
    Mixing the bare and string-form within one test function
    is the inconsistency to avoid.
31. Use hex values for colors in lowercase: `'#ff0000'`. Use
    `'#00000000'` instead of `'transparent'`.
32. Use standard JavaScript decimal notation in test functions:
    `99.99` (not locale-specific comma separators).
33. Use `qsTr()` for text values: `qsTr("Button1")`.

## Per-control specifics

34. For `TextArea`, `TextEdit`, `TextInput`, `TextField`: test
    various inputs including characters, numbers, and special
    characters.
35. For `Dial` testing: add a test case to verify value change
    by simulating a move of the handle.
36. For `NumberAnimation`: use `tryCompare` to ensure animation
    completes before verifying values.
37. For `Image` testing: include tests for whether the image
    loads successfully.
38. For `RegularExpressionValidator`: test both correct and
    incorrect input values.
39. For Dialog standard buttons: find the correct button with
    `let okButton = dialog.standardButton(Dialog.Ok)`.

## Property dependencies

40. When a property depends on QML components not in testing
    scope, exclude tests for that property.

    Also applies when the property's effective value is set by
    a `State { PropertyChanges { ... } }` block. Setting the
    right-hand side of a sibling binding does not flip the
    bound output — the active state's `PropertyChanges`
    overrides it. Drive the state's *guard* property in the
    test, or skip the assertion.

## Window and singleton sources

41. **`Window` / `ApplicationWindow` sources** must not be
    instantiated via `createTemporaryObject(component, root)`
    — windows ignore the `Item` parent, become top-level, and
    break `when: windowShown`. Use
    `Qt.createComponent(<url>)` +
    `comp.createObject(null, { requiredProperty: … })`.
    Supply every `required property` the source declares.
    Destroy `win` and `comp` at the end of each test function.

    **URL form:** see
    [qt-quick-test-template.md § Variant 7 § URL form by source location](qt-quick-test-template.md#variant-7--window--applicationwindow).
    The table there distinguishes static/shared library
    backings, `qt_add_executable`-backed modules (whose
    sources are **not** in the test binary's `qrc:/`), and
    the no-CMake-info fallback — it is the single source of
    truth for URL selection. Do not embed an alternative
    priority list here; it would drift out of sync.

42. **Sources marked `pragma Singleton`** (or registered via
    `QT_QML_SINGLETON_TYPE TRUE` in CMake) are accessed by
    name, not instantiated. Import the singleton's module and
    read/write its properties directly
    (`MySingleton.someProperty = …`). Never wrap a singleton
    in `Component { MySingleton {} }` — the engine refuses.
    Restore mutated writable properties at the end of each
    test function so other tests see a known state.

## Triggering pointer-handler signals

43. **Never invoke a pointer handler's signal as a function.**
    `TapHandler.tapped`, `HoverHandler.hoveredChanged`,
    `DragHandler.translationChanged`, etc. carry required
    `(QEventPoint, Qt::MouseButton)` arguments; calling them
    as functions throws `Insufficient arguments` and fails
    the test. Exercise the handler via `mouseClick(<hostItem>,
    …)` on the visual item that owns it. For non-rendering
    hosts (`Image { source: "" }`), give them a measurable
    size first.

## Sizing click targets

44. **Set explicit `width` and `height` on inline component
    definitions whose tested type uses implicit /
    layout-driven sizing.** Under `QT_QPA_PLATFORM=offscreen`,
    an instance sized by `Layout.fillWidth`, anchors, or
    implicit metrics may be 0×0 when the synthetic click
    dispatches; `tryCompare` then burns the 5 s timeout.
    `implicitWidth`/`implicitHeight` are starting points, not
    size guarantees. Use:

    ```qml
    Component {
        id: myButtonComponent
        MyButton { width: 100; height: 40; text: qsTr("Press") }
    }
    ```

## Qt Quick 3D source handling

45. **Qt Quick 3D graphical-node sources** (`Model`, `Node`,
    `*Camera`, `*Light`, `Skybox`, `SceneEnvironment`, or
    other `QtQuick3D` node types needing a scene) wrapped in
    the canonical `Item { id: root }` template create the
    object outside any graphics scene — the test reports PASS
    while a runtime `QWARN: Created graphical object was not
    placed in the graphics scene` fires. **Skip** these
    sources; note the type in the final reply. View3D-rooted
    sources and `*Material` types fall through to the
    standard template.

## Unreachable inner items

46. **Inner items the test would meaningfully exercise must
    declare `objectName`; items with only an `id` are
    unreachable from a sibling test file.** QML `id`s are
    scoped to the declaring document — `findChild` and every
    cross-document lookup resolves by `objectName`. When the
    source's test-worthy children (controls whose signals
    fire, properties the source mutates at runtime, effects
    whose values change on interaction) lack `objectName`,
    ask the user once whether to add `objectName`
    declarations and extend coverage. If accepted, apply
    minimal source edits — one `objectName: "<id>"` per item,
    matching the existing `id`, on the same item, no other
    changes — then generate against the edited source. If an
    item already carries a non-matching `objectName`, do
    **not** overwrite; reach it by the existing name. If
    declined or no user is available, skip the affected
    assertions and list each unreached item by `id` and
    source line in the final reply with the one-line edit
    needed.

    Outer-scope properties remain testable without
    `objectName`s and should still be covered: the Window's
    `title`/`visible`, root-component `property` aliases,
    singleton properties (rule 42). Do **not** fall back to
    fragile workarounds — `parent.children` indexing,
    `toString()` matching, or visual-tree walks pass for the
    wrong reasons and break on cosmetic source edits.

    Applies to nested-component (Variant 2) and Window /
    ApplicationWindow (Variant 7) sources. Single-component
    (Variant 1) and singleton (Variant 8) sources are
    unaffected.

## No-op test functions

47. **Every test function must contain at least one assertion
    against state the test's actions are expected to change.**
    `compare`, `tryCompare`, or `tryCompare(spy, "count", N)`
    all qualify; existence checks
    (`verify(!!x, "Object exists")`) count as prerequisites,
    **not** as the test body. A function whose body is only
    setup + actions + `wait` passes regardless of behavior —
    it tests nothing.

    Symptom: a `test_*` that mutates input (mouse, key,
    property write) but ends without a `compare` or
    `tryCompare` referencing the affected output. The runner
    reports PASS; a real regression in the action's handler
    would still report PASS.

    Resolution: name the expected post-action state and
    assert it. For pointer-handler / signal flows, target a
    `SignalSpy` and `tryCompare` its `count`. For property
    mutation, `tryCompare` the property to its expected new
    value. For paths that should *not* change a property
    (e.g. cancel sequences where `onReleased` must not run),
    `tryCompare` the property to the unchanged value it
    should retain.

    Mechanical check before emitting a generated test:
    scan each `test_*` function body. If it contains a
    mouse, key, or property-write statement but no
    `compare` / `tryCompare` other than existence checks,
    either add the missing assertion or delete the test —
    do not emit it as-is.
