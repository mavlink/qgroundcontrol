---
trigger: model_decision
description: "Apply Qt6 QML best practices when writing or modifying QML code"
---

# QML Best Practices

**Imports (Qt 6)**: No version numbers. No QtQuick.Window when
QtQuick is imported. Use style-specific import for control
customization. Qt 5 code requires version numbers.

**Bindings**: Prefer declarative `prop: expr` over imperative `=`.
Imperative `=` permanently destroys bindings. Cache expensive
expressions in `readonly property`. No circular dependencies.

**Layouts**: Never mix anchors + Layout.* on same item. Size Layout
children with Layout.* only -- bare width/height breaks negotiation.
Do not anchor to invisible items or across tree branches.

**Loader**: Use for conditional UI. Guard item with status check.
No Qt.createComponent(url) strings -- use inline Component {}.
createObject() only when parent is dynamic.

**Delegates**: Use `required property` for roles. Keep delegates
minimal. reuseItems: true (Qt 6.7+), reset in onPooled. No mutable
JS vars in delegates.

**States**: No `target` in PropertyChanges (Qt 6); use
`id.prop: val`. Target transitions with from/to.

**Images**: Always set sourceSize. asynchronous: true for network
or large files. Check Image.status.

**Pitfalls**: `parent` in delegates is the internal container, not
ListView -- use ListView.view. Dynamic scope is fragile -- use
explicit id refs. Timer.running defaults to false. Connections
targets one object -- use multiple blocks for multiple sources.
