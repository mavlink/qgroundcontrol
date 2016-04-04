/*=====================================================================

 QGroundControl Open Source Ground Control Station

 (c) 2009 - 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

 This file is part of the QGROUNDCONTROL project

 QGROUNDCONTROL is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 QGROUNDCONTROL is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

 ======================================================================*/

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
