/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    viewPanel: panel
    id: logwindow
    property bool loaded: false

    QGCPalette { id: qgcPal }


    Connections {
        target: debugMessageModel

        onDataChanged: {
            // Keep the view in sync if the button is checked
            if (loaded) {
                if (followTail.checked) {
                    listview.positionViewAtEnd();
                }
            }
        }
    }

    Component {
        id: delegateItem
        Rectangle {
            color:  index % 2 == 0 ? qgcPal.window : qgcPal.windowShade
            height: Math.round(ScreenTools.defaultFontPixelHeight * 0.5 + field.height)
            width:  listview.width

            Text {
                anchors.verticalCenter: parent.verticalCenter
                id:       field
                text:     display
                color:    qgcPal.text
                width:    parent.width
                wrapMode: Text.Wrap
            }
        }
    }

    QGCViewPanel {
        id:               panel
        anchors.fill:     parent

        Rectangle {
            anchors.fill: parent
            color: qgcPal.window
        }

        ListView {
            Component.onCompleted: {
                loaded = true
            }
            anchors.margins: ScreenTools.defaultFontPixelHeight
            anchors.top:     parent.top
            anchors.left:    parent.left
            anchors.right:   parent.right
            anchors.bottom:  followTail.top
            id:              listview
            model:           debugMessageModel
            delegate:        delegateItem
        }

        QGCButton {
            id:  followTail
            anchors.bottom:  parent.bottom
            anchors.right:   parent.right
            anchors.margins: ScreenTools.defaultFontPixelWidth
            text:            "Show Latest"
            checkable:       true
            checked:         true

            onCheckedChanged: {
                if (checked && loaded) {
                    listview.positionViewAtEnd();
                }
            }
        }
    }
}
