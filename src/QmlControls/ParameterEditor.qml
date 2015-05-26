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

/// @file
///     @author Don Gagne <don@thegagnes.com>

import QtQuick 2.3
import QtQuick.Controls 1.3
import QtQuick.Controls.Styles 1.2
import QtQuick.Dialogs 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

QGCView {
    viewComponent: parameterList

    /// true: show full information, false: for use in smaller widgets
    property bool fullMode: true

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }
    QGCLabel { id: __textMeasure; text: "X"; visible: false }
    property Fact __editorDialogFact: Fact { }


    readonly property real __leftMargin: 10
    readonly property real __rightMargin: 20
    readonly property int __maxParamChars: 16

    property real __textHeight: __textMeasure.contentHeight
    property real __textWidth:  __textMeasure.contentWidth

    Component {
        id: parameterList

        QGCViewPanel {
            id: panel

            ParameterEditorController { id: controller; factPanel: panel }

            Component {
                id: factRowsComponent

                Column {
                    id:     factColumn
                    x:      __leftMargin

                    QGCLabel {
                        height:				__textHeight + (ScreenTools.pixelSizeFactor * (9))
                        text:               group
                        verticalAlignment:	Text.AlignVCenter
                        font.pointSize:     ScreenTools.fontPointFactor * (16);
                    }

                    Rectangle {
                        width:  parent.width
                        height: 1
                        color:  __qgcPal.text
                    }

                    Repeater {
                        model: autopilot ? controller.getFactsForGroup(componentId, group) : 0

                        Column {
                            property Fact modelFact: controller.getParameterFact(componentId, modelData)

                            Item {
                                x:			__leftMargin
                                width:      parent.width
                                height:		__textHeight + (ScreenTools.pixelSizeFactor * (9))

                                QGCLabel {
                                    id:                 nameLabel
                                    width:              __textWidth * (__maxParamChars + 1)
                                    height:             parent.height
                                    verticalAlignment:	Text.AlignVCenter
                                    text:               modelFact.name
                                }

                                QGCLabel {
                                    id:                 valueLabel
                                    width:              __textWidth * 20
                                    height:             parent.height
                                    anchors.left:       nameLabel.right
                                    verticalAlignment:	Text.AlignVCenter
                                    color:              modelFact.valueEqualsDefault ? __qgcPal.text : "orange"
                                    text:               modelFact.valueString + " " + modelFact.units
                                }

                                QGCLabel {
                                    height:             parent.height
                                    anchors.left:       valueLabel.right
                                    verticalAlignment:	Text.AlignVCenter
                                    visible:            fullMode
                                    text:               modelFact.shortDescription
                                }

                                MouseArea {
                                    anchors.fill:       parent
                                     acceptedButtons:   Qt.LeftButton

                                    onClicked: {
                                        __editorDialogFact = modelFact
                                        panel.showDialog(editorDialogComponent, "Parameter Editor", fullMode ? 50 : -1, StandardButton.Cancel | StandardButton.Save)
                                    }
                                }
                            }

                            Rectangle {
                                x:      __leftMargin
                                width:  factColumn.width - __leftMargin - __rightMargin
                                height: 1
                                color:  __qgcPal.windowShade
                            }
                        } // Column - Fact
                    } // Repeater - Facts
                } // Column - Facts
            } // Component - factRowsComponent

            Column {
                anchors.fill: parent

                Item {
                    width:  parent.width
                    height: firstButton.height

                    QGCLabel {
                        font.pointSize: ScreenTools.fontPointFactor * (20)
                        visible:        fullMode
                        text:           "PARAMETER EDITOR"
                    }

                    Row {
                        spacing:            10
                        layoutDirection:    Qt.RightToLeft
                        width:              parent.width

                        QGCButton {
                            text:		"Clear RC to Param"
                            onClicked:	controller.clearRCToParam()
                        }
                        QGCButton {
                            text:		"Save to file"
                            visible:	fullMode
                            onClicked:	controller.saveToFile()
                        }
                        QGCButton {
                            text:		"Load from file"
                            visible:	fullMode
                            onClicked:	controller.loadFromFile()
                        }
                        QGCButton {
                            id:			firstButton
                            text:		"Refresh"
                            onClicked:	controller.refresh()
                        }
                    }
                }

                Item {
                    id:		lastSpacer
                    height: 10
                    width:	5
                }

                Item {
                    width:  parent.width
                    height: parent.height - (lastSpacer.y + lastSpacer.height)

                    ScrollView {
                        id :	groupScroll
                        width:	__textWidth * 25
                        height: parent.height

                        Column {
                            Repeater {
                                model: controller.componentIds

                                Column {
                                    id: componentColumn

                                    readonly property int componentId: parseInt(modelData)

                                    QGCLabel {
                                        height:				contentHeight + (ScreenTools.pixelSizeFactor * (9))
                                        text:               "Component #: " + componentId.toString()
                                        verticalAlignment:	Text.AlignVCenter
                                        font.pointSize:     ScreenTools.fontPointFactor * (16);
                                    }

                                    Repeater {
                                        model: controller.getGroupsForComponent(componentColumn.componentId)

                                        Column {
                                            QGCButton {
                                                x:		__leftMargin
                                                width: groupScroll.width - __leftMargin - __rightMargin
                                                text:	modelData

                                                onClicked: {
                                                    factRowsLoader.sourceComponent = null
                                                    factRowsLoader.componentId = componentId
                                                    factRowsLoader.group = modelData
                                                    factRowsLoader.sourceComponent = factRowsComponent
                                                }
                                            }

                                            Item {
                                                width:  1
                                                height: ScreenTools.pixelSizeFactor * (3)
                                            }
                                        } // Column - Group
                                    } // Repeater - Groups

                                    Item {
                                        height: 10
                                        width:	10
                                    }
                                } // Column - Component
                            } // Repeater - Components
                        } // Column - Component
                    } // ScrollView - Groups

                    ScrollView {
                        id:             factScrollView
                        anchors.left:   groupScroll.right
                        anchors.right:  parent.right
                        height:         parent.height

                        Loader {
                            id:     factRowsLoader
                            width:  factScrollView.width

                            property int componentId:   controller.componentIds[0]
                            property string group:      controller.getGroupsForComponent(controller.componentIds[0])[0]
                            sourceComponent:            factRowsComponent
                        }
                    } // ScrollView - Facts
                } // Item - Group ScrollView + Facts
            } // Column - Outer
        }
    } // Component - Parameter List

    Component {
        id: editorDialogComponent

        QGCViewDialog {
            id:             editorDialog

            ParameterEditorController { id: controller; factPanel: editorDialog }

            property bool fullMode: true

            function accept() {
                __editorDialogFact.value = valueField.text
                editorDialog.hideDialog()
            }

            Column {
                spacing:        __textHeight
                anchors.left:   parent.left
                anchors.right:  parent.right

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       __editorDialogFact.shortDescription ? __editorDialogFact.shortDescription : "Description missing"
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    visible:    __editorDialogFact.longDescription
                    text:       __editorDialogFact.longDescription
                }

                QGCTextField {
                    id:     valueField
                    text:   __editorDialogFact.valueString
                }

                QGCLabel { text: __editorDialogFact.name }

                Row {
                    spacing: __textWidth

                    QGCLabel { text: "Units:" }
                    QGCLabel { text: __editorDialogFact.units ? __editorDialogFact.units : "none" }
                }

                Row {
                    spacing: __textWidth

                    QGCLabel { text: "Minimum value:" }
                    QGCLabel { text: __editorDialogFact.min }
                }

                Row {
                    spacing: __textWidth

                    QGCLabel { text: "Maxmimum value:" }
                    QGCLabel { text: __editorDialogFact.max }
                }

                Row {
                    spacing: __textWidth

                    QGCLabel { text: "Default value:" }
                    QGCLabel { text: __editorDialogFact.defaultValueAvailable ? __editorDialogFact.defaultValue : "none" }
                }

                QGCLabel {
                    width:      parent.width
                    wrapMode:   Text.WordWrap
                    text:       "Warning: Modifying parameters while vehicle is in flight can lead to vehicle instability and possible vehicle loss. " +
                                    "Make sure you know what you are doing and double-check your values before Save!"
                }
            } // Column - Fact information


            QGCButton {
                anchors.rightMargin:    __textWidth
                anchors.right:          rcButton.left
                anchors.bottom:         parent.bottom
                visible:                __editorDialogFact.defaultValueAvailable
                text:                   "Reset to default"

                onClicked: {
                    __editorDialogFact.value = __editorDialogFact.defaultValue
                    editorDialog.hideDialog()
                }
            }

            QGCButton {
                id:             rcButton
                anchors.right:  parent.right
                anchors.bottom: parent.bottom
                visible:        __editorDialogFact.defaultValueAvailable
                text:           "Set RC to Param..."
                onClicked:      controller.setRCToParam(__editorDialogFact.name)
            }
        } // Rectangle - editorDialog
    } // Component - Editor Dialog
} // QGCView