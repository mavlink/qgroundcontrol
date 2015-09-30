import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

/// Canvas has some sort of bug in it which can cause it to not paint when top level Views
/// are switched. In order to fix this we ahve a signal hacked into ScreenTools to force
/// a repaint.
Canvas {
    Connections {
        target: ScreenTools

        onRepaintRequested: arrowCanvas.requestPaint()
    }
}
