import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.FlightMap

/// \brief Editor panel for PerimeterScanComplexItem.
///
/// Appears in the right-hand mission item list when the item is selected.
/// The polygon tools toolbar on the map is provided automatically by
/// QGCMapPolygonVisuals — no extra wiring is needed here.

Rectangle {
    id:     _root
    height: visible ? (editorColumn.height + (_margin * 2)) : 0
    width:  availableWidth
    color:  qgcPal.windowShadeDark
    radius: _radius

    required property var  missionItem
    required property real availableWidth

    property real _margin:     ScreenTools.defaultFontPixelWidth / 2
    property real _fieldWidth: ScreenTools.defaultFontPixelWidth * 10.5
    property real _radius:     ScreenTools.defaultFontPixelWidth / 2

    // These callbacks are invoked by QGCMapPolygonVisuals when it is used
    // in a "capture" mode (legacy path — kept for API completeness).
    function polygonCaptureStarted()                              { missionItem.clearPolygon() }
    function polygonCaptureFinished(coordinates) {
        for (var i = 0; i < coordinates.length; i++) {
            missionItem.addPolygonCoordinate(coordinates[i])
        }
    }
    function polygonAdjustVertex(vertexIndex, vertexCoordinate)   { missionItem.adjustPolygonCoordinate(vertexIndex, vertexCoordinate) }
    function polygonAdjustStarted()  {}
    function polygonAdjustFinished() {}

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ColumnLayout {
        id:             editorColumn
        anchors {
            top:    parent.top
            left:   parent.left
            right:  parent.right
            margins: _margin
        }
        spacing: _margin

        // Wizard hint – shown until the polygon is valid.
        QGCLabel {
            Layout.fillWidth:    true
            wrapMode:            Text.WordWrap
            horizontalAlignment: Text.AlignHCenter
            text:                qsTr("Use the Polygon Tools to draw the perimeter to scan.")
            visible:             !missionItem.perimeterPolygon.isValid
        }

        // Settings – shown once the polygon has been defined.
        GridLayout {
            Layout.fillWidth: true
            columnSpacing:    _margin
            rowSpacing:       _margin
            columns:          2
            visible:          missionItem.perimeterPolygon.isValid

            QGCLabel { text: qsTr("Altitude") }
            FactTextField {
                fact:               missionItem.altitude
                Layout.preferredWidth: _fieldWidth
                showUnits:          true
            }

            QGCLabel { text: qsTr("Perimeter") }
            QGCLabel {
                text: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(
                          missionItem.complexDistance).toFixed(0)
                      + " "
                      + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
            }
        }
    }
}
