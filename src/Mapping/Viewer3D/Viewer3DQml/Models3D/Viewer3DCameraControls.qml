import QtQuick

import QGroundControl.Viewer3D

/// Gazebo-style input handling for the 3D viewer. Forwards mouse/touch gestures
/// to a Viewer3DCameraController:
///   - Left drag: pan (picked ground point stays under the cursor)
///   - Middle drag or Shift+left drag: orbit around the point under the cursor
///   - Right drag: zoom toward/away from the pressed point (up = in, down = out)
///   - Wheel: zoom toward/away from the point under the cursor
///   - Pinch: zoom toward the gesture centroid
///   - Two-finger twist: rotate heading around the point under the centroid
///   - Two-finger vertical swipe: tilt around the point under the centroid
Item {
    id: root

    property Viewer3DCameraController controller

    DragHandler {
        id: panHandler
        target: null
        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.NoModifier
        onActiveChanged: {
            if (active) {
                root.controller.beginPan(centroid.pressPosition)
                // Event compression can collapse the whole drag into the
                // activating event with no centroid change afterwards, so
                // apply the movement accumulated so far right away
                root.controller.panTo(centroid.position)
            }
        }
        onCentroidChanged: {
            if (active) {
                root.controller.panTo(centroid.position)
            }
        }
    }

    DragHandler {
        id: orbitHandler
        target: null
        acceptedButtons: Qt.MiddleButton
        // acceptedButtons does not filter touch points (they have no
        // buttons), so keep touch away from the button-driven handlers
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        onActiveChanged: {
            if (active) {
                root.controller.beginOrbit(centroid.pressPosition)
                root.controller.orbitTo(centroid.position)
            }
        }
        onCentroidChanged: {
            if (active) {
                root.controller.orbitTo(centroid.position)
            }
        }
    }

    // Gazebo-style right-drag zoom: drag up zooms in, down zooms out
    DragHandler {
        id: dragZoomHandler
        target: null
        acceptedButtons: Qt.RightButton
        acceptedDevices: PointerDevice.Mouse | PointerDevice.TouchPad
        property real _lastY: 0
        onActiveChanged: {
            if (active) {
                _lastY = centroid.pressPosition.y
                root.controller.zoom((_lastY - centroid.position.y) * 3, centroid.pressPosition)
                _lastY = centroid.position.y
            }
        }
        onCentroidChanged: {
            if (active) {
                root.controller.zoom((_lastY - centroid.position.y) * 3, centroid.pressPosition)
                _lastY = centroid.position.y
            }
        }
    }

    // Trackpad-friendly orbit: macOS trackpads have no easy right-drag
    DragHandler {
        id: shiftOrbitHandler
        target: null
        acceptedButtons: Qt.LeftButton
        acceptedModifiers: Qt.ShiftModifier
        onActiveChanged: {
            if (active) {
                root.controller.beginOrbit(centroid.pressPosition)
                root.controller.orbitTo(centroid.position)
            }
        }
        onCentroidChanged: {
            if (active) {
                root.controller.orbitTo(centroid.position)
            }
        }
    }

    WheelHandler {
        id: zoomHandler
        target: null
        onWheel: (event) => {
            root.controller.zoom(event.angleDelta.y, Qt.point(event.x, event.y))
        }
    }

    // The PinchHandler owns all two-finger gestures: scale zooms, twist
    // rotates, and vertical centroid movement tilts (Google Earth style:
    // swipe up tilts toward the horizon, gain full height = 180 deg). A
    // separate two-finger DragHandler for tilt would race the PinchHandler
    // for the exclusive grab, randomly making tilt swipes unresponsive.
    PinchHandler {
        id: pinchHandler
        target: null
        property real _lastScale: 1.0
        property real _lastRotation: 0.0
        property real _lastY: 0
        onActiveChanged: {
            _lastScale = 1.0
            _lastRotation = 0.0
            if (active) {
                // Apply the movement consumed before activation right away:
                // touch-move compression can leave no centroid change between
                // activation and release, so waiting for onCentroidChanged
                // would drop the whole swipe
                _lastY = centroid.pressPosition.y
                root.controller.tiltBy(((_lastY - centroid.position.y) / root.height) * 180, centroid.position)
                _lastY = centroid.position.y
            }
        }
        onCentroidChanged: {
            if (active) {
                root.controller.tiltBy(((_lastY - centroid.position.y) / root.height) * 180, centroid.position)
                _lastY = centroid.position.y
            }
        }
        onActiveScaleChanged: {
            if (active && (activeScale > 0)) {
                root.controller.zoomBy(_lastScale / activeScale, centroid.position)
                _lastScale = activeScale
            }
        }
        // Two-finger twist: the world follows the fingers. activeRotation is
        // positive for a visually clockwise twist (y-down screen), which
        // matches a heading increase, so it feeds through directly.
        onActiveRotationChanged: {
            if (active) {
                root.controller.rotateBy(activeRotation - _lastRotation, centroid.position)
                _lastRotation = activeRotation
            }
        }
    }
}
