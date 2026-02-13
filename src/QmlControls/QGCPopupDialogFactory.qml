import QtQuick
import QtQml

/// Factory for creating and opening QGCPopupDialog instances from a Component.
/// Place the factory immediately before its associated Component. The dialog is created
/// with the factory's parent as the initial parent, then QGCPopupDialog reparents itself
/// to Overlay.overlay on completion.
///
/// Example:
///     QGCPopupDialogFactory {
///         id: myDialogFactory
///         dialogComponent: myDialogComponent
///     }
///
///     Component {
///         id: myDialogComponent
///         QGCPopupDialog { ... }
///     }
///
///     onFoo: myDialogFactory.open()
///     onBar: myDialogFactory.open({ title: "My Title", myProp: someValue })
Item {
    id: root
    visible: false

    required property Component dialogComponent

    function open(props) {
        var dialogParent = root.parent ? root.parent : null
        var dialog = props === undefined || props === null
            ? dialogComponent.createObject(dialogParent)
            : dialogComponent.createObject(dialogParent, props)
        dialog.open()
        return dialog
    }
}
