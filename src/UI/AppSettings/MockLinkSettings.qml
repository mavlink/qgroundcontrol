import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    spacing: _rowSpacing

    readonly property int _MAV_AUTOPILOT_GENERIC:       0
    readonly property int _MAV_AUTOPILOT_PX4:           12
    readonly property int _MAV_AUTOPILOT_ARDUPILOTMEGA: 3
    readonly property int _MAV_TYPE_FIXED_WING:         1
    readonly property int _MAV_TYPE_QUADROTOR:          2

    function saveSettings() {
        switch (firmwareTypeCombo.currentIndex) {
        case 0:
            subEditConfig.firmware = _MAV_AUTOPILOT_PX4
            break
        case 1:
            subEditConfig.firmware = _MAV_AUTOPILOT_ARDUPILOTMEGA
            if (vehicleTypeCombo.currentIndex === 1) {          // Hardcoded _MAV_TYPE_FIXED_WING
                subEditConfig.vehicle = _MAV_TYPE_FIXED_WING
            } else {
                subEditConfig.vehicle = _MAV_TYPE_QUADROTOR
            }
            break
        default:
            subEditConfig.firmware = _MAV_AUTOPILOT_GENERIC
            break
        }
        subEditConfig.sendStatus = sendStatus.checked
        subEditConfig.enableCamera = enableCamera.checked
        subEditConfig.incrementVehicleId = incrementVehicleId.checked
        subEditConfig.cameraCaptureVideo = cameraCaptureVideo.checked
        subEditConfig.cameraCaptureImage = cameraCaptureImage.checked
        subEditConfig.cameraHasModes = cameraHasModes.checked
        subEditConfig.cameraHasVideoStream = cameraHasVideoStream.checked
        subEditConfig.cameraCanCaptureImageInVideoMode = cameraCanCaptureImageInVideoMode.checked
        subEditConfig.cameraCanCaptureVideoInImageMode = cameraCanCaptureVideoInImageMode.checked
        subEditConfig.cameraHasBasicZoom = cameraHasBasicZoom.checked
        subEditConfig.cameraHasTrackingPoint = cameraHasTrackingPoint.checked
        subEditConfig.cameraHasTrackingRectangle = cameraHasTrackingRectangle.checked
    }

    Component.onCompleted: {
        switch (subEditConfig.firmware) {
        case _MAV_AUTOPILOT_PX4:
            firmwareTypeCombo.currentIndex = 0
            break
        case _MAV_AUTOPILOT_ARDUPILOTMEGA:
            firmwareTypeCombo.currentIndex = 1
            break
        default:
            firmwareTypeCombo.currentIndex = 2
            break
        }
        if (subEditConfig.vehicle === _MAV_TYPE_FIXED_WING) {          // Hardcoded _MAV_TYPE_FIXED_WING
            vehicleTypeCombo.currentIndex = 1
        } else {
            vehicleTypeCombo.currentIndex = 0
        }
    }

    QGCCheckBoxSlider {
        id: sendStatus
        Layout.fillWidth: true
        text: qsTr("Send Status Text and Voice")
        checked: subEditConfig.sendStatus
    }

    QGCCheckBoxSlider {
        id: enableCamera
        Layout.fillWidth: true
        text: qsTr("Enable Camera")
        checked: subEditConfig.enableCamera
    }

    QGCCheckBoxSlider {
        id: incrementVehicleId
        Layout.fillWidth: true
        text: qsTr("Increment Vehicle Id")
        checked: subEditConfig.incrementVehicleId
    }

    QGCLabel {
        Layout.fillWidth: true
        text: qsTr("Camera Capabilities")
        font.bold: true
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraCaptureVideo
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_CAPTURE_VIDEO"
        checked: subEditConfig.cameraCaptureVideo
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraCaptureImage
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_CAPTURE_IMAGE"
        checked: subEditConfig.cameraCaptureImage
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraHasModes
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_HAS_MODES"
        checked: subEditConfig.cameraHasModes
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraHasVideoStream
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM"
        checked: subEditConfig.cameraHasVideoStream
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraCanCaptureImageInVideoMode
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_CAN_CAPTURE_IMAGE_IN_VIDEO_MODE"
        checked: subEditConfig.cameraCanCaptureImageInVideoMode
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraCanCaptureVideoInImageMode
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_CAN_CAPTURE_VIDEO_IN_IMAGE_MODE"
        checked: subEditConfig.cameraCanCaptureVideoInImageMode
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraHasBasicZoom
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_HAS_BASIC_ZOOM"
        checked: subEditConfig.cameraHasBasicZoom
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraHasTrackingPoint
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_HAS_TRACKING_POINT"
        checked: subEditConfig.cameraHasTrackingPoint
        visible: enableCamera.checked
    }

    QGCCheckBoxSlider {
        id: cameraHasTrackingRectangle
        Layout.fillWidth: true
        text: "CAMERA_CAP_FLAGS_HAS_TRACKING_RECTANGLE"
        checked: subEditConfig.cameraHasTrackingRectangle
        visible: enableCamera.checked
    }

    LabelledComboBox {
        id: firmwareTypeCombo
        Layout.fillWidth: true
        label: qsTr("Firmware Type")
        model: [ qsTr("PX4 Pro"), qsTr("ArduPilot"), qsTr("Generic MAVLink") ]

        property bool apmFirmwareSelected: currentIndex === 1
    }

    LabelledComboBox {
        id:                     vehicleTypeCombo
        Layout.fillWidth:  true
        label:                  qsTr("Vehicle Type")
        model:                  [ qsTr("ArduCopter"), qsTr("ArduPlane") ]
        visible:                firmwareTypeCombo.apmFirmwareSelected
    }
}
