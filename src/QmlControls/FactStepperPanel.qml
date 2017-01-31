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
import QtQuick 2.5
import QtQuick.Controls 1.4
import QtQuick.Layouts 1.2
import QtQuick.Controls.Styles 1.2
import QtQuick.Controls.Private 1.0

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0
import QGroundControl.ScreenTools 1.0

QGCView {
    /// ListModel must contains elements which look like this:
    ///     ListElement {
    ///         title:          "Roll sensitivity"
    ///         param:          "MC_ROLL_TC"
    ///         group:          "Roll"
    ///     }
    property ListModel steppersModel

    property alias contentHeight: pidGroupsListView.contentHeight

    property var qgcViewPanel
    FactPanelController {
        id: controller
        factPanel: qgcViewPanel
    }

    QGCPalette {
        id: palette
        colorGroupEnabled: enabled
    }
    property real _margins: ScreenTools.defaultFontPixelHeight
    property real _thinMargins: ScreenTools.smallFontPointSize

    ListView {
        id: pidGroupsListView
        model: steppersModel
        focus: false
        interactive: false
        orientation: ListView.Vertical
        anchors.margins: _margins
        anchors.fill: parent
        spacing: 5

        section {
            property: "group"
            delegate: QGCLabel {
                id: groupTitleLabel
                lineHeight: 1.3
                lineHeightMode: Text.ProportionalHeight
                verticalAlignment: Text.AlignVCenter
                text: qsTr(section)
                font.pointSize: ScreenTools.mediumFontPointSize
            }
        }

        delegate: Rectangle {
            visible: fact
            id: stepperRect
            anchors.left: parent.left
            anchors.right: parent.right
            height: cellColumn.height + _thinMargins * 2
            color: palette.windowShade

            property Fact fact

            Component.onCompleted: {
                if (param) {
                    fact = controller.getParameterFact(-1, param)
                }
            }

            ColumnLayout {
                id: cellColumn
                spacing: _thinMargins
                anchors.margins: _thinMargins
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top

                property alias labelWidth: nameLabel.width

                RowLayout {
                    id: stepperRow
                    spacing: _margins
                    anchors.left: parent.left

                    QGCLabel {
                        id: nameLabel
                        text: fact && fact.shortDescription ? fact.shortDescription : ""
                        wrapMode: Text.WordWrap
                        horizontalAlignment: Text.AlignRight
                        verticalAlignment: Text.AlignVCenter

                        Layout.minimumWidth: stepper.width
                        Layout.maximumWidth: stepper.width
                    }

                    FactStepper {
                        id: stepper
                        visible: fact
                        showHelp: false
                        showUnits: true

                        fact: stepperRect.fact

                        onValidationError: {
                            helpDetails.visible = true
                        }
                    }

                    QGCButton {
                        id: _tooltipButton
                        visible: fact

                        text: "?"
                        tooltip: fact ? fact.longDescription : ""

                        Layout.minimumWidth: stepper.height
                        Layout.maximumWidth: stepper.height
                        Layout.minimumHeight: stepper.height
                        Layout.maximumHeight: stepper.height

                        onClicked: {
                            helpDetails.visible = !helpDetails.visible
                        }
                    } // QGCButton _tooltipButton
                }


                Column {
                    id: helpDetails
                    visible: false
                    Layout.fillWidth: true
                    spacing:        defaultTextHeight / 2

                    QGCLabel {
                        width:      parent.width
                        wrapMode:   Text.WordWrap
                        visible:    fact.longDescription
                        text:       fact.longDescription
                    }

                    Row {
                        spacing: defaultTextWidth
                        visible: !fact.minIsDefaultForType

                        QGCLabel { text: qsTr("Minimum value:") }
                        QGCLabel { text: fact.minString }
                    }

                    Row {
                        spacing: defaultTextWidth
                        visible: !fact.maxIsDefaultForType

                        QGCLabel { text: qsTr("Maximum value:") }
                        QGCLabel { text: fact.maxString }
                    }

                    Row {
                        spacing: defaultTextWidth

                        QGCLabel { text: qsTr("Parameter:") }
                        QGCLabel { text: param }
                    }
                }
            }
        }
    } // QGCListView
} // QGCView
