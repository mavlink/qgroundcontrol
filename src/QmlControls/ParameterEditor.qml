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
    viewPanel: panel

    /// true: show full information, false: for use in smaller widgets
    property bool fullMode: true

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }
    property Fact __editorDialogFact: Fact { }

    readonly property real __leftMargin: 10
    readonly property real __rightMargin: 20
    readonly property int __maxParamChars: 16

    ParameterEditorController {
        id: controller;
        factPanel: panel

        onShowErrorMessage: {
            showMessage("Parameter Load Errors", errorMsg, StandardButton.Ok)
        }
    }

    Component {
        id: editorDialogComponent

        ParameterEditorDialog { fact: __editorDialogFact }
    } // Component - Editor Dialog

    Component {
        id: factRowsComponent

        Column {
            id:     factColumn
            x:      __leftMargin

            QGCLabel {
                text:               group
                verticalAlignment:	Text.AlignVCenter
                font.pixelSize:     ScreenTools.mediumFontPixelSize
            }

            Rectangle {
                width:  parent.width
                height: 1
                color:  __qgcPal.text
            }

            Repeater {
                model: controller.getFactsForGroup(componentId, group)

                Column {
                    property Fact modelFact: controller.getParameterFact(componentId, modelData)

                    Item {
                        x:			__leftMargin
                        width:      parent.width
                        height:		ScreenTools.defaultFontPixelSize * 1.75

                        QGCLabel {
                            id:                 nameLabel
                            width:              defaultTextWidth * (__maxParamChars + 1)
                            height:             parent.height
                            verticalAlignment:	Text.AlignVCenter
                            text:               modelFact.name
                        }

                        QGCLabel {
                            id:                 valueLabel
                            width:              defaultTextWidth * 20
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
                                showDialog(editorDialogComponent, "Parameter Editor", fullMode ? 50 : -1, StandardButton.Cancel | StandardButton.Save)
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

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent


        Column {
            anchors.fill: parent

            Item {
                width:  parent.width
                height: toolsButton.height

                QGCLabel {
                    font.pixelSize: ScreenTools.largeFontPixelSize
                    visible:        fullMode
                    text:           "PARAMETER EDITOR"
                }

                QGCButton {
                    id:             toolsButton
                    anchors.right:  parent.right
                    text:           "Tools"

                    menu: Menu {
                        MenuItem {
                            text:           "Refresh"
                            onTriggered:	controller.refresh()
                        }
                        MenuItem {
                            text:           "Reset all to defaults"
                            onTriggered:	controller.resetAllToDefaults()
                        }
                        MenuSeparator { }
                        MenuItem {
                            text:           "Load from file"
                            visible:        fullMode
                            onTriggered:	controller.loadFromFile()
                        }
                        MenuItem {
                            text:           "Save to file"
                            visible:        fullMode
                            onTriggered:	controller.saveToFile()
                        }
                        MenuSeparator { }
                        MenuItem {
                            text:           "Clear RC to Param"
                            onTriggered:	controller.clearRCToParam()
                        }
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
                    width:	defaultTextWidth * 25
                    height: parent.height

                    Column {
                        Repeater {
                            model: controller.componentIds

                            Column {
                                id: componentColumn

                                readonly property int componentId: parseInt(modelData)

                                QGCLabel {
                                    height:				contentHeight + (ScreenTools.defaultFontPixelHeight * 0.5)
                                    text:               "Component #: " + componentId.toString()
                                    verticalAlignment:	Text.AlignVCenter
                                    font.pixelSize:     ScreenTools.mediumFontPixelSize
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
                                            height: ScreenTools.defaultFontPixelSize * 0.25
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
    } // QGCViewPanel
} // QGCView
