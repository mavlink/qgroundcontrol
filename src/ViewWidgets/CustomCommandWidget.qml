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

import QtQuick 2.2

import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Controllers 1.0

ViewWidget {
	connectedComponent: commandComponenet

	Component {
		id: commandComponenet

        Item {
            id: bogusFactPanel

            // We aren't really using the controller in a FactPanel for this usage so we
            // pass in a bogus item to keep it from getting upset.
            CustomCommandWidgetController { id: controller; factPanel: bogusFactPanel }

            Item {
                anchors.top:    parent.top
                anchors.bottom: buttonRow.top
                width:          parent.width

                QGCLabel {
                    id:             errorOutput
                    anchors.fill:   parent
                    wrapMode:       Text.WordWrap
                    visible:        false
                }

                QGCLabel {
                    id:             warning
                    anchors.fill:   parent
                    wrapMode:       Text.WordWrap
                    visible:        !controller.customQmlFile
                    text:           "You can create your own commands and parameter editing user interface in this widget. " +
                                        "You do this by providing your own Qml file. " +
                                        "This support is a work in progress and the details may change somewhat in the future. " +
                                        "By using this feature you are connecting directly to the internals of QGroundControl. " +
                                        "Doing so incorrectly may cause instability both in QGroundControl and/or your vehicle. " +
                                        "So make sure to test your changes thoroughly before using them in flight.\n\n" +
                                        "Click 'Select Qml file' to provide your custom qml file.\n" +
                                        "Click 'Clear Qml file' to reset to none.\n" +
                                        "Example usage: http://www.qgroundcontrol.org/custom_command_qml_widgets"
                }

                Loader {
                    id: loader
                    anchors.fill:   parent
                    source:         controller.customQmlFile
                    visible:        controller.customQmlFile

                    onStatusChanged: {
                        if (loader.status == Loader.Error) {
                            if (sourceComponent.status == Component.Error) {
                                errorOutput.text = sourceComponent.errorString()
                                errorOutput.visible = true
                                loader.visible = false
                            }
                        }
                    }
                }
            }

            Row {
                id:             buttonRow
                spacing:        10
                anchors.bottom: parent.bottom

                QGCButton {
                    text:       "Select Qml file..."
                    onClicked:  controller.selectQmlFile()
                }

                QGCButton {
                    text:       "Clear Qml file"

                    onClicked: {
                        errorOutput.visible = false
                        controller.clearQmlFile()
                    }
                }
            }
        }
	}
}
