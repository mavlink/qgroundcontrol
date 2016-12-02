/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.4
import QtQuick.Dialogs  1.1

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {
    id:     tcpLinkSettings
    width:  parent ? parent.width : 0
    height: tcpColumn.height

    function saveSettings() {
        if(subEditConfig) {
            subEditConfig.host = hostField.text
            subEditConfig.port = parseInt(portField.text)
        }
    }

    Column {
        id:         tcpColumn
        width:      tcpLinkSettings.width
        spacing:    ScreenTools.defaultFontPixelHeight / 2
        QGCLabel {
            id:     tcpLabel
            text:   qsTr("TCP Link Settings")
        }
        Rectangle {
            height: 1
            width:  tcpLabel.width
            color:  qgcPal.button
        }
        Item {
            height: ScreenTools.defaultFontPixelHeight / 2
            width:  parent.width
        }
        Row {
            spacing:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   qsTr("Host Address:")
                width:  _firstColumn
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCTextField {
                id:     hostField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcp ? subEditConfig.host : ""
                width:  _secondColumn
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        Row {
            spacing:    ScreenTools.defaultFontPixelWidth
            QGCLabel {
                text:   qsTr("TCP Port:")
                width:  _firstColumn
                anchors.verticalCenter: parent.verticalCenter
            }
            QGCTextField {
                id:     portField
                text:   subEditConfig && subEditConfig.linkType === LinkConfiguration.TypeTcp ? subEditConfig.port.toString() : ""
                width:  _firstColumn
                inputMethodHints:       Qt.ImhFormattedNumbersOnly
                anchors.verticalCenter: parent.verticalCenter
            }
        }
    }
}
