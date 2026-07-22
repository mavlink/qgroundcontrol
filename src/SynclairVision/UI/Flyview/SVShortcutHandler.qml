import QtQuick
import QtQuick.Controls
import QtQuick.Window

import QGroundControl

Item {
    id: root

    readonly property int actionHUD: 1
    readonly property int actionToolbar: 2
    readonly property int actionSynclair: 3
    readonly property int actionCamera1: 4
    readonly property int actionCamera2: 5
    readonly property int actionCamera3: 6
    readonly property int actionCamera4: 7
    readonly property int actionCamera5: 8
    readonly property int actionNextCamera: 9
    readonly property int actionDeselectCamera: 10
    readonly property int actionRecord: 11
    readonly property int actionPhoto: 12
    readonly property int actionLockControls: 13
    property bool toolbarVisible: false

    // Actions not listed here are disabled when shortcutsEnabled is false.
    readonly property var actionPolicies: ({
        [root.actionSynclair]: {
            allowWhenShortcutsDisabled: true,
            requiresVisibleToolbar: true
        }
    })

    // Keep the case order from the legacy handler: the first action wins duplicate bindings.
    readonly property var shortcutRegistry: buildShortcutRegistry(
        SVSettings.shortcutHUD,
        SVSettings.shortcutToolbar,
        SVSettings.shortcutSynclair,
        SVSettings.shortcutCamera1,
        SVSettings.shortcutCamera2,
        SVSettings.shortcutCamera3,
        SVSettings.shortcutCamera4,
        SVSettings.shortcutCamera5,
        SVSettings.shortcutNextCamera,
        SVSettings.shortcutDeselectCamera,
        SVSettings.shortcutRecord,
        SVSettings.shortcutPhoto,
        SVSettings.shortcutLockControls)

    function buildShortcutRegistry(hud, toolbar, synclair, camera1, camera2, camera3, camera4, camera5,
                                   nextCamera, deselectCamera, record, photo, lockControls) {
        const registry = {}
        const bindings = [
            [hud, root.actionHUD],
            [toolbar, root.actionToolbar],
            [synclair, root.actionSynclair],
            [camera1, root.actionCamera1],
            [camera2, root.actionCamera2],
            [camera3, root.actionCamera3],
            [camera4, root.actionCamera4],
            [camera5, root.actionCamera5],
            [nextCamera, root.actionNextCamera],
            [deselectCamera, root.actionDeselectCamera],
            [record, root.actionRecord],
            [photo, root.actionPhoto],
            [lockControls, root.actionLockControls]
        ]

        for (let index = 0; index < bindings.length; ++index) {
            const shortcut = bindings[index][0]
            if (shortcut !== 0 && registry[shortcut] === undefined) {
                registry[shortcut] = bindings[index][1]
            }
        }

        return registry
    }

    function textInputHasFocus() {
        const focusItem = root.Window.window ? root.Window.window.activeFocusItem : null
        return focusItem instanceof TextInput || focusItem instanceof TextEdit
    }

    function dispatch(shortcut) {
        const action = root.shortcutRegistry[shortcut]
        const policy = root.actionPolicies[action]
        if (!root.visible || !root.Window.window || !root.Window.window.active
                || SVSettings.shortcutCaptureActive || root.textInputHasFocus()
                || (Overlay.overlay && Overlay.overlay.visible)
                || SVState.cursorTrackingSessionActive) {
            return
        }

        if (!SVState.shortcutsEnabled
                && (!policy || !policy.allowWhenShortcutsDisabled
                    || (policy.requiresVisibleToolbar && !root.toolbarVisible))) {
            return
        }

        switch (action) {
        case root.actionHUD:
            SVState.toggleHud()
            break
        case root.actionToolbar:
            SVState.toggleToolbar()
            break
        case root.actionSynclair:
            SVState.toggleSynclairOverlay()
            break
        case root.actionCamera1:
            SVState.setCamera(0)
            break
        case root.actionCamera2:
            SVState.setCamera(1)
            break
        case root.actionCamera3:
            SVState.setCamera(2)
            break
        case root.actionCamera4:
            SVState.setCamera(3)
            break
        case root.actionCamera5:
            SVState.setCamera(4)
            break
        case root.actionNextCamera:
            SVState.nextCamera()
            break
        case root.actionDeselectCamera:
            SVState.clearCamera()
            break
        case root.actionRecord:
            SVState.toggleRecord()
            break
        case root.actionPhoto:
            SVState.takePhoto()
            break
        case root.actionLockControls:
            SVState.toggleLockControls()
            break
        default:
            break
        }
    }

    Connections {
        target: QGroundControl.application

        function onUnacceptedKeyPress(key) {
            root.dispatch(key)
        }

        function onUnacceptedMouseRelease(button) {
            root.dispatch(SVSettings.mouseButtonShortcutBase - button)
        }

        function onUnacceptedWheel(angleDeltaY) {
            root.dispatch(angleDeltaY > 0 ? SVSettings.scrollUp : SVSettings.scrollDown)
        }
    }
}
