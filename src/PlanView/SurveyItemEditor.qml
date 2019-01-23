import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Dialogs  1.2
import QtQuick.Extras   1.4
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real   _margin:                    ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:                   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property real   _cameraMinTriggerInterval:  missionItem.cameraCalc.minTriggerInterval.rawValue

    function polygonCaptureStarted() {
        missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Column {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        spacing:            _margin

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(_cameraMinTriggerInterval.toFixed(1))
            wrapMode:       Text.WordWrap
            color:          qgcPal.warningText
            visible:        missionItem.cameraShots > 0 && _cameraMinTriggerInterval !== 0 && _cameraMinTriggerInterval > missionItem.timeBetweenShots
        }

        CameraCalc {
            cameraCalc:             missionItem.cameraCalc
            vehicleFlightIsFrontal: true
            distanceToSurfaceLabel: qsTr("Altitude")
            frontalDistanceLabel:   qsTr("Trigger Distance")
            sideDistanceLabel:      qsTr("Spacing")
        }

        SectionHeader {
            id:     transectsHeader
            text:   qsTr("Transects")
        }

        GridLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            columnSpacing:  _margin
            rowSpacing:     _margin
            columns:        2
            visible:        transectsHeader.checked

            QGCLabel { text: qsTr("Angle") }
            FactTextField {
                fact:                   missionItem.gridAngle
                Layout.fillWidth:       true
                onUpdated:              angleSlider.value = missionItem.gridAngle.value
            }

            QGCSlider {
                id:                     angleSlider
                minimumValue:           0
                maximumValue:           359
                stepSize:               1
                tickmarksEnabled:       false
                Layout.fillWidth:       true
                Layout.columnSpan:      2
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                onValueChanged:         missionItem.gridAngle.value = value
                Component.onCompleted:  value = missionItem.gridAngle.value
                updateValueWhileDragging: true
            }

            QGCLabel { text: qsTr("Turnaround dist") }
            FactTextField {
                fact:               missionItem.turnAroundDistance
                Layout.fillWidth:   true
            }
        }

        ColumnLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        transectsHeader.checked

            QGCButton {
                text:               qsTr("Rotate Entry Point")
                onClicked:          missionItem.rotateEntryPoint();
            }

            /*
              Temporarily removed due to bug https://github.com/mavlink/qgroundcontrol/issues/7005
            FactCheckBox {
                text:       qsTr("Split concave polygons")
                fact:       _splitConcave
                visible:    _splitConcave.visible
                property Fact _splitConcave: missionItem.splitConcavePolygons
            }
            */

            FactCheckBox {
                text:               qsTr("Hover and capture image")
                fact:               missionItem.hoverAndCapture
                visible:            missionItem.hoverAndCaptureAllowed
                enabled:            !missionItem.followTerrain
                onClicked: {
                    if (checked) {
                        missionItem.cameraTriggerInTurnAround.rawValue = false
                    }
                }
            }

            FactCheckBox {
                text:               qsTr("Refly at 90 deg offset")
                fact:               missionItem.refly90Degrees
                enabled:            !missionItem.followTerrain
            }

            FactCheckBox {
                text:               qsTr("Images in turnarounds")
                fact:               missionItem.cameraTriggerInTurnAround
                enabled:            missionItem.hoverAndCaptureAllowed ? !missionItem.hoverAndCapture.rawValue : true
            }

            FactCheckBox {
                text:               qsTr("Fly alternate transects")
                fact:               missionItem.flyAlternateTransects
                visible:            _vehicle.fixedWing || _vehicle.vtol
            }

            QGCCheckBox {
                id:                 relAlt
                Layout.alignment:   Qt.AlignLeft
                text:               qsTr("Relative altitude")
                checked:            missionItem.cameraCalc.distanceToSurfaceRelative
                enabled:            missionItem.cameraCalc.isManualCamera && !missionItem.followTerrain
                visible:            QGroundControl.corePlugin.options.showMissionAbsoluteAltitude || (!missionItem.cameraCalc.distanceToSurfaceRelative && !missionItem.followTerrain)
                onClicked:          missionItem.cameraCalc.distanceToSurfaceRelative = checked

                Connections {
                    target: missionItem.cameraCalc
                    onDistanceToSurfaceRelativeChanged: relAlt.checked = missionItem.cameraCalc.distanceToSurfaceRelative
                }
            }
        }

        SectionHeader {
            id:         terrainHeader
            text:       qsTr("Terrain")
            checked:    missionItem.followTerrain
        }

        ColumnLayout {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        terrainHeader.checked

            QGCCheckBox {
                id:         followsTerrainCheckBox
                text:       qsTr("Vehicle follows terrain")
                checked:    missionItem.followTerrain
                onClicked:  missionItem.followTerrain = checked
            }

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:      _margin
                rowSpacing:         _margin
                columns:            2
                visible:            followsTerrainCheckBox.checked

                QGCLabel { text: qsTr("Tolerance") }
                FactTextField {
                    fact:               missionItem.terrainAdjustTolerance
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Max Climb Rate") }
                FactTextField {
                    fact:               missionItem.terrainAdjustMaxClimbRate
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Max Descent Rate") }
                FactTextField {
                    fact:               missionItem.terrainAdjustMaxDescentRate
                    Layout.fillWidth:   true
                }
            }
        }

        SectionHeader {
            id:     statsHeader
            text:   qsTr("Statistics")
        }

        TransectStyleComplexItemStats { }
    } // Column
} // Rectangle
