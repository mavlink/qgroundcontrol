import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

Rectangle {
    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    width: 400
    height: 400
    color: palette.window

    Column {
        anchors.fill: parent
        Label { text: "Safety Config"; color: palette.windowText; font.pointSize: 20 }
        Label { text: "Return to Land setup"; color: palette.windowText }
        Label { text: "Return to Land Triggers"; color: palette.windowText }
        Row {
            CheckBox { text: "Telemetry signal timeout - Return to Land after" }
            FactTextField { fact: autopilot.parameters["NAV_DLL_N"] }
        }
        Row {
            CheckBox { text: "RC Transmitter signal loss - Return to Land after" }
            FactTextField { fact: autopilot.parameters["COM_RC_LOSS_T"] }
        }
    }
}
