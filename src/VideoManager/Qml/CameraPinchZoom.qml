import QtQuick

// Steps the camera's zoom as two fingers spread or pinch: one step per zoomStepScale of pinch, the same in and out.
PinchArea {
    id: root

    /// The camera to zoom; any object with hasZoom and stepZoom(direction), which takes -1 (wide) or 1 (tele), will do.
    property var camera: null
    /// Pinch scale per zoom step.
    property real zoomStepScale: 1.25

    // Zoom steps sent during the current pinch.
    property int _sent: 0
    readonly property bool _hasZoom: !!root.camera && root.camera.hasZoom

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

    enabled: root._hasZoom

    onPinchStarted: root._sent = 0
    onPinchUpdated: (pinch) => { root._sent = root._stepToward(pinch.scale, root._sent) }
}
