# Qt Quick Test — property testing patterns

How to test the values of properties on a QML component. The
patterns assume the canonical template from
`qt-quick-test-template.md` and a `findChild`-based lookup of
the target object.

Apply rule 7 throughout: only propose tests for properties
**explicitly defined** in the source under test. Do not invent
tests for properties the component inherits but does not
override.

## Default values

Test only **contract defaults** — values downstream code
depends on, where flipping the default changes observable
behaviour. Skip implementation-detail defaults (font sizes,
padding, derived dimensions, style-chain values). Borderline:
if anything outside the component references the value, it's
a contract; otherwise an implementation detail. Rules 8–11
codify specific exclusions; the same principle generalises.

Verify a freshly instantiated component has the expected
initial value.

```qml
function test_defaultText() {
    let mybutton = createTemporaryObject(myButtonComponent, root)
    verify(!!mybutton, "Component exists")
    compare(mybutton.text, qsTr(""))
}

function test_defaultEnabled() {
    let mybutton = createTemporaryObject(myButtonComponent, root)
    verify(!!mybutton, "Component exists")
    compare(mybutton.enabled, true)
}
```

Use `qsTr()` for text values per rule 33.

## Read / write properties

Assign a new value and assert the round-trip.

```qml
function test_textWritesAndReads() {
    let mybutton = createTemporaryObject(myButtonComponent, root)
    verify(!!mybutton, "Component exists")
    mybutton.text = qsTr("Submit")
    compare(mybutton.text, qsTr("Submit"))
}
```

Use plain JavaScript decimal notation for numeric values, never
locale-specific separators (rule 32):

```qml
function test_priceWritesAndReads() {
    let priceBox = createTemporaryObject(priceBoxComponent, root)
    verify(!!priceBox, "Component exists")
    priceBox.value = 99.99
    compare(priceBox.value, 99.99)
}
```

## Computed (binding) properties

When a property is bound to other state, drive that state and
assert the computed value.

```qml
function test_totalUpdatesWhenItemAdded() {
    let cart = createTemporaryObject(cartComponent, root)
    verify(!!cart, "Component exists")
    cart.itemCount = 3
    cart.unitPrice = 10
    compare(cart.total, 30)
}
```

If the binding depends on a focus change, set `focus` explicitly
(rule 28):

```qml
function test_validationStateOnBlur() {
    let app = createTemporaryObject(formComponent, root)
    verify(!!app, "App exists")
    let emailField = findChild(app, "emailField")
    verify(!!emailField, "Object exists")
    emailField.focus = true
    emailField.text = qsTr("not-an-email")
    emailField.focus = false
    verify(!emailField.acceptableInput)
}
```

## The .background accessor

For the `background` property of a styled control, use the
`.background` accessor (rule 6). Do not attempt to assert
`background` directly as a string or color value.

```qml
function test_dialBackgroundColor() {
    let app = createTemporaryObject(myFormComponent, root)
    verify(!!app, "App exists")
    let dial = findChild(app, "tempDial")
    verify(!!dial, "Object exists")
    compare(dial.background.color, '#ff0000')
}
```

Apply the same pattern for `contentItem`, `indicator`, and
similar style hooks exposed as Item subobjects.

## Color properties

Use lowercase hex values for colors (rule 31). Use
`'#00000000'` instead of `'transparent'`.

```qml
function test_warningColor() {
    let badge = createTemporaryObject(badgeComponent, root)
    verify(!!badge, "Component exists")
    badge.severity = "warning"
    compare(badge.color, '#ffaa00')
}

function test_hiddenIsTransparent() {
    let badge = createTemporaryObject(badgeComponent, root)
    verify(!!badge, "Component exists")
    badge.hidden = true
    compare(badge.color, '#00000000')
}
```

## Property aliases

Property aliases are tested as ordinary read/write properties.
Round-trip on the alias and confirm the underlying property
also changes if observable.

```qml
function test_textAliasWritesThrough() {
    let labeledButton = createTemporaryObject(labeledButtonComponent, root)
    verify(!!labeledButton, "Component exists")
    labeledButton.label = qsTr("Save")
    compare(labeledButton.label, qsTr("Save"))
}
```

## Property dependencies on out-of-scope components

When a property depends on another QML component that is not
in the testing scope (e.g. a sibling component the test cannot
instantiate, or an externally provided model), exclude tests
for that property (rule 40).

```qml
// Source component:
//   property string formattedPrice: priceFormatter.format(value)
// where priceFormatter is provided by the parent and not loaded
// in the test scope.

// Do NOT write a test for formattedPrice. Test value directly:
function test_valueAssignment() {
    let priceBox = createTemporaryObject(priceBoxComponent, root)
    verify(!!priceBox, "Component exists")
    priceBox.value = 42
    compare(priceBox.value, 42)
}
```

When in doubt, prefer omission to a fragile test. The output is
a generated test that reads cleanly with what is actually
verifiable.

## Properties that are NOT tested

The following are explicitly excluded by the rules; do not
generate tests for them even when they appear on the source
component:

- **`appControl` size** (rule 8) — top-level size depends on
  the host environment and asserting it produces flaky tests.
- **`anchors` properties** (rule 9) — anchors are layout
  primitives, not values to verify in unit tests.
- **`currentIndex`** (rule 10) — model-state-coupled and prone
  to test-vs-source coupling.
- **`cursorVisible`** (rule 11) — focus- and timing-sensitive,
  unreliable in headless test runs.

If the source under test makes these properties central to its
behaviour, generate tests for the **observable side effects**
instead (e.g. for `currentIndex`, test the signal emitted on
selection change, not the index value itself).

## Conventions

- Do not define custom messages on `compare` and `verify`
  (rule 30). The default failure messages are sufficient and
  consistent.
- Always declare `objectName` on every descendant the test
  reaches via `findChild`, and verify the result with
  `verify(!!object, "Object exists")` (rule 5).
