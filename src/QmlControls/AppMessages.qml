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

Rectangle {
    id:              logwindow
    anchors.fill:    parent
    anchors.margins: ScreenTools.defaultFontPixelWidth
    color:           qgcPal.window

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

    ListView {
        Component.onCompleted: {
            loaded = true
        }
        anchors.top:     parent.top
        anchors.left:    parent.left
        anchors.right:   parent.right
        anchors.bottom:  followTail.top
        anchors.bottomMargin: ScreenTools.defaultFontPixelWidth
        clip:            true
        id:              listview
        model:           debugMessageModel
        delegate:        delegateItem
    }

    FileDialog {
        id:             writeDialog
        folder:         shortcuts.home
        nameFilters:    ["Log files (*.txt)", "All Files (*)"]
        selectExisting: false
        title:          "Select log save file"
        onAccepted: {
            debugMessageModel.writeMessages(fileUrl);
            visible = false;
        }
        onRejected:     visible = false
    }

    Connections {
        target:          debugMessageModel
        onWriteStarted:  writeButton.enabled = false;
        onWriteFinished: writeButton.enabled = true;
    }

    QGCButton {
        id:              writeButton
        anchors.bottom:  parent.bottom
        anchors.left:    parent.left
        onClicked:       writeDialog.visible = true
        text:            "Save App Log"
    }

    BusyIndicator {
        id:              writeBusy
        anchors.bottom:  writeButton.bottom
        anchors.left:    writeButton.right
        height:          writeButton.height
        visible:        !writeButton.enabled
    }

    QGCButton {
        id:              followTail
        anchors.bottom:  parent.bottom
        anchors.right:   parent.right
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
