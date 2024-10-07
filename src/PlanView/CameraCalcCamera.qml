import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette

// Camera calculator "Camera" section for mission item editors
ColumnLayout {
    spacing: _margin

    property var    cameraCalc

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle

    Component.onCompleted: {
        cameraBrandCombo.selectCurrentBrand()
        cameraModelCombo.selectCurrentModel()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        Layout.fillWidth:   true
        spacing:            _margin

        QGCComboBox {
            id:                 cameraBrandCombo
            Layout.fillWidth:   true
            model:              cameraCalc.cameraBrandList
            onModelChanged:     selectCurrentBrand()
            onActivated: (index) => { cameraCalc.cameraBrand = currentText }

            Connections {
                target:                 cameraCalc
                onCameraBrandChanged:   cameraBrandCombo.selectCurrentBrand()
            }

            function selectCurrentBrand() {
                currentIndex = cameraBrandCombo.find(cameraCalc.cameraBrand)
            }
        }

        QGCComboBox {
            id:                 cameraModelCombo
            Layout.fillWidth:   true
            model:              cameraCalc.cameraModelList
            visible:            !cameraCalc.isManualCamera && !cameraCalc.isCustomCamera
            onModelChanged:     selectCurrentModel()
            onActivated: (index) => { cameraCalc.cameraModel = currentText }

            Connections {
                target:                 cameraCalc
                onCameraModelChanged:   cameraModelCombo.selectCurrentModel()
            }

            function selectCurrentModel() {
                currentIndex = cameraModelCombo.find(cameraCalc.cameraModel)
            }
        }

        // Camera based grid ui
        ColumnLayout {
            Layout.fillWidth:   true
            spacing:            _margin
            visible:            !cameraCalc.isManualCamera

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            _margin
                visible:            !cameraCalc.fixedOrientation.value

                QGCRadioButton {
                    width:          _editFieldWidth
                    text:           "Landscape"
                    checked:        !!cameraCalc.landscape.value
                    onClicked:      cameraCalc.landscape.value = 1
                }

                QGCRadioButton {
                    id:             cameraOrientationPortrait
                    text:           "Portrait"
                    checked:        !cameraCalc.landscape.value
                    onClicked:      cameraCalc.landscape.value = 0
                }
            }

            // Custom camera specs
            ColumnLayout {
                id:                 custCameraCol
                Layout.fillWidth:   true
                spacing:            _margin
                enabled:            cameraCalc.isCustomCamera

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            _margin

                    Item { Layout.fillWidth: true }
                    QGCLabel {
                        Layout.preferredWidth:  _root._fieldWidth
                        text:                   qsTr("Width")
                    }
                    QGCLabel {
                        Layout.preferredWidth:  _root._fieldWidth
                        text:                   qsTr("Height")
                    }
                }

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            _margin

                    QGCLabel { text: qsTr("Sensor"); Layout.fillWidth: true }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.sensorWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.sensorHeight
                    }
                }

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            _margin

                    QGCLabel { text: qsTr("Image"); Layout.fillWidth: true }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.imageWidth
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.imageHeight
                    }
                }

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            _margin
                    QGCLabel {
                        text:                   qsTr("Focal length")
                        Layout.fillWidth:       true
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.focalLength
                    }
                }
            }
        }
    }
}
