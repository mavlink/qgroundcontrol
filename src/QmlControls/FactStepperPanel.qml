/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

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
        spacing: 8

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
            id: pidListView
            model: steppersFilteredModel
            focus: false
            clip: true
            interactive: false
            orientation: ListView.Vertical
            Layout.fillWidth: true
            spacing: 8

            Layout.minimumHeight: contentHeight

            property int maxNameTextWidth: 0
            onModelChanged: maxNameTextWidth = 0
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

        ColumnLayout {
            id: factEditor
            spacing: _thinMargins
            anchors.left: parent.left
            anchors.right: parent.right

            visible: fact

            property Fact fact

            Component.onCompleted: {
                if (param) {
                    fact = controller.getParameterFact(-1, param)
                }
            }

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

                    Layout.minimumWidth: pidListView.maxNameTextWidth

                    onTextChanged: {
                        pidListView.maxNameTextWidth = Math.max(width, pidListView.maxNameTextWidth)
                    }
                }

                FactStepper {
                    id: stepper
                    visible: fact
                    showHelp: false
                    showUnits: true

                    fact: factEditor.fact

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

            ColumnLayout {
                id: helpDetails
                visible: helpButton.checked
                Layout.fillWidth: true
                spacing: defaultTextHeight / 2

                QGCLabel {
                    Layout.fillWidth: true
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

                Rectangle {
                    Layout.fillWidth: true
                    Layout.minimumHeight: 1
                    Layout.maximumHeight: 1
                    color: qgcPal.windowShade
                }
            }
        }
    }
} // QGCView
