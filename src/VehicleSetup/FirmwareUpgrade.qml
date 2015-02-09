import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0

Rectangle {
    width: 600
    height: 400

    property var qgcPal: QGCPalette { colorGroup: QGCPalette.Active }

    color: qgcPal.window

    Text {
        text: "FIRMWARE UPDATE"
        color: qgcPal.windowText
        font.pointSize: 20
    }

    Column {
        QGCRadioButton {
            text: qsTr("Standard Version (stable)")
        }
        QGCRadioButton {
            text: qsTr("Beta Testing (beta)")
        }
        QGCRadioButton {
            text: qsTr("Developer Build (master)")
        }
        QGCRadioButton {
            text: qsTr("Custom firmware file...")
        }
    }
}
