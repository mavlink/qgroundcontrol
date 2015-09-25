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

    QGCPalette { id: __qgcPal; colorGroupEnabled: true }
    property Fact __editorDialogFact: Fact { }

    readonly property real __leftMargin: 10
    readonly property real __rightMargin: 20
    readonly property int __maxParamChars: 16

    property bool _searchFilter: false  ///< true: showing results of search
    property var _searchResults         ///< List of parameter names from search results

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
        id: searchDialogComponent

        QGCViewDialog {

            function accept() {
                _searchResults = controller.searchParametersForComponent(-1, searchFor.text, searchInName.checked, searchInDescriptions.checked)
                _searchFilter = true
                hideDialog()
            }

            function reject() {
                _searchFilter = false
                hideDialog()
            }

            QGCLabel {
                id:     searchForLabel
                text:   "Search for:"
            }

            QGCTextField {
                id:                 searchFor
                anchors.topMargin:  defaultTextHeight / 3
                anchors.top:        searchForLabel.bottom
                width:              defaultTextWidth * 20
            }

            QGCLabel {
                id:                 searchInLabel
                anchors.topMargin:  defaultTextHeight
                anchors.top:        searchFor.bottom
                text:               "Search in:"
            }

            QGCCheckBox {
                id:                 searchInName
                anchors.topMargin:  defaultTextHeight / 3
                anchors.top:        searchInLabel.bottom
                text:               "Name"
            }

            QGCCheckBox {
                id:                 searchInDescriptions
                anchors.topMargin:  defaultTextHeight / 3
                anchors.top:        searchInName.bottom
                text:               "Descriptions"
            }

            QGCLabel {
                anchors.topMargin:  defaultTextHeight
                anchors.top:        searchInDescriptions.bottom
                width:              parent.width
                wrapMode:           Text.WordWrap
                text:               "Hint: Leave 'Search For' blank and click Apply to list all parameters sorted by name."
            }
        }
    }

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
                model: parameterNames

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
                            text:               modelFact.shortDescription
                        }

                        MouseArea {
                            anchors.fill:       parent
                             acceptedButtons:   Qt.LeftButton

                            onClicked: {
                                __editorDialogFact = modelFact
                                showDialog(editorDialogComponent, "Parameter Editor", 50, StandardButton.Cancel | StandardButton.Save)
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

    Component {
        id: groupedViewComponent

        Item {
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
                    id:                 factRowsLoader
                    width:              factScrollView.width
                    sourceComponent:    factRowsComponent

                    property int componentId:       controller.componentIds[0]
                    property string group:          controller.getGroupsForComponent(controller.componentIds[0])[0]
                    property var parameterNames:    controller.getParametersForGroup(componentId, group)
                }
            } // ScrollView - Facts
        } // Item
    } // Component - groupedViewComponent

    Component {
        id: searchResultsViewComponent

        Item {
            ScrollView {
                id:             factScrollView
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         parent.height

                Loader {
                    id:                 factRowsLoader
                    width:              factScrollView.width
                    sourceComponent:    factRowsComponent

                    property int componentId:       -1
                    property string group:          "Search results"
                    property var parameterNames:    _searchResults
                }
            } // ScrollView - Facts
        } // Item
    } // Component - sortedViewComponent

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent


        Column {
            anchors.fill: parent

            Item {
                width:  parent.width
                height: toolsButton.height

                QGCLabel {
                    id:             titleText
                    font.pixelSize: ScreenTools.largeFontPixelSize
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
                        MenuItem {
                            text:           "Search..."
                            onTriggered:    showDialog(searchDialogComponent, "Parameter Search", 50, StandardButton.Reset | StandardButton.Apply)
                        }
                        MenuSeparator { }
                        MenuItem {
                            text:           "Load from file..."
                            onTriggered:	controller.loadFromFile()
                        }
                        MenuItem {
                            text:           "Save to file..."
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

            Loader {
                width:              parent.width
                height:             parent.height - (lastSpacer.y + lastSpacer.height)
                sourceComponent:    _searchFilter ? searchResultsViewComponent: groupedViewComponent
            }
        } // Column - Outer
    } // QGCViewPanel
} // QGCView
