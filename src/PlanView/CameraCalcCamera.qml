import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

// Camera calculator "Camera" section for mission item editors
Column {
    anchors.left:   parent.left
    anchors.right:  parent.right
    spacing:        _margin

    property var    cameraCalc

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var    _vehicleCameraList: _vehicle ? _vehicle.staticCameraList : []

    Component.onCompleted:{
        cameraBrandCombo.selectCurrentBrand()
        cameraModelCombo.selectCurrentModel()
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id: cameraOrientationGroup
    }

    Column {
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        _margin

        QGCComboBox {
            id:             cameraBrandCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          cameraCalc.cameraBrandList
            onModelChanged: selectCurrentBrand()
            onActivated:    cameraCalc.cameraBrand = currentText

            Connections {
                target:                 cameraCalc
                onCameraBrandChanged:   cameraBrandCombo.selectCurrentBrand()
            }

            function selectCurrentBrand() {
                currentIndex = cameraBrandCombo.find(cameraCalc.cameraBrand)
            }
        }

        QGCComboBox {
            id:             cameraModelCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          cameraCalc.cameraModelList
            visible:        !cameraCalc.isManualCamera && !cameraCalc.isCustomCamera
            onModelChanged: selectCurrentModel()
            onActivated:    cameraCalc.cameraModel = currentText

            Connections {
                target:                 cameraCalc
                onCameraModelChanged:   cameraModelCombo.selectCurrentModel()
            }

            function selectCurrentModel() {
                currentIndex = cameraModelCombo.find(cameraCalc.cameraModel)
            }
        }

        // Camera based grid ui
        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        !cameraCalc.isManualCamera

            Row {
                spacing:                    _margin
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    !cameraCalc.fixedOrientation.value

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
            Column {
                id:             custCameraCol
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                enabled:        cameraCalc.isCustomCamera

                RowLayout {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
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
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
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
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
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
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    spacing:        _margin
                    QGCLabel {
                        text:                   qsTr("Focal length")
                        Layout.fillWidth:       true
                    }
                    FactTextField {
                        Layout.preferredWidth:  _root._fieldWidth
                        fact:                   cameraCalc.focalLength
                    }
                }
            } // Column - custom camera specs
        } // Column - Camera spec based ui
    } // Column - Camera Section
} // Column
