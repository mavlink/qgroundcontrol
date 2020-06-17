/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

Item {

    property var    _activeVehicle: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable ? QGroundControl.multiVehicleManager.activeVehicle : null
    property real   _margins:       ScreenTools.defaultFontPixelHeight
    property string _noVehicleText: qsTr("No vehicle connected")
    property string _assignQmlFile: "<p>" +
        "You can create your own commands and parameter editing user interface in this widget. " +
        "You do this by providing your own Qml file. " +
        "This support is a work in progress and the details may change somewhat in the future. " +
        "By using this feature you are connecting directly to the internals of QGroundControl. " +
        "Doing so incorrectly may cause instability both in QGroundControl and/or your vehicle. " +
        "So make sure to test your changes thoroughly before using them in flight.</p>" +
        "<p>Click 'Load Custom Qml file' to provide your custom qml file.</p>" +
        "<p>Click 'Reset' to reset to none.</p>" +
        "<p>Example usage: <a href='https://docs.qgroundcontrol.com/en/app_menu/custom_command_widget.html'>https://docs.qgroundcontrol.com/en/app_menu/custom_command_widget.html</a></p>"

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    CustomCommandWidgetController {
        id:         controller
        onCustomQmlFileChanged: _updateLoader()
    }

    Component.onCompleted: _updateLoader()

    on_ActiveVehicleChanged: _updateLoader()

    function _updateLoader() {
        loader.sourceComponent = undefined
        loader.visible = false
        textOutput.text = _noVehicleText
        if (_activeVehicle) {
            if (controller.customQmlFile === "") {
                textOutput.text = _assignQmlFile
            } else {
                loader.source = controller.customQmlFile
            }
        }
    }

    Item {
        anchors.fill:   parent

        Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window

            Loader {
                id:                 loader
                anchors.margins:    _margins
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        parent.top
                anchors.bottom:     buttonRow.top
                visible:            false

                onStatusChanged: {
                    if (loader.status == Loader.Error) {
                        textOutput.text = sourceComponent.errorString()
                    } else if (loader.status == Loader.Ready) {
                        loader.visible = true
                    }
                }
            }
            QGCFlickable {
                id: container
                anchors.fill:       loader
                contentHeight:      textOutput.height
                flickableDirection: QGCFlickable.VerticalFlick
                visible:            !loader.visible
                QGCLabel {
                    id:                 textOutput
                    width:              container.width
                    wrapMode:           Text.WordWrap
                    textFormat:         Text.RichText
                    onLinkActivated:    Qt.openUrlExternally(link)
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
                    onClicked:  controller.clearQmlFile()
                }
            }

        }
    }
}
