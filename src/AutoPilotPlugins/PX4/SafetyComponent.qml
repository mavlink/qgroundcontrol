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
        Label { text: "Work in Progress"; color: palette.windowText }
        Label { text: "Return to Land Altitude"; color: palette.windowText }
        FactLabel { fact: autopilot.parameters["RTL_RETURN_ALT"] }
        FactTextField { fact: autopilot.parameters["RTL_RETURN_ALT"] }
    }
}
