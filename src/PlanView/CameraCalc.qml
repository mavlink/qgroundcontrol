import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                   1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.Controls          1.0
import QGroundControl.FactControls      1.0
import QGroundControl.Palette           1.0

// Camera calculator section for mission item editors
Column {
    anchors.left:   parent.left
    anchors.right:  parent.right
    spacing:        _margin

    property var    cameraCalc
    property bool   vehicleFlightIsFrontal: true
    property string distanceToSurfaceLabel
    property string frontalDistanceLabel
    property string sideDistanceLabel

    property real   _margin:            ScreenTools.defaultFontPixelWidth / 2
    property int    _cameraIndex:       1
    property real   _fieldWidth:        ScreenTools.defaultFontPixelWidth * 10.5
    property var    _cameraList:        [ ]
    property var    _vehicle:           QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property var    _vehicleCameraList: _vehicle ? _vehicle.staticCameraList : []

    readonly property int _gridTypeManual:          0
    readonly property int _gridTypeCustomCamera:    1
    readonly property int _gridTypeCamera:          2

    Component.onCompleted: {
        _cameraList.push(cameraCalc.manualCameraName)
        _cameraList.push(cameraCalc.customCameraName)
        for (var i=0; i<_vehicle.staticCameraList.length; i++) {
            _cameraList.push(_vehicle.staticCameraList[i].name)
        }
        gridTypeCombo.model = _cameraList
        var knownCameraIndex = gridTypeCombo.find(cameraCalc.cameraName.value)
        if (knownCameraIndex !== -1) {
            gridTypeCombo.currentIndex = knownCameraIndex
        } else {
            console.log("Internal error: Known camera not found", cameraCalc.cameraName.value)
            gridTypeCombo.currentIndex = _gridTypeCustomCamera
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup {
        id: cameraOrientationGroup
    }

    ExclusiveGroup { id: fixedValueGroup }

    SectionHeader {
        id:         cameraHeader
        text:       qsTr("Camera")
        showSpacer: false
    }

    Column {
        anchors.left:   parent.left
        anchors.right:  parent.right
        spacing:        _margin
        visible:        cameraHeader.checked

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
                    exclusiveGroup: cameraOrientationGroup
                    onClicked:      cameraCalc.landscape.value = 1
                }

                QGCRadioButton {
                    id:             cameraOrientationPortrait
                    text:           "Portrait"
                    checked:        !cameraCalc.landscape.value
                    exclusiveGroup: cameraOrientationGroup
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

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                Item { Layout.fillWidth: true }
                QGCLabel {
                    Layout.preferredWidth:  _root._fieldWidth
                    text:                   qsTr("Front Lap")
                }
                QGCLabel {
                    Layout.preferredWidth:  _root._fieldWidth
                    text:                   qsTr("Side Lap")
                }
            }

            RowLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                spacing:        _margin
                QGCLabel { text: qsTr("Overlap"); Layout.fillWidth: true }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   cameraCalc.frontalOverlap
                }
                FactTextField {
                    Layout.preferredWidth:  _root._fieldWidth
                    fact:                   cameraCalc.sideOverlap
                }
            }

            QGCLabel {
                wrapMode:               Text.WordWrap
                text:                   qsTr("Select one:")
                Layout.preferredWidth:  parent.width
                Layout.columnSpan:      2
            }

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCRadioButton {
                    id:                     fixedDistanceRadio
                    text:                   distanceToSurfaceLabel
                    checked:                !!cameraCalc.valueSetIsDistance.value
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              cameraCalc.valueSetIsDistance.value = 1
                }

                FactTextField {
                    fact:                   cameraCalc.distanceToSurface
                    enabled:                fixedDistanceRadio.checked
                    Layout.fillWidth:       true
                }

                QGCRadioButton {
                    id:                     fixedImageDensityRadio
                    text:                   qsTr("Ground Res")
                    checked:                !cameraCalc.valueSetIsDistance.value
                    exclusiveGroup:         fixedValueGroup
                    onClicked:              cameraCalc.valueSetIsDistance.value = 0
                }

                FactTextField {
                    fact:                   cameraCalc.imageDensity
                    enabled:                fixedImageDensityRadio.checked
                    Layout.fillWidth:       true
                }
            }

            // Calculated values
/*
            Taking these out for now since they take up so much space. May come back at a later time.
            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCLabel { text: frontalDistanceLabel }
                FactTextField {
                    Layout.fillWidth:   true
                    fact:               cameraCalc.adjustedFootprintFrontal
                    enabled:            false
                }

                QGCLabel { text: sideDistanceLabel }
                FactTextField {
                    Layout.fillWidth:   true
                    fact:               cameraCalc.adjustedFootprintSide
                    enabled:            false
                }
            } // GridLayout
*/

        } // Column - Camera spec based ui

        // No camera spec ui
        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            columnSpacing:  _margin
            rowSpacing:     _margin
            columns:        2
            visible:        cameraCalc.isManualCamera

            QGCLabel { text: distanceToSurfaceLabel }
            FactTextField {
                fact:               cameraCalc.distanceToSurface
                Layout.fillWidth:   true
            }

            QGCLabel { text: frontalDistanceLabel }
            FactTextField {
                Layout.fillWidth:   true
                fact:               cameraCalc.adjustedFootprintFrontal
            }

            QGCLabel { text: sideDistanceLabel }
            FactTextField {
                Layout.fillWidth:   true
                fact:               cameraCalc.adjustedFootprintSide
            }
        } // GridLayout
    } // Column - Camera Section
} // Column
