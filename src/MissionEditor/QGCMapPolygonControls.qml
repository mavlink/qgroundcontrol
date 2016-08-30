import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0

/// Controls for drawing/editing map polygon
Column {
    id:     root
    spacing: _margin

    property var sectionLabel: qsTr("Polygon:") ///< Section label
    property var flightMap                      ///< Must be set to FlightMap control
    property var polygon                        ///< Must be set to MapPolygon

    property real _margin: ScreenTools.defaultFontPixelWidth / 2

    function polygonCaptureStarted() {
        polygon.clear()
    }

    function polygonCaptureFinished(coordinates) {
        polygon.path = coordinates
    }

    function polygonAdjustVertex(vertexIndex, vertexCoordinate) {
        polygon.adjustCoordinate(vertexIndex, vertexCoordinate)
    }

    function polygonAdjustStarted() { }
    function polygonAdjustFinished() { }

    QGCLabel { text: sectionLabel }

    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         1
        color:          qgcPal.text
    }

    Row {
        spacing: ScreenTools.defaultFontPixelWidth

        QGCButton {
            text:       flightMap.polygonDraw.drawingPolygon ? qsTr("Finish Draw") : qsTr("Draw")
            visible:    !flightMap.polygonDraw.adjustingPolygon
            enabled:    ((flightMap.polygonDraw.drawingPolygon && flightMap.polygonDraw.polygonReady) || !flightMap.polygonDraw.drawingPolygon)

            onClicked: {
                if (flightMap.polygonDraw.drawingPolygon) {
                    flightMap.polygonDraw.finishCapturePolygon()
                } else {
                    flightMap.polygonDraw.startCapturePolygon(root)
                }
            }
        }

        QGCButton {
            text:       flightMap.polygonDraw.adjustingPolygon ? qsTr("Finish Adjust") : qsTr("Adjust")
            visible:    polygon.path.length > 0 && !flightMap.polygonDraw.drawingPolygon

            onClicked: {
                if (flightMap.polygonDraw.adjustingPolygon) {
                    flightMap.polygonDraw.finishAdjustPolygon()
                } else {
                    flightMap.polygonDraw.startAdjustPolygon(root, polygon.path)
                }
            }
        }
    }
}
