/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


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
