# Qt Quick Test — pitfalls

Symptom-keyed list of mistakes to avoid when generating Qt
Quick Test cases. Many of these are codified as negative rules
in `SKILL.md`; this reference groups them by the symptom a
reader would notice.

## Synthetic mouse events do not reach the control

Symptom: `mouseClick` / `mousePress` / `mouseRelease` runs
without error, but the spy stays at `count: 0` and a
`tryCompare` times out — most often around 5 s per test.

- **Component parented to the `TestCase`.** The default parent
  for `createTemporaryObject(comp)` is the surrounding
  `TestCase`, which has `visible: false`. Children created
  under it never enter the visual tree, so synthetic events
  never hit them. Always pass the outer `Item { id: root }`
  as the parent: `createTemporaryObject(comp, root)`. This is
  the single most common reason for click tests to fail
  silently.
- **Tested control has zero or negative size.** When a
  control's `width` or `height` is bound to an out-of-scope
  identifier (e.g., `parent.width - 2` where `parent.width`
  resolves to `undefined`), the control collapses to a
  non-positive area and synthetic events miss. Set the
  control's size explicitly inside the test before clicking,
  or apply rule 40 and skip the click test.
- **Implicit / layout-driven size on `QT_QPA_PLATFORM=offscreen`.**
  Even when the source type declares `implicitWidth` /
  `implicitHeight`, the offscreen platform runner sometimes
  dispatches the synthetic click before the layout pass has
  fixed the final size, hitting a 0×0 instance. Set explicit
  `width` and `height` on the inline `Component { ... }`
  definition (rule 44). The 5 s `tryCompare` timeout is the
  giveaway — a working click test typically finishes in
  ~100 ms.

## Test passes locally, fails in CI

- **`SignalSpy` attached after the action that emits.** Set
  `spy.target = control` and call `spy.clear()` **before** the
  action that triggers the signal (rule 12). A spy attached
  after the emit reports `count: 0` and the `tryCompare` times
  out.
- **Reliance on locale-specific number formatting.** Use
  `99.99` in tests, never `99,99` or other locale-dependent
  forms (rule 32).
- **Reliance on translation state.** Wrap user-visible strings
  with `qsTr()` consistently (rule 33).

## Flaky / intermittent failures

- **Asserting an animated property immediately after starting
  the animation.** Animations are asynchronous; use
  `tryCompare` against the target property, with a timeout
  comfortably larger than the animation duration (rule 36).
- **Hardcoded `wait(N)` on `valueModified` of a `SpinBox`.**
  Do not use `wait` for `SignalSpy` on `valueModified` (rule 15). Use `tryCompare(spy, "count", N)` instead.
- **Plain `compare` (or `verify`) after a mouse event.** Use
  `tryCompare` for every property or spy assertion that
  follows a `mousePress` / `mouseMove` / `mouseRelease` /
  `mouseClick` / `mouseDoubleClickSequence` (rule 27).
  `tryCompare` polls and runs the event loop between checks,
  so `wait(100)` between the action and the assertion is
  unnecessary — pair the mouse event directly with
  `tryCompare`. Reserve plain `compare` for state read
  *before* any input event.
- **Testing `cursorVisible`** (rule 11). Cursor visibility is
  driven by focus and a blink timer; unit tests cannot reliably
  observe it.

## Test does not get discovered or run

- **Missing `import QtTest`** at the top of the file.
- **Missing `when: windowShown`** on the `TestCase`. Without
  it, rendering- and focus-dependent assertions run before the
  scene exists.
- **Function not prefixed `test_`.** Only functions named
  `test_<something>` are picked up by Qt Quick Test as tests.

## "Insufficient arguments" when triggering a pointer-handler signal

Symptom: `tryCompare(tappedSpy, "count", 1)` fails with the
JUnit message
`Uncaught exception: Insufficient arguments`, often within a
few milliseconds.

- **Calling `tapHandler.tapped()` directly** (rule 43).
  `TapHandler.tapped(QEventPoint, Qt::MouseButton)` is a
  signal with required arguments; QML cannot synthesize them
  for a bare invocation. Drive the handler with
  `mouseClick(hostItem)` against the visual item that owns the
  handler. Same rule for `HoverHandler`, `DragHandler`, and
  the other pointer handlers — their signals all carry
  required event-point / button payloads.

## Binding assertion fails because a `State` overrides it

Symptom: `compare(d.flag, expected)` reports the *declared
binding's* RHS value did not take effect after the test
mutated its source property.

- **Active state's `PropertyChanges` overrides the binding**
  (rule 40 sub-rule). When the source declares
  `states: [ State { when: ...; PropertyChanges { swipe.enabled: true } } ]`,
  the bound output is driven by whichever state is active,
  not by the sibling `swipe.enabled: !root.done` binding.
  Drive the *state guard* (`isEditMode`) in the test, or
  omit the assertion.

## False negatives on text input

- **Using `keyClick()` for text input** (rule 25). Assign the
  desired string directly to the `text` property after setting
  `focus = true`. `keyClick` produces individual key events
  that do not match how text input is normally driven and is
  unreliable for verification.
- **Using `Qt.Key_At`, `Qt.Key_Dollar`, `Qt.Key_Percent`, or
  `Qt.Key_Hash`** (rule 29). These keys map to platform- and
  layout-specific scancodes and produce inconsistent results.
  If the test needs special characters, assign them via the
  `text` property.

## Tests that should not exist

- **Asserting `appControl` size** (rule 8). The top-level
  size is set by the test host's window, not by the source
  under test.
- **Asserting `anchors`** properties (rule 9). Anchors describe
  layout intent, not values to verify.
- **Asserting `currentIndex`** (rule 10). Test the
  observable side effect (signal, derived property) instead.
- **Tests for properties that depend on out-of-scope
  components** (rule 40). When a binding's right-hand side
  references something not loaded in the test, omit the test
  for that property.

## Style / convention drift

- **Custom messages on `compare` and `verify`** beyond the
  canonical "Component exists" / "Object exists" idioms (rule 30). The default Qt Test failure messages are sufficient and
  consistent.
- **`'transparent'` instead of `'#00000000'`** for transparent
  colors (rule 31).
- **Mixed-case hex colors.** Use lowercase: `'#ff0000'`, not
  `'#FF0000'` (rule 31).
- **`mouseDoubleClick` instead of `mouseDoubleClickSequence`**
  (rule 26).

## SignalSpy issues

- **Reusing a spy across targets without `clear()`** (rule 12).
  When changing the spy's target inside a test, call
  `spy.clear()` before the next action.
- **One spy for multiple controls.** With multiple similar
  controls, declare one spy per target with descriptive IDs
  (`ageAcceptedSpy`, `priceAcceptedSpy`) and set targets
  independently (rules 21, 22).
- **Spying on a signal not declared in the source.** Add
  `SignalSpy` only for signals defined in the tested code
  (rule 12). Do not invent signal names.

## findChild misuse

- **Empty `objectName` on `findChild`** (rule 4). Always pass a
  non-empty string and ensure the source declares
  `objectName: "..."` on the descendant.
- **Skipping the existence check** after `findChild` (rule 5).
  Always follow with `verify(!!object, "Object exists")`.

## Source descendant has only an `id`, not an `objectName`

Symptom: the generated test contains only thin outer-scope
assertions (component compiles, Window `visible`) when the
source clearly has interactive children — a `Button` with
press/release handlers, a `MultiEffect` whose properties
mutate, a `Slider`, etc. — that look test-worthy. Rerunning
the skill regenerates the same thin test.

- **`id`s are document-scoped; cross-document lookup is
  `objectName`-only** (rule 46). The test file cannot resolve
  `id: button` declared in `Main.qml` — `findChild` matches
  on `objectName`, not `id`. When the source's test-worthy
  children lack `objectName`, the skill asks the user once
  whether to add `objectName` declarations and extend
  coverage. If accepted, it applies minimal source edits
  (one `objectName: "<id>"` per item, matching the existing
  `id`) and generates against the edited source. If declined
  or no user is available, it skips the affected assertions
  and lists each unreached item in the final reply with the
  one-line edit needed.
- **Do not work around with `parent.children[0]`,
  `contentItem.data` walks, or `toString()` matching.** They
  pass for the wrong reasons and break on cosmetic source
  edits (reordering, type renames), giving false confidence
  while losing the coverage signal. Add `objectName` to the
  source instead.
- Applies to the nested-component (Variant 2) and
  Window / ApplicationWindow (Variant 7) variants. The
  single-component (Variant 1) and singleton (Variant 8)
  variants do not reach into children, so they are
  unaffected.

## Things that look like a problem but are not

- The `Item` `width` and `height` are illustrative; choose
  values that comfortably contain the component being tested
  (rule 2). They do not need to match production sizing.
- A spy declared at `TestCase` scope without a target set is
  fine — the target is set inside the test function. The spy
  is inert until then.
