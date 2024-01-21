/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl.Controls
import QGroundControl.ScreenTools

RowLayout {
    property alias label:                   _label.text
    property alias buttonText:              _button.text
    property real  buttonPreferredWidth:    -1

    signal clicked

    id:         _root
    spacing:    ScreenTools.defaultFontPixelWidth * 2

    QGCLabel { 
        id:                 _label
        Layout.fillWidth:   true 
    }

    QGCButton {
        id:                     _button
        Layout.preferredWidth:  buttonPreferredWidth
        onClicked:              _root.clicked()
    }
}

