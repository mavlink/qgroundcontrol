---
trigger: model_decision
description: "Review Qt6 QML code for correctness and best practices"
---

# Qt QML Review

**Imports**: No versioned imports in Qt 6. No QtQuick.Window when
QtQuick imported. Use style-specific import for control customization.
Sort: Qt modules, third-party, local, QML folders.

**Bindings**: Use typed properties not `property var`. Imperative `=`
destroys bindings. Watch for multi-cycle binding loops.

**Layout**: Never mix anchors + Layout.* on same item. No bare
width/height on Layout children; use Layout.* properties. Do not
anchor to invisible items. No parent.parent in anchor targets.

**Loader**: Guard Loader.item with status check. No
Qt.createComponent(url) strings. No Qt.createQmlObject(). Track and
destroy dynamic objects.

**Delegates**: Use `required property` for roles. Also declare
`required property int index` when index is accessed. No var in
delegates with reuseItems. No connect() in Component.onCompleted.

**States**: Qt 6 PropertyChanges: `id.prop: val` not `target: id`.
Transitions need explicit from/to. No imperative = in PropertyChanges.

**Signals**: Connections must have explicit target. Use
`function onFoo()` syntax. Never mix old and new syntax in same
Connections block.

**Performance**: Use Item not transparent Rectangle for grouping.
visible: false not opacity: 0. Text.PlainText when possible. Cache
expensive bindings in readonly property.

**Images**: Always set sourceSize. asynchronous: true for network
sources. Check Image.status for error handling on dynamic sources.

**JS**: let/const not var. Strict === not ==.

**Errors**: Use https:// not http://. No hardcoded Unix paths
like /tmp/ -- use QStandardPaths.
