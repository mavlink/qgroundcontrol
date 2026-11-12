pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// The satellites the receiver has in view, as signal bars and a sky plot, behind a row that shows and hides them.
/// Hidden, they are not created.
ColumnLayout {
    id: root

    property GPSSatelliteModel satellites
    /// The bars and the plot are shown. Until Show or Hide sets it, they are shown in a panel wider than the status
    /// column of the toolbar's drop-down, which starts with them hidden.
    property bool expanded: root.width >= ScreenTools.defaultFontPixelWidth * 44

    readonly property real _plotSize: ScreenTools.defaultFontPixelWidth * 34
    /// The plot has room beside the bars; in a narrower panel it is above them.
    readonly property bool _sideBySide: root.width >= root._plotSize * 2

    spacing: ScreenTools.defaultFontPixelHeight / 2

    LabelledButton {
        objectName: "gpsSatelliteViewToggle"
        label: qsTr("Satellite Signals")
        buttonText: root.expanded ? qsTr("Hide") : qsTr("Show")
        onClicked: root.expanded = !root.expanded
    }

    Loader {
        Layout.fillWidth: true
        Layout.minimumWidth: 0
        active: root.expanded
        visible: active

        sourceComponent: GridLayout {
            columns: root._sideBySide ? 2 : 1
            columnSpacing: ScreenTools.defaultFontPixelWidth * 2
            rowSpacing: root.spacing

            GPSSkyPlot {
                objectName: "gpsSkyPlot"
                Layout.alignment: Qt.AlignHCenter | Qt.AlignTop
                // Square, and no wider than a panel narrower than the plot.
                Layout.preferredWidth: Math.min(root._plotSize, root.width)
                Layout.preferredHeight: Math.min(root._plotSize, root.width)
                satellites: root.satellites
            }

            // Its implicit width, all bars in one row, would widen the panel.
            GPSSatelliteSignalBars {
                objectName: "gpsSatelliteSignalBars"
                Layout.alignment: Qt.AlignTop
                Layout.fillWidth: true
                Layout.minimumWidth: 0
                Layout.preferredWidth: 0
                satellites: root.satellites
            }
        }
    }
}
