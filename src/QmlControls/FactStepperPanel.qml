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
import QtQml.Models 2.2

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

    property alias contentHeight: pidsView.implicitHeight

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

    DelegateModel {
        id: steppersFilteredModel
        delegate: listItem
        model: steppersModel
        groups: [
            DelegateModelGroup {
                includeByDefault: false
                name: "simple"
            },
            DelegateModelGroup {
                includeByDefault: true
                name: "advanced"
            },
            DelegateModelGroup {
                includeByDefault: false
                name: "stabilized"
            },
            DelegateModelGroup {
                includeByDefault: false
                name: "acro"
            }
        ]
        filterOnGroup: "advanced"

        Component.onCompleted: {
            var rowCount = steppersModel.count
            items.remove(0, rowCount)
            for (var i = 0; i < rowCount; i++) {
                var entry = steppersModel.get(i)
                if (typeof entry.advanced === "undefined" || !entry.advanced) {
                    items.insert(entry, "simple")
                } else {
                    items.insert(entry, "advanced")
                }

                if (typeof entry.acro === "undefined" || !entry.acro) {
                    items.insert(entry, "stabilized")
                } else {
                    items.insert(entry, "acro")
                }
            }
        }
    }


    ColumnLayout {
        id: pidsView
        anchors.fill: parent
        ListView {
            id: pidGroupsListView
            model: steppersFilteredModel
            focus: false
            clip: true
            interactive: false
            orientation: ListView.Vertical
            Layout.fillWidth: true
            spacing: 5

            Layout.minimumHeight: contentHeight
        } // QGCListView

//        RowLayout {
//            Layout.fillWidth: true

//            QGCButton {
//                text:       qsTr("Advanced")
//                onClicked:  steppersFilteredModel.filterOnGroup = "advanced"
//            }
//            QGCButton {
//                text:       qsTr("Simple")
//                onClicked:  steppersFilteredModel.filterOnGroup = "simple"
//            }
//            QGCButton {
//                text:       qsTr("Acro")
//                onClicked:  steppersFilteredModel.filterOnGroup = "acro"
//            }
//            QGCButton {
//                text:       qsTr("Stabilized")
//                onClicked:  steppersFilteredModel.filterOnGroup = "stabilized"
//            }
//        }

//        ListView {
//            id: pidGroupsListView
//            model: steppersFilteredModel
//            focus: false
//            interactive: false
//            orientation: ListView.Vertical
////            anchors.fill: parent
//            Layout.fillWidth: true
//            spacing: 5

//        } // QGCListView
    }

    Component {
        id: listSection

        QGCLabel {
            id: groupTitleLabel
            lineHeight: 1.3
            lineHeightMode: Text.ProportionalHeight
            verticalAlignment: Text.AlignVCenter
            text: qsTr(section)
            font.pointSize: ScreenTools.mediumFontPointSize
        }
    } // listSection

    Component {
        id: listItem

        Rectangle {
            visible: fact
            id: stepperRect
            anchors.left: parent.left
            anchors.right: parent.right
            height: cellColumn.height + _thinMargins * 2
            color: qgcPal.windowShade

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

                        onValidationError: validationErrorLabel.text = errorText
                        onValueChanged: validationErrorLabel.text = ""
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
                            helpDetails.toggleVisible()
                        }
                    } // QGCButton _tooltipButton
                }


                QGCLabel {
                    id: validationErrorLabel
                    Layout.fillWidth: true
                    wrapMode: Text.WordWrap
                    visible: text !== ""
                    color: qgcPal.warningText
                }

                Column {
                    id: helpDetails
                    visible: false
                    Layout.fillWidth: true
                    spacing:        defaultTextHeight / 2

                    function toggleVisible(forceShow) {
                        forceShow = typeof forceShow !== 'undefined' ? forceShow : false

                        visible = fact && (!visible || forceShow)
                    }

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
        } // listItem
    }
} // QGCView
