import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Groups of rows from PX4ConfigRows.groupRows, each under a ConfigGroupHeader. rowDelegate is created for each row
/// with the required properties modelData (the row) and index (its position in the group).
Repeater {
    property var        groupShown:         function (group) { return true }
    property Component  rowDelegate
    /// Prefix of the test hook objectName of each group, which ends with the group name in letters and digits
    property string     objectNamePrefix:   ""

    ColumnLayout {
        objectName:         objectNamePrefix !== "" ? objectNamePrefix + "_group_" + modelData.name.replace(/[^A-Za-z0-9]/g, "") : ""
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
        visible:            groupShown(modelData)

        required property var modelData

        ConfigGroupHeader {
            text: modelData.name
        }

        Repeater {
            model:      modelData.rows
            delegate:   rowDelegate
        }
    }
}
