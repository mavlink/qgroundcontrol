import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.FlightMap

TransectStyleComplexItemEditor {
    transectAreaDefinitionComplete: missionItem.surveyAreaPolygon.isValid
    transectAreaDefinitionHelp:     qsTr("Use the Polygon Tools to create the polygon which outlines your survey area.")
    transectValuesHeaderName:       qsTr("Transects")
    transectValuesComponent:        _transectValuesComponent
    presetsTransectValuesComponent: _transectValuesComponent

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property var    _missionItem:   missionItem

    Component {
        id: _transectValuesComponent

        GridLayout {
            Layout.fillWidth:   true
            columnSpacing:      _margin
            rowSpacing:         _margin
            columns:            2

            QGCLabel { text: qsTr("Angle") }
            FactTextField {
                fact:                   missionItem.gridAngle
                Layout.fillWidth:       true
                onUpdated:              angleSlider.value = missionItem.gridAngle.value
            }

            QGCSlider {
                id:                     angleSlider
                from:           0
                to:           359
                stepSize:               1
                Layout.fillWidth:       true
                Layout.columnSpan:      2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                onValueChanged:         missionItem.gridAngle.value = value
                Component.onCompleted:  value = missionItem.gridAngle.value
                live: true
            }

            QGCLabel {
                text:       qsTr("Turnaround dist")
                visible:    !forPresets
            }
            FactTextField {
                Layout.fillWidth:   true
                fact:               missionItem.turnAroundDistance
                visible:            !forPresets
            }

            QGCOptionsComboBox {
                Layout.columnSpan:  2
                Layout.fillWidth:   true
                visible:            !forPresets

                model: [
                    {
                        text:       qsTr("Hover and capture image"),
                        fact:       missionItem.hoverAndCapture,
                        enabled:    missionItem.cameraCalc.distanceMode === QGroundControl.AltitudeFrameRelative || missionItem.cameraCalc.distanceMode === QGroundControl.AltitudeFrameAbsolute,
                        visible:    missionItem.hoverAndCaptureAllowed
                    },
                    {
                        text:       qsTr("Refly at 90 deg offset"),
                        fact:       missionItem.refly90Degrees,
                        enabled:    missionItem.cameraCalc.distanceMode !== QGroundControl.AltitudeFrameCalcAboveTerrain,
                        visible:    true
                    },
                    {
                        text:       qsTr("Images in turnarounds"),
                        fact:       missionItem.cameraTriggerInTurnAround,
                        enabled:    missionItem.hoverAndCaptureAllowed ? !missionItem.hoverAndCapture.rawValue : true,
                        visible:    true
                    },
                    {
                        text:       qsTr("Fly alternate transects"),
                        fact:       missionItem.flyAlternateTransects,
                        enabled:    true,
                        visible:    _vehicle ? (_vehicle.fixedWing || _vehicle.vtol) : false
                    }
                ]
            }

            SectionHeader {
                Layout.columnSpan:   2
                Layout.fillWidth:   true
                text:               qsTr("Squares and circles (no-fly)")
                visible:            !forPresets
            }

            QGCLabel {
                Layout.columnSpan:  2
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               qsTr("Add squares or circles inside the survey area to avoid. The vehicle will fly around them.")
                visible:            !forPresets
            }

            Item {
                Layout.columnSpan:  2
                Layout.fillWidth:   true
                Layout.preferredHeight: squareCircleColumn.height
                visible:            !forPresets

                Column {
                    id:                 squareCircleColumn
                    width:             parent.width
                    spacing:            _margin

                    RowLayout {
                        width: parent.width - 1
                        QGCLabel {
                            text:       qsTr("Side (m)")
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                        }
                        FactTextField {
                            id:             squareSideField
                            text:           "20"
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 5
                            validator:       DoubleValidator { bottom: 1; top: 500; decimals: 1 }
                        }
                        QGCButton {
                            text:   qsTr("Add square")
                            onClicked: missionItem.addSquare(parseFloat(squareSideField.text) || 20)
                        }
                    }
                    RowLayout {
                        width: parent.width - 1
                        QGCLabel {
                            text:       qsTr("Radius (m)")
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 8
                        }
                        FactTextField {
                            id:             circleRadiusField
                            text:           "10"
                            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 5
                            validator:       DoubleValidator { bottom: 1; top: 500; decimals: 1 }
                        }
                        QGCButton {
                            text:   qsTr("Add circle")
                            onClicked: missionItem.addCircle(parseFloat(circleRadiusField.text) || 10)
                        }
                    }
                    Repeater {
                        model: missionItem.obstaclePolygons ? missionItem.obstaclePolygons.count : 0
                        delegate: RowLayout {
                            width: parent.width - 1
                            QGCLabel {
                                text: qsTr("Shape %1").arg(index + 1)
                            }
                            Item { Layout.fillWidth: true }
                            QGCButton {
                                text: qsTr("Remove")
                                onClicked: missionItem.removeObstaclePolygon(index)
                            }
                        }
                    }
                }
            }
        }
    }

    KMLOrSHPFileDialog {
        id:             kmlOrSHPLoadDialog
        title:          qsTr("Select Polygon File")

        onAcceptedForLoad: (file) => {
            missionItem.surveyAreaPolygon.loadKMLOrSHPFile(file)
            missionItem.resetState = false
            //editorMap.mapFitFunctions.fitMapViewportTomissionItems()
            close()
        }
    }
}
