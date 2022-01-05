/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.15
import QtQuick.Layouts  1.15

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

QGCPopupDialog {
    property alias  text:           label.text
    property var    acceptFunction: null        // Mainly used by MainRootWindow.showMessage to specify accept function in call

    onAccepted: {
        if (acceptFunction) {
            acceptFunction()
        }
    }

    ColumnLayout {
        QGCLabel {
            id:                     label
            Layout.maximumWidth:    mainWindow.width / (ScreenTools.isMobile ? 2 : 3)
            wrapMode:               Text.WordWrap
        }
    }
}
