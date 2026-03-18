import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Camera section for mission item editors
Column {
    required property var missionItem

    property alias buttonGroup: cameraSectionHeader.buttonGroup
    property alias showSpacer:  cameraSectionHeader.showSpacer
    property alias checked:     cameraSectionHeader.checked
    property bool showSectionHeader: true

    spacing: _margin

    property var    _camera:        missionItem ? missionItem.cameraSection : null
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
            fact:       _camera ? _camera.cameraAction : null
            indexModel: false
        }

        LabelledFactTextField {
            width:      parent.width
            label:      qsTr("Time")
            fact:       _camera ? _camera.cameraPhotoIntervalTime : null
            visible:    _camera ? _camera.cameraAction.rawValue === 1 : false
        }

        LabelledFactTextField {
            width:      parent.width
            label:      qsTr("Distance")
            fact:       _camera ? _camera.cameraPhotoIntervalDistance : null
            visible:    _camera ? _camera.cameraAction.rawValue === 2 : false
        }

        RowLayout {
            width:      parent.width
            spacing:    ScreenTools.defaultFontPixelWidth
            visible:    _camera ? _camera.cameraModeSupported : false

            QGCCheckBox {
                id:                 modeCheckBox
                text:               qsTr("Mode")
                checked:            _camera ? _camera.specifyCameraMode : false
                onClicked:          { if (_camera) _camera.specifyCameraMode = checked }
            }

            FactComboBox {
                fact:               _camera ? _camera.cameraMode : null
                indexModel:         false
                enabled:            modeCheckBox.checked
                Layout.fillWidth:   true
            }
        }

        QGCCheckBox {
            id:                 gimbalCheckBox
            text:               qsTr("Gimbal")
            checked:            _camera ? _camera.specifyGimbal : false
            onClicked:          { if (_camera) _camera.specifyGimbal = checked }
        }

        FactTextFieldSlider {
            width:          parent.width
            label:          qsTr("Pitch")
            fact:           _camera ? _camera.gimbalPitch : null
            enabled:        gimbalCheckBox.checked
        }

        FactTextFieldSlider {
            width:          parent.width
            label:          qsTr("Yaw")
            fact:           _camera ? _camera.gimbalYaw : null
            enabled:        gimbalCheckBox.checked
        }
    }
}
