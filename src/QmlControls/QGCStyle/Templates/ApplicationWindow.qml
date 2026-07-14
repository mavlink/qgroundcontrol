import Qt.labs.StyleKit as Labs
import QtQuick
import QtQuick.Templates as T

T.ApplicationWindow {
    id: root

    Labs.StyleKit.style: qgcStyle
    Labs.StyleKit.transitionsEnabled: StylePreferences.animationsEnabled
    color: palette.window
    font: StyleTypography.bodyFont

    PointHandler {
        id: touchInputObserver

        target: null
        acceptedDevices: PointerDevice.TouchScreen

        onActiveChanged: {
            if (touchInputObserver.active) {
                InputCapabilities.refresh()
                StylePreferences.noteTouchInput()
            }
        }
    }

    QGCStyleKit {
        id: qgcStyle
    }
}
