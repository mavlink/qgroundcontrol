import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.FlightMap

Rectangle {
    id:         _root
    height:     childrenRect.y + childrenRect.height + _margin
    width:      availableWidth
    color:      qgcPal.windowShadeDark
    radius:     _radius

    property bool   transectAreaDefinitionComplete: true
    property string transectAreaDefinitionHelp:     _internalError
    property string transectValuesHeaderName:       _internalError
    property var    transectValuesComponent:        undefined
    property var    presetsTransectValuesComponent: undefined

    readonly property string _internalError: "Internal Error"

    property var    _missionItem:               missionItem
    property real   _margin:                    ScreenTools.defaultFontPixelWidth / 2
    readonly property string _entryPointLabel:   missionItem.gridEntryLocation === 0 ? qsTr("Top Left") :
                                                missionItem.gridEntryLocation === 1 ? qsTr("Top Right") :
                                                missionItem.gridEntryLocation === 2 ? qsTr("Bottom Left") : qsTr("Bottom Right")
    property real   _fieldWidth:                ScreenTools.defaultFontPixelWidth * 10.5
    property var    _vehicle:                   QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.offlineEditingVehicle
    property string _doneAdjusting:             qsTr("Done")
    
    function polygonCaptureStarted() {
        _missionItem.clearPolygon()
    }

    function polygonCaptureFinished(coordinates) {
        for (var i=0; i<coordinates.length; i++) {
            _missionItem.addPolygonCoordinate(coordinates[i])
        }
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        _missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        id:                 editorColumn
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right

        QGCLabel {
                id:                 wizardLabel
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                horizontalAlignment:    Text.AlignHCenter
                text:                   qsTr("Use the Polygon Tools to create the polygon which outlines the field.")
                visible:                !missionItem.sprayAreaPolygon.isValid || missionItem.wizardMode
            }

        ColumnLayout {
            Layout.fillWidth:   true
            spacing:        _margin
            visible:        !wizardLabel.visible

            GridLayout {
                Layout.fillWidth:   true
                columnSpacing:  _margin
                rowSpacing:     _margin
                columns:        2

                QGCLabel {
                    text:       qsTr("Speed")
                }
                FactTextField {
                    fact:               missionItem.speed
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Alt") }
                AltitudeFactTextField {
                    fact:               missionItem.altitude
                    altitudeMode:       QGroundControl.AltitudeModeTerrain
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text:       qsTr("Width")
                }
                FactTextField {
                    fact:               missionItem.sprayWidth
                    Layout.fillWidth:   true
                }

                QGCLabel { text: qsTr("Angle") }
                FactTextField {
                    fact:                   missionItem.gridAngle
                    Layout.fillWidth:       true
                    onUpdated:              angleSlider.value = missionItem.gridAngle.value
                }

                QGCSlider {
                    id:                     angleSlider
                    from:                   0
                    to:                     359
                    stepSize:               1
                    tickmarksEnabled:       false
                    Layout.fillWidth:       true
                    Layout.columnSpan:      2
                    Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                    onValueChanged:         missionItem.gridAngle.value = value
                    Component.onCompleted:  value = missionItem.gridAngle.value
                    live:                   true
                }

                QGCLabel {
                    text:       qsTr("Turnaround dist")
                }
                FactTextField {
                    fact:               missionItem.turnAroundDistance
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text:       qsTr("Obstacle buffer")
                }
                FactTextField {
                    fact:               missionItem.obstacleBuffer
                    Layout.fillWidth:   true
                }

                QGCLabel {
                    text:       qsTr("Transect start")
                }
                QGCButton {
                    Layout.fillWidth: true
                    text:       _entryPointLabel
                    onClicked:  missionItem.rotateEntryPoint()
                }
            }

            SectionHeader {
                Layout.fillWidth:   true
                text:               qsTr("Obstacles (no-fly zones)")
            }

            QGCLabel {
                Layout.fillWidth:   true
                wrapMode:           Text.WordWrap
                text:               qsTr("Add polygons inside the field to avoid (trees, buildings). The drone will fly around them.")
            }

            QGCButton {
                text:               qsTr("Add obstacle")
                onClicked:          missionItem.addObstaclePolygon()
            }

            Repeater {
                model: missionItem.obstaclePolygons ? missionItem.obstaclePolygons.count : 0
                delegate: RowLayout {
                    Layout.fillWidth: true
                    QGCLabel {
                        text: qsTr("Obstacle %1").arg(index + 1)
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
