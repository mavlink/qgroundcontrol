import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0

Text {
    property var __palette: QGCPalette { colorGroup: enabled ? QGCPalette.Active : QGCPalette.Disabled }
    property bool enabled: true

    color: __palette.windowText
}
