import QtQuick

// Steps the camera's zoom as two fingers spread or pinch: one step per zoomStepScale of pinch, the same in and out.
//
// It takes the touches that reach it, and so none that another item has. Under a mouse area, which has the first
// finger as its press, that is the second finger alone: the mouse area tells where its finger is (heldFinger). A
// PinchArea can't be told. As the mouse area's parent it would see that finger, but would also take the touches of
// the items over the mouse area, such as the virtual joysticks'.
Item {
    id: root

    /// The camera to zoom; any object with hasZoom and stepZoom(direction), which takes -1 (wide) or 1 (tele), will do.
    property var camera: null
    /// Pinch scale per zoom step.
    property real zoomStepScale: 1.25
    /// Where a finger is that is down as the press of a mouse area over this item, in this item's coordinates.
    property point heldFinger
    /// Whether that finger is down.
    property bool heldFingerDown: false

    /// Two fingers are down: the held one and one here, or two here.
    readonly property bool pinching: root.heldFingerDown ? touch1.pressed : (touch1.pressed && touch2.pressed)

    readonly property bool _hasZoom: !!root.camera && root.camera.hasZoom
    // How far apart the two fingers were when the pinch began, and whether the held finger was one of them. 0 without a
    // pinch.
    property real _startSpread: 0
    property bool _startHeld: false
    // Zoom steps sent during the current pinch.
    property int _sent: 0

    /// Sends the camera one zoom step toward the step count @p scale stands for, given the @p sent steps of this pinch
    /// so far; returns the count sent after it. A step a pinch update skips goes with the next one.
    function _stepToward(scale, sent) {
        const target = Math.round(Math.log(scale) / Math.log(root.zoomStepScale))
        if (!root._hasZoom || target === sent) {
            return sent
        }
        const direction = target > sent ? 1 : -1
        root.camera.stepZoom(direction)
        return sent + direction
    }

    /// Follows the two fingers, after either moved or was put down or lifted.
    function _followFingers() {
        if (!root.pinching) {
            root._startSpread = 0
            return
        }
        const first = root.heldFingerDown ? root.heldFinger : Qt.point(touch1.x, touch1.y)
        const second = root.heldFingerDown ? Qt.point(touch1.x, touch1.y) : Qt.point(touch2.x, touch2.y)
        const spread = Math.hypot(second.x - first.x, second.y - first.y)
        if (root._startSpread <= 0 || root._startHeld !== root.heldFingerDown) {
            root._startSpread = spread
            root._startHeld = root.heldFingerDown
            root._sent = 0
        } else {
            root._sent = root._stepToward(spread / root._startSpread, root._sent)
        }
    }

    onPinchingChanged: root._followFingers()
    onHeldFingerChanged: root._followFingers()

    // The platform's own pinch gesture, as a trackpad's. Touches stop at the touch area over it.
    PinchArea {
        anchors.fill:   parent
        enabled:        root._hasZoom

        onPinchStarted: root._sent = 0
        onPinchUpdated: (pinch) => { root._sent = root._stepToward(pinch.scale, root._sent) }
    }

    MultiPointTouchArea {
        anchors.fill:       parent
        enabled:            root._hasZoom
        mouseEnabled:       false
        maximumTouchPoints: 2
        touchPoints: [
            TouchPoint { id: touch1 },
            TouchPoint { id: touch2 }
        ]

        onTouchUpdated: root._followFingers()
    }
}
