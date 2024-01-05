import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette

// Camera section for mission item editors
ColumnLayout {
    Layout.fillWidth:   true
    spacing:            _horizontalSpacing

    property var cameraSection  ///< CameraSection object

    property real _horizontalSpacing:   ScreenTools.defaultFontPixelWidth
    property real _verticalSpacing:     ScreenTools.defaultFontPixelHeight / 2
    property real _maxTextWidth:    ScreenTools.defaultFontPixelWidth * 40

    ColumnLayout {
        Layout.fillWidth:   true
        spacing:            _verticalSpacing

        RowLayout {
            Layout.fillWidth:   true
            spacing:            _horizontalSpacing
            visible:            cameraSection.cameraModeSupported

            QGCCheckBoxSlider {
                id:                 modeCheckBox
                checked:            cameraSection.specifyCameraMode
                onClicked:          cameraSection.specifyCameraMode = checked
            }
            QGCLabel {
                Layout.fillWidth:   true
                text:               qsTr("Camera mode")
                enabled:            modeCheckBox.checked
            }
            FactComboBox {
                Layout.alignment:   Qt.AlignRight
                fact:               cameraSection.cameraMode
                indexModel:         false
                enabled:            modeCheckBox.checked
            }
        }

        LabelledFactComboBox {
            label:          qsTr("Camera command")
            fact:           cameraSection.cameraAction
            indexModel:     false
        }

        LabelledFactTextField {
            label:      qsTr("Time interval")
            fact:       cameraSection.cameraPhotoIntervalTime
            visible:    cameraSection.cameraAction.rawValue === 1
        }

        LabelledFactTextField {
            label:      qsTr("Distance interval")
            fact:       cameraSection.cameraPhotoIntervalDistance
            visible:    cameraSection.cameraAction.rawValue === 2
        }

        RowLayout {
            Layout.fillWidth:   true
            spacing:            _horizontalSpacing

            QGCCheckBoxSlider {
                id:         gimbalCheckBox
                Layout.alignment:   Qt.AlignVCenter
                checked:    cameraSection.specifyGimbal
                onClicked:  cameraSection.specifyGimbal = checked
            }
            QGCLabel { 
                Layout.alignment:   Qt.AlignVCenter
                text:       qsTr("Gimbal") 
                enabled:    gimbalCheckBox.checked
            }

            ColumnLayout {
                spacing: 0
                enabled: gimbalCheckBox.checked

                QGCLabel { text: qsTr("Pitch") }
                FactTextField {
                    fact:           cameraSection.gimbalPitch
                    implicitWidth:  ScreenTools.defaultFontPixelWidth * 9
                }
            }

            ColumnLayout {
                spacing: 0
                enabled: gimbalCheckBox.checked
    
                QGCLabel { text: qsTr("Yaw") }
                FactTextField {
                    fact:           cameraSection.gimbalYaw
                    implicitWidth:  ScreenTools.defaultFontPixelWidth * 9
                }
            }
        }
    }
}
