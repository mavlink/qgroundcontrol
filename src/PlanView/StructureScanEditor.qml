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
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.FlightMap     1.0

// Editor for Survery mission items
Rectangle {
    id:         _root
    height:     visible ? (editorColumn.height + (_margin * 2)) : 0
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    // The following properties must be available up the hierarchy chain
    //property real   availableWidth    ///< Width for control
    //property var    missionItem       ///< Mission Item for editor

    property real   _margin:        ScreenTools.defaultFontPixelWidth / 2
    property real   _fieldWidth:    ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:       QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle

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
            text:           qsTr("Note: Polygon respresents structure surface not vehicle flight path.")
            wrapMode:       Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
        }

        QGCLabel {
            anchors.left:   parent.left
            anchors.right:  parent.right
            text:           qsTr("WARNING: Photo interval is below minimum interval (%1 secs) supported by camera.").arg(missionItem.cameraMinTriggerInterval.toFixed(1))
            wrapMode:       Text.WordWrap
            color:          qgcPal.warningText
            visible:        missionItem.cameraShots > 0 && missionItem.cameraMinTriggerInterval !== 0 && missionItem.cameraMinTriggerInterval > missionItem.timeBetweenShots
        }

        CameraCalc {
            cameraCalc:             missionItem.cameraCalc
            vehicleFlightIsFrontal: false
            distanceToSurfaceLabel: qsTr("Scan Distance")
            frontalDistanceLabel:   qsTr("Layer Height")
            sideDistanceLabel:      qsTr("Trigger Distance")
        }

        SectionHeader {
            id:         scanHeader
            text:       qsTr("Scan")
        }

        Column {
            anchors.left:   parent.left
            anchors.right:  parent.right
            spacing:        _margin
            visible:        scanHeader.checked

            GridLayout {
                anchors.left:   parent.left
                anchors.right:  parent.right
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCLabel {
                    text:       qsTr("Structure height")
                    visible:    !missionItem.cameraCalc.isManualCamera
                }
                FactTextField {
                    fact:               missionItem.structureHeight
                    Layout.fillWidth:   true
                    visible:            !missionItem.cameraCalc.isManualCamera
                }

                QGCLabel {
                    text:       qsTr("# Layers")
                    visible:    missionItem.cameraCalc.isManualCamera
                }
                FactTextField {
                    fact:               missionItem.layers
                    Layout.fillWidth:   true
                    visible:            missionItem.cameraCalc.isManualCamera
                }

                QGCLabel { text: qsTr("Bottom layer alt") }
                FactTextField {
                    fact:               missionItem.altitude
                    Layout.fillWidth:   true
                }

                QGCCheckBox {
                    text:               qsTr("Relative altitude")
                    checked:            missionItem.altitudeRelative
                    Layout.columnSpan:  2
                    onClicked:          missionItem.altitudeRelative = checked
                }
            }

            Item {
                height: ScreenTools.defaultFontPixelHeight / 2
                width:  1
            }

            QGCButton {
                text:       qsTr("Rotate entry point")
                onClicked:  missionItem.rotateEntryPoint()
            }
        } // Column - Scan

        SectionHeader {
            id:     statsHeader
            text:   qsTr("Statistics")
        }

        Grid {
            columns:        2
            columnSpacing:  ScreenTools.defaultFontPixelWidth
            visible:        statsHeader.checked

            QGCLabel { text: qsTr("Photo count") }
            QGCLabel { text: missionItem.cameraShots }

            QGCLabel { text: qsTr("Photo interval") }
            QGCLabel { text: missionItem.timeBetweenShots.toFixed(1) + " " + qsTr("secs") }
        }
    } // Column
} // Rectangle
