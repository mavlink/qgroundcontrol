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
    property bool   vehicleFlightIsFrontal:         true
    property string distanceToSurfaceLabel
    property int    distanceToSurfaceAltitudeMode:  QGroundControl.AltitudeModeNone
    property string frontalDistanceLabel
    property string sideDistanceLabel

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property string _cameraName:        cameraCalc.cameraName.value
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _cameraList:        [ ]
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var    _vehicleCameraList: _vehicle ? _vehicle.staticCameraList : []
    property bool   _cameraComboFilled: false

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    Component.onCompleted: _fillCameraCombo()

    on_CameraNameChanged: _updateSelectedCamera()

    function _fillCameraCombo() {
        _cameraComboFilled = true
        _cameraList.push(cameraCalc.manualCameraName)
        _cameraList.push(cameraCalc.customCameraName)
        for (var i=0; i<_vehicle.staticCameraList.length; i++) {
            _cameraList.push(_vehicle.staticCameraList[i].name)
        }
        gridTypeCombo.model = _cameraList
        _updateSelectedCamera()
    }

    function _updateSelectedCamera() {
        if (_cameraComboFilled) {
            var knownCameraIndex = gridTypeCombo.find(_cameraName)
            if (knownCameraIndex !== -1) {
                gridTypeCombo.currentIndex = knownCameraIndex
            } else {
                console.log("Internal error: Known camera not found", _cameraName)
                gridTypeCombo.currentIndex = _gridTypeCustomCamera
            }
        }
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
            id:             gridTypeCombo
            anchors.left:   parent.left
            anchors.right:  parent.right
            model:          _cameraList
            currentIndex:   -1
            onActivated:    cameraCalc.cameraName.value = gridTypeCombo.textAt(index)
        } // QGCComboxBox

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
                visible:        cameraCalc.isCustomCamera

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
