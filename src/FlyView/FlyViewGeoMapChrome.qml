/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GeoMap

/// View chrome for a FlyViewGeoMap: diagnostic buttons plus the terrain
/// stats overlay, shown only when the GeoMap debug UI setting is on. Hosted
/// by the Fly View GeoMap engine adapter; the 2D/3D and compass camera
/// controls live in FlyViewGeoMapControls.
Item {
    id: root

    required property var geoMap

    // UI tests look chrome items up by objectName
    property string objectNamePrefix: "flyViewGeoMap"

    // Margins let the Fly View host push the chrome clear of its own widgets
    property real buttonsTopMargin: ScreenTools.defaultFontPixelWidth
    property real buttonsRightMargin: ScreenTools.defaultFontPixelWidth
    property real overlayBottomMargin: ScreenTools.defaultFontPixelWidth / 2

    readonly property bool _debugUIEnabled: QGroundControl.settingsManager.flyViewSettings.geoMapDebugUI.rawValue

    // Bottom-left overlay: live SurfaceModel stats for manual verification;
    // last line adds the perf counters while Stats is on. Terrain dataset
    // attribution lives in the user guide (docs/en/qgc-user-guide/fly_view/geoview.md)
    Rectangle {
        anchors.left: parent.left
        anchors.bottom: parent.bottom
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth / 2
        anchors.bottomMargin: root.overlayBottomMargin
        width: debugOverlayLabel.implicitWidth + ScreenTools.defaultFontPixelWidth
        height: debugOverlayLabel.implicitHeight + ScreenTools.defaultFontPixelWidth
        radius: ScreenTools.defaultFontPixelWidth / 2
        color: Qt.rgba(0, 0, 0, 0.5)
        visible: root._debugUIEnabled

        QGCLabel {
            id: debugOverlayLabel
            objectName: root.objectNamePrefix + "DebugOverlay"
            anchors.centerIn: parent
            font.family: ScreenTools.fixedFontFamily
            color: "white"
            text: qsTr("patches: %1  pending: %2  max zoom: %3")
                      .arg(root.geoMap.patchCount)
                      .arg(root.geoMap.pendingCount)
                      .arg(root.geoMap.maxZoomLevel)
                  + (root.geoMap.modelStats !== "" ? "\n" + root.geoMap.modelStats : "")
        }
    }

    Column {
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: root.buttonsTopMargin
        anchors.rightMargin: root.buttonsRightMargin
        spacing: ScreenTools.defaultFontPixelHeight / 2
        visible: root._debugUIEnabled

        // Diagnostic: report holes, terrain cliffs, and bad height
        // data to the debug output
        QGCButton {
            objectName: root.objectNamePrefix + "AnalyzeButton"
            text: qsTr("Analyze")
            onClicked: root.geoMap.analyzeSurface()
        }

        // Render-statistics overlay toggle (perf diagnostics)
        QGCButton {
            objectName: root.objectNamePrefix + "StatsButton"
            text: qsTr("Stats")
            onClicked: root.geoMap.renderStats = !root.geoMap.renderStats
        }

        // Perf capture: records per-second counters; the CSV path
        // shows in the debug overlay when stopped
        QGCButton {
            objectName: root.objectNamePrefix + "RecordButton"
            text: root.geoMap.perfCapturing ? qsTr("Stop") : qsTr("Record")
            onClicked: root.geoMap.perfCapturing ? root.geoMap.stopPerfCapture() : root.geoMap.startPerfCapture()
        }
    }
}
