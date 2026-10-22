import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

/// Bar of the emergency, critical and low battery levels, each a fraction of full charge
ColumnLayout {
    property real emergency:    0
    property real critical:     0
    property real low:          0

    spacing: ScreenTools.defaultFontPixelHeight * 0.25

    QGCPalette {
        id: barPal
    }

    Row {
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 28
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.75

        Repeater {
            model: [
                { "from": 0,                "to": emergency,    "color": barPal.colorRed },
                { "from": emergency,        "to": critical,     "color": barPal.colorOrange },
                { "from": critical,         "to": low,          "color": barPal.colorYellow },
                { "from": low,              "to": 1,            "color": barPal.colorGreen }
            ]

            Rectangle {
                width:  Math.max(0, (modelData.to - modelData.from)) * parent.width
                height: parent.height
                color:  modelData.color

                required property var modelData
            }
        }
    }

    QGCLabel {
        font.pointSize: ScreenTools.smallFontPointSize
        color:          barPal.textSecondary
        text:           qsTr("Emergency %1%, critical %2%, low %3%")
                            .arg(Math.round(emergency * 100))
                            .arg(Math.round(critical * 100))
                            .arg(Math.round(low * 100))
    }
}
