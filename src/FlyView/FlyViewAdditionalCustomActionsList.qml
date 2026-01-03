import QtQuick

// Custom builds can override this resource to add additional custom actions
Item {
    visible: false

    property var guidedController

    property bool anyActionAvailable: {
        for (var i = 0; i < model.length; i++) {
            if (model[i].visible)
                return true
        }

        return false
    }

    property var model: [ ]
}
