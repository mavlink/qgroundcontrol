/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.Palette
import QGroundControl.ScreenTools

RowLayout {
    id:                 control
    Layout.fillWidth:   true
    spacing:            ScreenTools.defaultFontPixelWidth * 2

    property var tileSet: null

    signal clicked

    property int _tileCount: tileSet.totalTileCount

    QGCLabel {
        Layout.fillWidth:   true
        text:               tileSet.name 
    }

    QGCLabel {
        id:     sizeLabel
        text:   tileSet.downloadStatus + (_tileCount > 0 ? " (" + _tileCount + " tiles)" : "")
    }

    Rectangle {
        width:   sizeLabel.height * 0.5
        height:  sizeLabel.height * 0.5
        radius:  width / 2
        color:   tileSet.complete ? "#31f55b" : "#fc5656"
        opacity: sizeLabel.text.length > 0 ? 1 : 0
    }

    QGCButton {
        text:       qsTr("Edit")
        onClicked:  control.clicked()
    }
}
