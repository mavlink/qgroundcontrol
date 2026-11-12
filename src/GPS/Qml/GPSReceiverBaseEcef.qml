pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The fixed base position entered as ECEF X, Y and Z, as a surveyor's or a reference station's data sheet gives it:
/// another view of the position the settings save as latitude, longitude and height.
ColumnLayout {
    id: root

    required property RTKSettings settings

    Layout.minimumWidth: 0
    spacing: ScreenTools.defaultFontPixelHeight / 2

    GPSFixedBaseEcef {
        id: ecef
        settings: root.settings
    }

    GPSFactTextField {
        objectName: "rtkFixedBaseEcefX"
        fact: ecef.x
    }

    GPSFactTextField {
        objectName: "rtkFixedBaseEcefY"
        fact: ecef.y
    }

    GPSFactTextField {
        objectName: "rtkFixedBaseEcefZ"
        fact: ecef.z
    }

    QGCNoteLabel {
        objectName: "rtkFixedBaseEcefUnsaved"
        visible: ecef.unsaved
        warning: true
        text: qsTr("X, Y and Z are not a position near the Earth's surface yet, so the base position is unchanged.")
    }
}
