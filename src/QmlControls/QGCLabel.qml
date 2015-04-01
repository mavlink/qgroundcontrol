import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

Text {
    QGCPalette { id: __qgcPal; colorGroupEnabled: enabled }
    ScreenTools { id: __screenTools }

    property real dpiAdjustPointSize: __qgcPal.defaultFontPointSize
    property bool enabled: true

    font.pointSize: __screenTools.dpiAdjustedPointSize(dpiAdjustPointSize)
    color:          __qgcPal.text
}
