/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Dialogs  1.2
import QtQuick.Layouts      1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0

AnalyzePage {
    id:                 mavlinkConsolePage
    pageComponent:      pageComponent
    pageName:           qsTr("Mavlink Console")
    pageDescription:    qsTr("Mavlink Console provides a connection to the vehicle's system shell.")

    Component {
        id: pageComponent

        ColumnLayout {
            id:     consoleColumn
            height: availableHeight
            width:  availableWidth

            TextArea {
                id:                consoleEditor
                Layout.fillHeight: true
                anchors.left:      parent.left
                anchors.right:     parent.right
                font.family:       ScreenTools.fixedFontFamily
                font.pointSize:    ScreenTools.defaultFontPointSize
                readOnly:          true

                cursorPosition:    conController.cursor
                text:              conController.text
            }

            QGCTextField {
                id:              command
                anchors.left:    parent.left
                anchors.right:   parent.right
                placeholderText: "Enter Commands here..."

                onAccepted: {
                    conController.sendCommand(text)
                    text = ""
                }
            }
        }
    } // Component
} // AnalyzePage
