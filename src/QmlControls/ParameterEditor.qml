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
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0
import QGroundControl.Controllers 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0

Rectangle {
	/// true: show full information, false: for use in smaller widgets
	property bool fullMode: true

	QGCPalette { id: __qgcPal; colorGroupEnabled: true }
	ScreenTools { id: __screenTools }
	ParameterEditorController { id: __controller }
	QGCLabel { id: __charWidth; text: "X"; visible: false }

	readonly property real __leftMargin: 10
	readonly property real __rightMargin: 20
	readonly property int __maxParamChars: 16

    color: __qgcPal.window

    // We use an ExclusiveGroup to maintain the visibility of a single editing control at a time
    ExclusiveGroup {
        id: __exclusiveEditorGroup
    }

    Column {
        anchors.fill:parent

        Row {
            spacing:            10
            layoutDirection:    Qt.RightToLeft
            width:              parent.width

            QGCButton {
                text:		"Clear RC to Param"
                onClicked:	__controller.clearRCToParam()
            }
            QGCButton {
                text:		"Save to file"
				visible:	fullMode
                onClicked:	__controller.saveToFile()
            }
            QGCButton {
                text:		"Load from file"
				visible:	fullMode
                onClicked:	__controller.loadFromFile()
            }
            QGCButton {
                id:			firstButton
                text:		"Refresh"
                onClicked:	__controller.refresh()
            }
        }

        Item {
            id:		lastSpacer
            height: 10
            width:	5
        }

        ScrollView {
            id :	scrollView
            width:	parent.width
            height: parent.height - (lastSpacer.y + lastSpacer.height)

            Column {
                Repeater {
                    model: __controller.componentIds

                    Column {
						id: componentColumn

						property int componentId: parseInt(modelData)

						QGCLabel {
							text: "Component #: " + componentId.toString()
							font.pointSize: __screenTools.dpiAdjustedPointSize(__qgcPal.defaultFontPointSize + 4);
						}

                        Item {
                            height: 10
                            width:	10
                        }

                        Repeater {
                            model: __controller.getGroupsForComponent(componentColumn.componentId)

                            Column {
                                Rectangle {
                                    id: groupRect
                                    color:	__qgcPal.windowShade
                                    height: groupBlock.height
                                    width:	scrollView.viewport.width - __rightMargin

                                    Column {
                                        id: groupBlock

                                        Rectangle {
                                            color:	__qgcPal.windowShadeDark
                                            height: groupLabel.height
                                            width:	groupRect.width

                                            QGCLabel {
                                                id:					groupLabel
                                                height:				contentHeight + 5
                                                x:					__leftMargin
                                                text:				modelData
                                                verticalAlignment:	Text.AlignVCenter
                                                font.pointSize:		__screenTools.dpiAdjustedPointSize(__qgcPal.defaultFontPointSize + 2);
                                            }
                                        }

                                        Repeater {
                                            model: __controller.getFactsForGroup(componentColumn.componentId, modelData)

                                            Row {
                                                spacing:	10
                                                x:			__leftMargin

												Fact { id: modelFact; name: modelData + ":" + componentColumn.componentId }

                                                QGCLabel {
                                                    text:	modelFact.name
                                                    width:	__charWidth.contentWidth * (__maxParamChars + 2)
                                                }

                                                QGCLabel {

                                                    text:   modelFact.valueString + " " + modelFact.units
                                                    width:  __charWidth.contentWidth * 20
                                                    height: contentHeight

                                                    MouseArea {
                                                        anchors.fill:		parent
														acceptedButtons:	Qt.LeftButton | Qt.RightButton

                                                        onClicked: {
															if (mouse.button == Qt.LeftButton) {
																editor.checked = true
																editor.focus = true
															} else if (mouse.button == Qt.RightButton) {
																__controller.setRCToParam(modelData)
															}
                                                        }
                                                    }

                                                    FactTextField {
                                                        id:                 editor
                                                        y:                  (parent.height - height) / 2
                                                        width:              parent.width
                                                        visible:            checked
                                                        focus:              true
                                                        fact:               modelFact
                                                        showUnits:          true
                                                        onEditingFinished:  checked = false

                                                        // We use an ExclusiveGroup to manage visibility
                                                        property bool checked: false
                                                        property ExclusiveGroup exclusiveGroup: __exclusiveEditorGroup
                                                        onExclusiveGroupChanged: {
                                                            if (exclusiveGroup)
                                                                exclusiveGroup.bindCheckable(editor)
                                                        }
                                                    }
                                                }

                                                QGCLabel {
                                                    text: modelFact.shortDescription
													visible: fullMode
                                                }
                                            } // Row - Fact value
                                        } // Repeater - Facts
                                    } // Column - Fact rows
                                } // Rectangle - Group

                                Item {
                                    height: 10
                                    width:	10
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
        } // ScrollView
    } // Column - Outer
}
