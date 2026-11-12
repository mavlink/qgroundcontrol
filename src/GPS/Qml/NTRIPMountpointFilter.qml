pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// Narrows the mountpoint list of a source table to the mountpoints whose name or description contains the text
/// entered, and says how many of the caster's mountpoints that leaves.
ColumnLayout {
    id: root

    required property NTRIPSourceTableController sourceTable
    /// The number of mountpoints the list shows.
    required property int listedCount

    Layout.fillWidth: true
    spacing: ScreenTools.defaultFontPixelHeight * 0.25
    visible: root.sourceTable.mountpointCount > 0

    QGCTextField {
        objectName: "ntripMountpointFilter"
        Layout.fillWidth: true
        placeholderText: qsTr("Filter by name")
        text: root.sourceTable.mountpointFilter
        onTextChanged: root.sourceTable.mountpointFilter = text
    }

    QGCNoteLabel {
        objectName: "ntripMountpointFilterCount"
        visible: root.sourceTable.mountpointFilter.trim() !== ""
        //: %1 is the number of mountpoints the filter leaves in the list, %2 the number the caster has
        text: qsTr("%1 of %2 mountpoints").arg(root.listedCount).arg(root.sourceTable.mountpointCount)
    }
}
