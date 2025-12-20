import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Camera section for mission item editors
Column {
    property alias buttonGroup: cameraSectionHeader.buttonGroup
    property alias showSpacer:  cameraSectionHeader.showSpacer
    property alias checked:     cameraSectionHeader.checked
    property bool showSectionHeader: true

    spacing: _margin

    property var    _camera:        missionItem.cameraSection
    property real   _fieldWidth:    ScreenTools.defaultFontPixelWidth * 16
    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2

    SectionHeader {
        id:             cameraSectionHeader
        width:          parent.width
        text:           qsTr("Camera")
        checked:        false
        visible:        showSectionHeader
    }

    Column {
        width:      parent.width
        spacing:    _margin
        visible:    !showSectionHeader || cameraSectionHeader.checked

        LabelledFactComboBox {
            id:         cameraActionCombo
            width:      parent.width
            label:      qsTr("Action")
            fact:       _camera.cameraAction
            indexModel: false
        }

        LabelledFactTextField {
            width:      parent.width
            label:      qsTr("Time")
            fact:       _camera.cameraPhotoIntervalTime
            visible:    _camera.cameraAction.rawValue === 1
        }

        LabelledFactTextField {
            width:      parent.width
            label:      qsTr("Distance")
            fact:       _camera.cameraPhotoIntervalDistance
            visible:    _camera.cameraAction.rawValue === 2
        }

        RowLayout {
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelWidth
            visible:    _camera.cameraModeSupported

            QGCCheckBox {
                id:                 modeCheckBox
                text:               qsTr("Mode")
                checked:            _camera.specifyCameraMode
                onClicked:          _camera.specifyCameraMode = checked
            }

            FactComboBox {
                fact:               _camera.cameraMode
                indexModel:         false
                enabled:            modeCheckBox.checked
                Layout.fillWidth:   true
            }
        }

        QGCCheckBox {
            id:                 gimbalCheckBox
            text:               qsTr("Gimbal")
            checked:            _camera.specifyGimbal
            onClicked:          _camera.specifyGimbal = checked
        }

        FactTextFieldSlider {
            width:          parent.width
            label:          qsTr("Pitch")
            fact:           _camera.gimbalPitch
            enabled:        gimbalCheckBox.checked
        }

        FactTextFieldSlider {
            width:          parent.width
            label:          qsTr("Yaw")
            fact:           _camera.gimbalYaw
            enabled:        gimbalCheckBox.checked
        }
    }
}
