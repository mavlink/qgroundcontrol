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



    ColumnLayout {
        id: pidsView
        anchors.fill: parent

        GridLayout {
            Layout.fillWidth: true

            ToggleButton {
                id: includeAdvancedToggle
                text: qsTr("Advanced")
                checked: false
            }

            ToggleButton {
                id: includeStabilizedToggle
                text: qsTr("Stabilized")
                checked: true
            }

            ToggleButton {
                id: includeRollToggle
                text: qsTr("Roll")
                checked:  true
            }
            ToggleButton {
                id: includePitchToggle
                text: qsTr("Pitch")
                checked:  true
            }
            ToggleButton {
                id: includeYawToggle
                text: qsTr("Yaw")
                checked:  true
            }
            ToggleButton {
                id: includeTPAToggle
                text: qsTr("TPA")
                checked: false
            }
        }

        DelegateModel {
            id: steppersFilteredModel
            delegate: listItem
            model: steppersModel
            property bool loaded: false

            function filtersChanged() {
                if (!loaded) {
                    return
                }

                items.remove(0, items.count)

                for (var i = 0; i < steppersModel.count; i++) {
                    var entry = steppersModel.get(i)

                    if (entry.advanced && !includeAdvanced) continue
                    if (entry.stabilized && !includeStabilized) continue
                    if (entry.group === "roll" && !includeRoll) continue
                    if (entry.group === "pitch" && !includePitch) continue
                    if (entry.group === "yaw" && !includeYaw) continue
                    if (entry.group === "tpa" && !includeTPA) continue

                    items.insert(entry)
                }
            }

            property bool includeAdvanced: includeAdvancedToggle.checked
            property bool includeStabilized: includeStabilizedToggle.checked
            property bool includeRoll: includeRollToggle.checked
            property bool includePitch: includePitchToggle.checked
            property bool includeYaw: includeYawToggle.checked
            property bool includeTPA: includeTPAToggle.checked
            property bool lockPitchRoll: true

            onIncludeAdvancedChanged: filtersChanged()
            onIncludeStabilizedChanged: filtersChanged()
            onLockPitchRollChanged: filtersChanged()

            onIncludeRollChanged: filtersChanged()
            onIncludePitchChanged: filtersChanged()
            onIncludeYawChanged: filtersChanged()
            onIncludeTPAChanged: filtersChanged()

            onLoadedChanged: filtersChanged()
        }

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


        Component.onCompleted: {
            steppersFilteredModel.loaded = true
        }
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

                    ToggleButton {
                        id: helpButton
                        visible: fact

                        text: "?"
                        tooltip: fact ? fact.longDescription : ""

                        Layout.minimumWidth: stepper.height
                        Layout.minimumHeight: stepper.height
                    } // QGCButton helpButton
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
                    visible: helpButton.checked
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
        } // listItem
    }
} // QGCView
