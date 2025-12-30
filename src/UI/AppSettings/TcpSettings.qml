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

GridLayout {
    columns:        2
    rowSpacing:     _rowSpacing
    columnSpacing:  _colSpacing

    function saveSettings() {
        subEditConfig.host = hostField.text.trim()
        subEditConfig.port = parseInt(portField.text)
    }

    QGCLabel { text: qsTr("Server Address") }
    QGCTextField {
        id:                     hostField
        Layout.preferredWidth:  _secondColumnWidth
        text:                   subEditConfig.host
        placeholderText:        qsTr("localhost or 192.168.1.1")

        // Allow IPv4, IPv6, hostnames, and domain names
        // This is permissive - actual validation happens at connection time
        validator: RegularExpressionValidator {
            // Allow alphanumeric, dots, colons, hyphens, and brackets (for IPv6)
            regularExpression: /^[a-zA-Z0-9\.\-:\[\]]+$/
        }
    }

    QGCLabel { text: qsTr("Port") }
    QGCTextField {
        id:                     portField
        Layout.preferredWidth:  _secondColumnWidth
        text:                   subEditConfig.port.toString()
        inputMethodHints:       Qt.ImhFormattedNumbersOnly
        placeholderText:        qsTr("5760")

        validator: IntValidator {
            bottom: 1
            top: 65535
        }
    }

    // Help text
    QGCLabel {
        Layout.columnSpan:      2
        Layout.preferredWidth:  _secondColumnWidth
        Layout.fillWidth:       true
        font.pointSize:         ScreenTools.smallFontPointSize
        wrapMode:               Text.WordWrap
        text:                   qsTr("You can enter an IP address (e.g. 192.168.1.1) or hostname (e.g. my-drone.local)")
        color:                  qgcPal.text
    }
}
