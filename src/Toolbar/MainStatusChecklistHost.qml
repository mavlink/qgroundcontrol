import QtQuick
import QtQuick.Layouts

/// Shows the single checklist instance owned by a Loader. The checklist keeps its progress, so it is handed back to
/// the loader when this host is destroyed.
Item {
    id: control

    required property Loader loader

    Layout.fillWidth:   true
    implicitHeight:     loader.item ? loader.item.implicitHeight : 0

    function adoptChecklist() {
        if (loader.item) {
            loader.item.parent = control
        }
    }

    Connections {
        target: control.loader
        function onItemChanged() { control.adoptChecklist() }
    }

    Binding {
        target:         control.loader.item
        property:       "width"
        value:          control.width
        when:           !!control.loader.item
        restoreMode:    Binding.RestoreNone
    }

    Component.onCompleted: adoptChecklist()

    Component.onDestruction: {
        if (loader.item) {
            loader.item.parent = loader
        }
    }
}
