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
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl

import QGroundControl.FactControls
import QGroundControl.Controls




Item {
    id: root

    default property alias contentItem: mainLayout.data

    QGCFlickable {
        anchors.fill:   parent
        contentWidth:   mainLayout.width
        contentHeight:  mainLayout.height

        GridLayout {
            id:         mainLayout
            columns:    2
            x:          Math.max(0, root.width / 2 - width / 2)                             // left aligned
            // Ensure enough width for two panels — adjust multiplier as needed
            width:      Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 100)
            rowSpacing: ScreenTools.defaultFontPixelHeight
            columnSpacing: ScreenTools.defaultFontPixelWidth * 6
        }
    }
}
