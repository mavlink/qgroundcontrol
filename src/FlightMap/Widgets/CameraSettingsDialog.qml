import QtQuick
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Extracted from PhotoVideoControl — camera and video settings dialog.
// Uses ColumnLayout of paired RowLayouts instead of the fragile
// GridLayout.TopToBottom + dynamicRows counter pattern.
QGCPopupDialog {
    id: root

    title:   qsTr("Settings")
    buttons: Dialog.Close

    required property var  camera
    required property var  cameraManager
    required property real contentSpacing

    property bool _multipleMavlinkCameras:       cameraManager.cameras.count > 1
    property bool _multipleMavlinkCameraStreams:  camera.streamLabels.length > 1
    property bool _cameraStorageSupported:        camera.storageStatus !== MavlinkCameraControlInterface.STORAGE_NOT_SUPPORTED
    property var  _videoSettings:                 QGroundControl.settingsManager.videoSettings

    ColumnLayout {
        spacing: root.contentSpacing

        // ── Camera selector ──────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: _multipleMavlinkCameras
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Camera"); Layout.fillWidth: true }
            QGCComboBox {
                Layout.fillWidth: true
                sizeToContents: true
                model: root.cameraManager.cameraLabels
                currentIndex: root.cameraManager.currentCamera
                onActivated: (index) => { root.cameraManager.currentCamera = index }
            }
        }

        // ── Video stream selector ────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: _multipleMavlinkCameraStreams
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Video Stream"); Layout.fillWidth: true }
            QGCComboBox {
                Layout.fillWidth: true
                sizeToContents: true
                model: root.camera.streamLabels
                currentIndex: root.camera.currentStream
                onActivated: (index) => { root.camera.currentStream = index }
            }
        }

        // ── Thermal view mode ────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.thermalStreamInstance
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Thermal View Mode"); Layout.fillWidth: true }
            QGCComboBox {
                Layout.fillWidth: true
                sizeToContents: true
                model: [ qsTr("Off"), qsTr("Blend"), qsTr("Full"), qsTr("Picture In Picture") ]
                currentIndex: root.camera.thermalMode
                onActivated: (index) => { root.camera.thermalMode = index }
            }
        }

        // ── Blend opacity ────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.thermalStreamInstance && root.camera.thermalMode === MavlinkCameraControlInterface.THERMAL_BLEND
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Blend Opacity"); Layout.fillWidth: true }
            QGCSlider {
                Layout.fillWidth: true
                to: 100; from: 0
                value: root.camera.thermalOpacity
                live: true
                onValueChanged: root.camera.thermalOpacity = value
            }
        }

        // ── MAVLink camera active settings ───────────────────────
        Repeater {
            model: root.camera.activeSettings

            RowLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelWidth

                required property string modelData
                property var  _fact:     root.camera.getFact(modelData)
                property bool _isBool:   _fact.typeIsBool
                property bool _isCombo:  !_isBool && _fact.enumStrings.length > 0
                property bool _isSlider: _fact && !isNaN(_fact.increment)
                property bool _isEdit:   !_isBool && !_isSlider && _fact.enumStrings.length < 1

                QGCLabel { text: _fact.shortDescription; Layout.fillWidth: true }

                FactComboBox {
                    Layout.fillWidth: true
                    sizeToContents: true
                    fact: parent._fact
                    indexModel: false
                    visible: parent._isCombo
                }
                FactTextField {
                    Layout.fillWidth: true
                    fact: parent._fact
                    visible: parent._isEdit
                }
                QGCSlider {
                    Layout.fillWidth: true
                    to: parent._fact.max
                    from: parent._fact.min
                    stepSize: parent._fact.increment
                    visible: parent._isSlider
                    live: false
                    property bool initialized: false
                    onValueChanged: {
                        if (!initialized) return
                        parent._fact.value = value
                    }
                    Component.onCompleted: {
                        value = parent._fact.value
                        initialized = true
                    }
                }
                QGCCheckBoxSlider {
                    checked: parent._fact ? parent._fact.value : false
                    visible: parent._isBool
                    onClicked: parent._fact.value = checked ? 1 : 0
                }
            }
        }

        // ── Photo mode ───────────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.capturesPhotos
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Photo Mode"); Layout.fillWidth: true }
            QGCComboBox {
                Layout.fillWidth: true
                sizeToContents: true
                model: [ qsTr("Single"), qsTr("Time Lapse") ]
                currentIndex: root.camera.photoCaptureMode
                onActivated: (index) => { root.camera.photoCaptureMode = index }
            }
        }

        // ── Photo interval ───────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.capturesPhotos && root.camera.photoCaptureMode === MavlinkCameraControlInterface.PHOTO_CAPTURE_TIMELAPSE
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Photo Interval (seconds)"); Layout.fillWidth: true }
            QGCSlider {
                Layout.fillWidth: true
                to: 60; from: 1; stepSize: 1
                value: root.camera.photoLapse
                displayValue: true; live: true
                onValueChanged: root.camera.photoLapse = value
            }
        }

        // ── Video grid lines ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.hasVideoStream
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Video Grid Lines"); Layout.fillWidth: true }
            QGCCheckBoxSlider {
                checked: _videoSettings.gridLines.rawValue
                onClicked: _videoSettings.gridLines.rawValue = checked ? 1 : 0
            }
        }

        // ── Video screen fit ─────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: root.camera.hasVideoStream
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Video Screen Fit"); Layout.fillWidth: true }
            FactComboBox {
                Layout.fillWidth: true
                sizeToContents: true
                fact: _videoSettings.videoFit
                indexModel: false
            }
        }

        // ── Reset camera defaults ────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Reset Camera Defaults"); Layout.fillWidth: true }
            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Reset")
                onClicked: resetPrompt.open()
                MessageDialog {
                    id: resetPrompt
                    title: qsTr("Reset Camera to Factory Settings")
                    text: qsTr("Confirm resetting all settings?")
                    buttons: MessageDialog.Yes | MessageDialog.No
                    onButtonClicked: function (button, role) {
                        if (button === MessageDialog.Yes) {
                            root.camera.resetSettings()
                        }
                        resetPrompt.close()
                    }
                }
            }
        }

        // ── Format storage ───────────────────────────────────────
        RowLayout {
            Layout.fillWidth: true
            visible: _cameraStorageSupported
            spacing: ScreenTools.defaultFontPixelWidth
            QGCLabel { text: qsTr("Storage"); Layout.fillWidth: true }
            QGCButton {
                Layout.fillWidth: true
                text: qsTr("Format")
                onClicked: formatPrompt.open()
                MessageDialog {
                    id: formatPrompt
                    title: qsTr("Format Camera Storage")
                    text: qsTr("Confirm erasing all files?")
                    buttons: MessageDialog.Yes | MessageDialog.No
                    onButtonClicked: function (button, role) {
                        if (button === MessageDialog.Yes) {
                            root.camera.formatCard()
                        }
                        formatPrompt.close()
                    }
                }
            }
        }
    }
}
