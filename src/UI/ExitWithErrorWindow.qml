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
import QtQuick.Window

ApplicationWindow {
    id:             errorWindow
    minimumWidth:   messageArea.width  + 60
    minimumHeight:  messageArea.height + 60
    width:          messageArea.width  + 60
    height:         messageArea.height + 60
    visible:        true

    //-------------------------------------------------------------------------
    //-- Main, full window background (Fly View)
    background: Item {
        id:             rootBackground
        anchors.fill:   parent
        Rectangle {
            anchors.fill:   parent
            color:          "#000000"
        }
    }

    Column {
        id:                 messageArea
        spacing:            20
        anchors.centerIn:   parent
        Label {
            width:          600
            text:           errorMessage
            color:          "#eecc44"
            wrapMode:       Text.WordWrap
            anchors.horizontalCenter: parent.horizontalCenter
        }
        Button {
            text:           qsTr("Close")
            highlighted:    true
            onClicked:      errorWindow.close()
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

}
