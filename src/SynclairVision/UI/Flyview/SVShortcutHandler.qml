import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl

Item {
    id: root

    focus: true
    Component.onCompleted: forceActiveFocus()

    Keys.onPressed: (event) => {
        if (event.key === Qt.Key_Tab || event.key === Qt.Key_Backtab) {
            event.accepted = true
            return
        }
    }


    Keys.onReleased: (event) => {
    if (event.isAutoRepeat) {
        event.accepted = true
        return
    }

    

    switch (event.key) {
        case SVSettings.shortcutHUD:
            SVState.toggleHud()
            break

        case SVSettings.shortcutToolbar:
            SVState.toggleToolbar()
            break

        case SVSettings.shortcutSynclair:
            SVState.toggleSynclairOverlay()
            break

        case SVSettings.shortcutCamera1:
            SVState.setCamera(0)
            break

        case SVSettings.shortcutCamera2:
            SVState.setCamera(1)
            break

        case SVSettings.shortcutCamera3:
            SVState.setCamera(2)
            break

        case SVSettings.shortcutCamera4:
            SVState.setCamera(3)
            break

        case SVSettings.shortcutCamera5:
            SVState.setCamera(4)
            break

        case SVSettings.shortcutNextCamera:
            SVState.nextCamera()
            break

        case SVSettings.shortcutDeselectCamera:
            SVState.clearCamera()
            break

        case SVSettings.shortcutRecord:
            SVState.toggleRecord()
            break

        case SVSettings.shortcutLockControls:
            SVState.toggleLockControls()
            break

        default:
            break
        }

        event.accepted = true

    }
}