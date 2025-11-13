import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id:                 control
    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelWidth * 2

    property var tileSet: null

    signal clicked

    property int _tileCount: tileSet ? tileSet.totalTileCount : 0

    QGCPalette { id: qgcPal }

    QGCLabel {
        Layout.fillWidth:   true
        text:               tileSet ? (tileSet.defaultSet ? qsTr("%1 (System Cache)").arg(tileSet.name) : tileSet.name) : ""
        font.italic:        tileSet && tileSet.defaultSet
    }

    QGCLabel {
        id:     sizeLabel
        text:   _computeDisplayText()
        function _computeDisplayText() {
            if (!tileSet) return ""
            if (tileSet.defaultSet) {
                return tileSet.savedTileSizeStr + " (" + tileSet.savedTileCount + " tiles)"
            }
            var result = tileSet.downloadStatus
            if (_tileCount > 0) result += " (" + _tileCount + " tiles)"
            return result + _queueSuffix()
        }
        function _queueSuffix() {
            if (!tileSet || tileSet.defaultSet) {
                return ""
            }
            var parts = []
            if (tileSet.pendingTiles > 0)
                parts.push(qsTr("%1 pending").arg(tileSet.pendingTiles))
            if (tileSet.downloadingTiles > 0)
                parts.push(qsTr("%1 active").arg(tileSet.downloadingTiles))
            if (tileSet.errorTiles > 0)
                parts.push(qsTr("%1 error").arg(tileSet.errorTiles))
            return parts.length ? " [" + parts.join(", ") + "]" : ""
        }
    }

    Rectangle {
        width:   sizeLabel.height * 0.5
        height:  sizeLabel.height * 0.5
        radius:  width / 2
        color:   tileSet && tileSet.defaultSet ? qgcPal.text : (tileSet && tileSet.complete ? qgcPal.colorGreen : qgcPal.colorRed)
        opacity: sizeLabel.text.length > 0 ? (tileSet && tileSet.defaultSet ? 0.4 : 1) : 0
    }

    QGCButton {
        text:       qsTr("Edit")
        onClicked:  control.clicked()
    }
}
