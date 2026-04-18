import QtQuick

/// Translates pinch gestures into camera zoom steps.
/// Attach as a child of the video display area.
PinchArea {
    id: root

    required property var camera
    readonly property bool hasZoom: camera && camera.hasZoom

    enabled: hasZoom

    property int _lastZoomStep: 0

    onPinchStarted: _lastZoomStep = 0

    onPinchUpdated: (pinch) => {
        if (!hasZoom) return

        let z = 0
        if (pinch.scale < 1) {
            z = Math.round(pinch.scale * -10)
        } else {
            z = Math.round(pinch.scale)
        }
        if (z !== _lastZoomStep) {
            _lastZoomStep = z
            camera.stepZoom(z)
        }
    }
}
