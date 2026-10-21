import QtQuick

import QGroundControl
import QGroundControl.Controls

/// Small uppercase label for the caption of a cell or section
QGCLabel {
    id:                     control
    font.pointSize:         ScreenTools.smallFontPointSize
    font.bold:              true
    font.capitalization:    Font.AllUppercase
    font.letterSpacing:     ScreenTools.defaultFontPixelWidth * 0.15
    color:                  qgcPal.textSecondary

    QGCPalette { id: qgcPal; colorGroupEnabled: control.enabled }
}
