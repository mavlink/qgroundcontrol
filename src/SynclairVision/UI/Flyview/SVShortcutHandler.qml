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
    readonly property int actionPreviousCamera: 14
    readonly property int actionAiDetection: 15
    readonly property int actionNextLayout: 16
    readonly property int actionGrid: 17
    readonly property int actionCrosshair: 18
    readonly property int actionSTT: 19
    readonly property int actionCursorTracking: 20
    readonly property int actionManualTracking: 21
    readonly property int actionDeselectTracking: 22
    property bool toolbarVisible: false
    property var flyView

    // --- MODE-INSTÄLLNINGAR FÖR SHORTCUTS ---
    // 0 = Click (Endast vid nedtryckning)
    // 1 = Press (Kör kontinuerligt direkt)
    // 2 = Click + Press (Klicka direkt, vänta delay, sedan kontinuerlig)
    property int inputMode: SVSettings.controlPanelInteraction
    property int holdDelay: 400       // Fördröjning i ms innan upprepning startar (för läge 2)
    property int repeatInterval: 50   // Hastighet i ms mellan varje steg (rekommenderat ~50ms)

    readonly property int joystickRightRole: 0
    readonly property int joystickDownRole: 1
    readonly property int joystickLeftRole: 2
    readonly property int joystickUpRole: 3
    readonly property int zoomInRole: 4
    readonly property int zoomOutRole: 5
    readonly property int smallMovementRole: 6
    property var heldVisualKeys: ({})
    property var heldVisualRoleCounts: [0, 0, 0, 0, 0, 0, 0]
    readonly property bool shortcutInputEligible: root.visible
        && root.Window.window && root.Window.window.active
        && !SVSettings.shortcutCaptureActive && !root.textInputHasFocus()
        && !(Overlay.overlay && Overlay.overlay.visible)
        && !SVState.cursorTrackingSessionActive
    readonly property bool visualShortcutsEligible: root.shortcutInputEligible && root.enabled
        && !SVState.lockControls && SVState.cameraSelected !== -1

    readonly property var actionPolicies: ({
        [root.actionSynclair]: {
            allowWhenShortcutsDisabled: true,
            requiresVisibleToolbar: true,
            requiresSynclairOverlayOff: true
        }
    })

    // --- GEMENSAM AKTIONSTRIGGER ---
    function triggerHeldActions(strength) {
        if (!root.visualShortcutsEligible) return

        const smallMovement = SVState.shortcutSmallMovementHeld

        // Kör rörelse för alla aktiva joystick-riktningar
        for (let direction = root.joystickRightRole; direction <= root.joystickUpRole; ++direction) {
            if (SVState.shortcutJoystickHeld[direction]) {
                SVState.changeEuler(direction, smallMovement, SVSettings.joystickSensitivity * strength)
            }
        }

        // Kör zoom (använd Math.max för att garantera att steget inte avrundas till 0 i backend)
        const zoomStep = Math.max(1, Math.round(SVSettings.zoomSensitivity * strength))
        if (SVState.shortcutZoomInHeld) {
            SVState.changeZoom(zoomStep)
        }
        if (SVState.shortcutZoomOutHeld) {
            SVState.changeZoom(-zoomStep)
        }
    }

    Timer {
        id: holdDelayTimer
        interval: root.holdDelay
        repeat: false
        onTriggered: {
            root.triggerHeldActions(1.0)
            repeatTimer.restart()
        }
    }

    Timer {
        id: repeatTimer
        interval: root.repeatInterval
        repeat: true
        onTriggered: root.triggerHeldActions(1.0)
    }

    function stopShortcutTimers() {
        holdDelayTimer.stop()
        repeatTimer.stop()
    }

    function startShortcutInputModeLogic() {
        const hasHeldMovement = SVState.shortcutJoystickHeld.some(h => h) || 
                                SVState.shortcutZoomInHeld || 
                                SVState.shortcutZoomOutHeld

        if (!hasHeldMovement) {
            stopShortcutTimers()
            return
        }

        // 1. Kör första klicket DIREKT för alla lägen
        root.triggerHeldActions(1.0)

        // 2. Hantera timers beroende på läge
        if (root.inputMode === 0) {
            // Mode 0: Click (Bara klicket ovan, ingen timer)
            stopShortcutTimers()
        } else if (root.inputMode === 1) {
            // Mode 1: Press (Starta kontinuerlig repeat direkt)
            stopShortcutTimers()
            repeatTimer.restart()
        } else if (root.inputMode === 2) {
            // Mode 2: Click + Press (Vänta holdDelay -> starta repeatTimer)
            stopShortcutTimers()
            holdDelayTimer.interval = root.holdDelay
            holdDelayTimer.restart()
        }
    }

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
        SVSettings.shortcutPreviousCamera,
        SVSettings.shortcutDeselectCamera,
        SVSettings.shortcutRecord,
        SVSettings.shortcutPhoto,
        SVSettings.shortcutLockControls,
        SVSettings.shortcutAiDetection,
        SVSettings.shortcutNextLayout,
        SVSettings.shortcutGrid,
        SVSettings.shortcutCrosshair,
        SVSettings.shortcutSTT,
        SVSettings.shortcutCursorTracking,
        SVSettings.shortcutManualTracking,
        SVSettings.shortcutDeselectTracking)

    function buildShortcutRegistry(hud, toolbar, synclair, camera1, camera2, camera3, camera4, camera5,
                                   nextCamera, previousCamera, deselectCamera, record, photo, lockControls,
                                   aiDetection, nextLayout, grid, crosshair,
                                   stt, cursorTracking, manualTracking, deselectTracking) {
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
            [previousCamera, root.actionPreviousCamera],
            [deselectCamera, root.actionDeselectCamera],
            [record, root.actionRecord],
            [photo, root.actionPhoto],
            [lockControls, root.actionLockControls],
            [aiDetection, root.actionAiDetection],
            [nextLayout, root.actionNextLayout],
            [grid, root.actionGrid],
            [crosshair, root.actionCrosshair],
            [stt, root.actionSTT],
            [cursorTracking, root.actionCursorTracking],
            [manualTracking, root.actionManualTracking],
            [deselectTracking, root.actionDeselectTracking]
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

    function visualRolesForKey(key) {
        const roles = []

        if (key !== 0 && key === SVSettings.shortcutJawRight) {
            roles.push(root.joystickRightRole)
        }
        if (key !== 0 && key === SVSettings.shortcutPitchDown) {
            roles.push(root.joystickDownRole)
        }
        if (key !== 0 && key === SVSettings.shortcutJawLeft) {
            roles.push(root.joystickLeftRole)
        }
        if (key !== 0 && key === SVSettings.shortcutPitchUp) {
            roles.push(root.joystickUpRole)
        }
        if (key !== 0 && key === SVSettings.shortcutZoomIn) {
            roles.push(root.zoomInRole)
        }
        if (key !== 0 && key === SVSettings.shortcutZoomOut) {
            roles.push(root.zoomOutRole)
        }
        if (key !== 0 && key === SVSettings.shortcutSmallMovement) {
            roles.push(root.smallMovementRole)
        }

        return roles
    }

    function setVisualRoleHeld(role, held) {
        const nextCount = Math.max(0, root.heldVisualRoleCounts[role] + (held ? 1 : -1))
        root.heldVisualRoleCounts[role] = nextCount

        if (role <= root.joystickUpRole) {
            const directions = SVState.shortcutJoystickHeld.slice()
            directions[role] = nextCount > 0
            SVState.shortcutJoystickHeld = directions
            return
        }

        if (role === root.zoomInRole) {
            SVState.shortcutZoomInHeld = nextCount > 0
        } else if (role === root.zoomOutRole) {
            SVState.shortcutZoomOutHeld = nextCount > 0
        } else {
            SVState.shortcutSmallMovementHeld = nextCount > 0
        }
    }

    function setVisualRolesHeld(roles, held) {
        for (let index = 0; index < roles.length; ++index) {
            root.setVisualRoleHeld(roles[index], held)
        }
    }

    function clearVisualHeldState() {
        root.stopShortcutTimers()
        root.heldVisualKeys = ({})
        root.heldVisualRoleCounts = [0, 0, 0, 0, 0, 0, 0]
        SVState.shortcutJoystickHeld = [false, false, false, false]
        SVState.shortcutZoomInHeld = false
        SVState.shortcutZoomOutHeld = false
        SVState.shortcutSmallMovementHeld = false
    }

    function trackVisualKeyPress(key) {
        if (!root.visualShortcutsEligible) {
            root.clearVisualHeldState()
            return []
        }

        const keyId = key.toString()
        // Ignorera operativsystemets egna tangentupprepningar
        if (root.heldVisualKeys[keyId] !== undefined) {
            return []
        }

        const roles = root.visualRolesForKey(key)
        if (roles.length === 0) {
            return []
        }

        root.heldVisualKeys[keyId] = roles
        root.setVisualRolesHeld(roles, true)

        root.startShortcutInputModeLogic()

        return roles
    }

    function trackVisualKeyRelease(key) {
        const keyId = key.toString()
        const roles = root.heldVisualKeys[keyId]
        if (roles === undefined) {
            return
        }

        delete root.heldVisualKeys[keyId]
        root.setVisualRolesHeld(roles, false)

        const hasHeldMovement = SVState.shortcutJoystickHeld.some(h => h) || 
                                SVState.shortcutZoomInHeld || 
                                SVState.shortcutZoomOutHeld

        if (!hasHeldMovement) {
            root.stopShortcutTimers()
        }
    }

    onVisualShortcutsEligibleChanged: {
        if (!visualShortcutsEligible) {
            clearVisualHeldState()
        }
    }

    Component.onDestruction: clearVisualHeldState()

    function dispatch(shortcut, visualRoles) {
        const action = root.shortcutRegistry[shortcut]
        const policy = root.actionPolicies[action]



        if (!root.shortcutInputEligible) {
            return
        }

        if (policy && policy.requiresSynclairOverlayOff && SVState.synclairOverlay) {
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
        case root.actionPreviousCamera:
            SVState.previousCamera()
            break
        case root.actionAiDetection:
            SVState.toggleAiOverlay()
            break
        case root.actionNextLayout:
            SVState.nextLayout()
            break
        case root.actionGrid:
            SVState.toggleGrid()
            break
        case root.actionCrosshair:
            SVState.toggleCrosshair()
            break
        case root.actionSTT:
            if (root.flyView) {
                root.flyView.submitImmediatePointTracking('singleTarget')
            }
            break
        case root.actionCursorTracking:
            if (root.flyView) {
                root.flyView.submitImmediatePointTracking('cursorTrack')
            }
            break
        case root.actionManualTracking:
            SVState.activateManualTracking()
            break
        case root.actionDeselectTracking:
            SVState.deselectTracking()
            break
        default:
            break
        }
    }

    Connections {
        target: QGroundControl.application

        function onUnacceptedKeyPress(key) {
            root.dispatch(key, root.trackVisualKeyPress(key))
        }

        function onUnacceptedKeyRelease(key) {
            root.trackVisualKeyRelease(key)
        }

        function onUnacceptedMouseRelease(button) {
            root.dispatch(SVSettings.mouseButtonShortcutBase - button)
        }

        function onUnacceptedWheel(angleDeltaY) {
            root.dispatch(angleDeltaY > 0 ? SVSettings.scrollUp : SVSettings.scrollDown)
        }
    }
}
