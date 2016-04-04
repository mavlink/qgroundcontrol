/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

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

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick                  2.5
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

QGCView {
    viewPanel:  panel

    property real   _margins:    ScreenTools.defaultFontPixelHeight
    property string _emptyText:  "<p>" +
        "You can create your own commands and parameter editing user interface in this widget. " +
        "You do this by providing your own Qml file. " +
        "This support is a work in progress and the details may change somewhat in the future. " +
        "By using this feature you are connecting directly to the internals of QGroundControl. " +
        "Doing so incorrectly may cause instability both in QGroundControl and/or your vehicle. " +
        "So make sure to test your changes thoroughly before using them in flight.</p>" +
        "<p>Click 'Load Custom Qml file' to provide your custom qml file.</p>" +
        "<p>Click 'Reset' to reset to none.</p>" +
        "<p>Example usage: http://www.qgroundcontrol.org/custom_command_qml_widgets</p>"

    QGCPalette                      { id: qgcPal; colorGroupEnabled: enabled }
    CustomCommandWidgetController   { id: controller; factPanel: panel }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            QGCLabel {
                id:                 textOutput
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     buttonRow.top
                wrapMode:           Text.WordWrap
                textFormat:         Text.RichText
                text:               _emptyText
                visible:            !loader.visible
            }
            Loader {
                id:                 loader
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     buttonRow.top
                source:             controller.customQmlFile
                visible:            false
                onStatusChanged: {
                    loader.visible = true
                    if (loader.status == Loader.Error) {
                        if (sourceComponent.status == Component.Error) {
                            textOutput.text = sourceComponent.errorString()
                            loader.visible = false
                        }
                    }
                }
            }
            Row {
                id:                 buttonRow
                spacing:            ScreenTools.defaultFontPixelWidth
                anchors.margins:    _margins
                anchors.bottom:     parent.bottom
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    text:       qsTr("Load Custom Qml file...")
                    width:      ScreenTools.defaultFontPixelWidth * 22
                    onClicked:  controller.selectQmlFile()
                }
                QGCButton {
                    text:       qsTr("Reset")
                    width:      ScreenTools.defaultFontPixelWidth * 22
                    onClicked: {
                        controller.clearQmlFile()
                        loader.visible  = false
                        textOutput.text = _emptyText
                    }
                }
            }
        }
	}
}
