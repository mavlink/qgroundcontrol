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
        subEditConfig.enableGimbal = enableGimbal.checked
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
        subEditConfig.gimbalHasRollAxis = gimbalHasRollAxis.checked
        subEditConfig.gimbalHasPitchAxis = gimbalHasPitchAxis.checked
        subEditConfig.gimbalHasYawAxis = gimbalHasYawAxis.checked
        subEditConfig.gimbalHasYawFollow = gimbalHasYawFollow.checked
        subEditConfig.gimbalHasYawLock = gimbalHasYawLock.checked
        subEditConfig.gimbalHasRetract = gimbalHasRetract.checked
        subEditConfig.gimbalHasNeutral = gimbalHasNeutral.checked
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
        id: enableGimbal
        Layout.fillWidth: true
        text: qsTr("Enable Gimbal")
        checked: subEditConfig.enableGimbal
    }

    QGCCheckBoxSlider {
        id: incrementVehicleId
        Layout.fillWidth: true
        text: qsTr("Increment Vehicle Id")
        checked: subEditConfig.incrementVehicleId
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

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("Camera Capabilities")
        visible: enableCamera.checked

        QGCCheckBoxSlider {
            id: cameraHasVideoStream
            Layout.fillWidth: true
            text: "CAMERA_CAP_FLAGS_HAS_VIDEO_STREAM"
            checked: subEditConfig.cameraHasVideoStream
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
    }

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading: qsTr("Gimbal Capabilities")
        visible: enableGimbal.checked

        QGCCheckBoxSlider {
            id: gimbalHasRollAxis
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_ROLL_AXIS"
            checked: subEditConfig.gimbalHasRollAxis
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasPitchAxis
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_PITCH_AXIS"
            checked: subEditConfig.gimbalHasPitchAxis
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasYawAxis
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_AXIS"
            checked: subEditConfig.gimbalHasYawAxis
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasYawFollow
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_FOLLOW"
            checked: subEditConfig.gimbalHasYawFollow
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasYawLock
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_YAW_LOCK"
            checked: subEditConfig.gimbalHasYawLock
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasRetract
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_RETRACT"
            checked: subEditConfig.gimbalHasRetract
            visible: enableGimbal.checked
        }

        QGCCheckBoxSlider {
            id: gimbalHasNeutral
            Layout.fillWidth: true
            text: "GIMBAL_MANAGER_CAP_FLAGS_HAS_NEUTRAL"
            checked: subEditConfig.gimbalHasNeutral
            visible: enableGimbal.checked
        }
    }
}
