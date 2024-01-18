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

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactSystem
import QGroundControl.ScreenTools

ColumnLayout {
    id:         root
    width:      availableWidth
    spacing:    ScreenTools.defaultFontPixelWidth

    property var model

    property real _availableHeight:  availableHeight

    FactPanelController {
        id:         controller
    }

    QGCTabBar {
        id:                 tabBar
        Layout.fillWidth:   true
        width:              parent.width

        Repeater {
            model: root.model
            QGCTabButton {
                text: buttonText
            }
        }
    }

    Loader {
        id:                loader
        source:            model.get(tabBar.currentIndex).tuningPage
        Layout.fillWidth:   true

        property bool useAutoTuning:    true
        property real availableHeight:  _availableHeight - loader.y
    }
}
