pragma Singleton
import QtQuick
import QGroundControl

QtObject {
    id: root

    readonly property ListModel model: ListModel {}
    property int _nextId: 0
    readonly property int _defaultDuration: 5000

    function add(title, message, severity, type) {
        severity = severity || "info"
        type     = type || "info"

        var id = _nextId++
        model.insert(0, {
            toastId:          id,
            toastTitle:       title,
            toastMessage:     message,
            toastSeverity:    severity,
            toastType:        type,
            toastDismissing:  false
        })

        var timer = _timerComponent.createObject(root, { toastId: id, interval: _defaultDuration })
        timer.start()
    }

    // Trigger soft dismissal animation
    function dismiss(id) {
        for (var i = 0; i < model.count; i++) {
            if (model.get(i).toastId === id) {
                model.setProperty(i, "toastDismissing", true)
                return
            }
        }
    }

    // Actual model deletion after animation completes
    function _remove(id) {
        for (var i = 0; i < model.count; i++) {
            if (model.get(i).toastId === id) {
                model.remove(i)
                return
            }
        }
    }

    property Component _timerComponent: Component {
        Timer {
            property int toastId
            repeat: false
            onTriggered: {
                root.dismiss(toastId)
                destroy()
            }
        }
    }
}