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
	QGCPalette { id: qgcPal; colorGroupEnabled: true }
	ScreenTools { id: screenTools }
	ParameterEditorController { id: controller }
	QGCLabel { id: charWidth; text: "X"; visible: false }

	readonly property real leftMargin: 10
	readonly property real rightMargin: 20
	readonly property int maxParamChars: 16

    color: qgcPal.window

    // We use an ExclusiveGroup to maintain the visibility of a single editing control at a time
    ExclusiveGroup {
        id: exclusiveEditorGroup
    }

    Column {
        anchors.fill:parent

        QGCLabel {
            text: "PARAMETER EDITOR"
            font.pointSize: screenTools.dpiAdjustedPointSize(20)
        }

        Item {
            height: 20
            width:	5
        }

        Row {
            spacing:            10
            layoutDirection:    Qt.RightToLeft
            width:              parent.width

            QGCButton {
                text: "Clear RC to Param"
                onClicked: controller.clearRCToParam()
            }
            QGCButton {
                text: "Save to file"
                onClicked: controller.saveToFile()
            }
            QGCButton {
                text: "Load from file"
                onClicked: controller.loadFromFile()
            }
            QGCButton {
                id: firstButton
                text: "Refresh"
                onClicked: controller.refresh()
            }
            QGCLabel {
                width:      firstButton.x - parent.spacing
                wrapMode:   Text.WordWrap
                text:       "Click a parameter value to modify. Right-click to set an RC to Param mapping. Use caution when modifying parameters here since the values are not checked for validity."
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
                    model: controller.componentIds

                    Column {
						id: componentColumn

						property int componentId: parseInt(modelData)

						QGCLabel {
							text: "Component #: " + componentId.toString()
							font.pointSize: screenTools.dpiAdjustedPointSize(qgcPal.defaultFontPointSize + 4);
						}

                        Item {
                            height: 10
                            width:	10
                        }

                        Repeater {
                            model: controller.getGroupsForComponent(componentColumn.componentId)

                            Column {
                                Rectangle {
                                    id: groupRect
                                    color:	qgcPal.windowShade
                                    height: groupBlock.height
                                    width:	scrollView.viewport.width - rightMargin

                                    Column {
                                        id: groupBlock

                                        Rectangle {
                                            color:	qgcPal.windowShadeDark
                                            height: groupLabel.height
                                            width:	groupRect.width

                                            QGCLabel {
                                                id:					groupLabel
                                                height:				contentHeight + 5
                                                x:					leftMargin
                                                text:				modelData
                                                verticalAlignment:	Text.AlignVCenter
                                                font.pointSize:		screenTools.dpiAdjustedPointSize(qgcPal.defaultFontPointSize + 2);
                                            }
                                        }

                                        Repeater {
                                            model: controller.getFactsForGroup(componentColumn.componentId, modelData)

                                            Row {
                                                spacing:	10
                                                x:			leftMargin

												Fact { id: modelFact; name: modelData + ":" + componentColumn.componentId }

                                                QGCLabel {
                                                    text:	modelFact.name
                                                    width:	charWidth.contentWidth * (maxParamChars + 2)
                                                }

                                                QGCLabel {

                                                    text:   modelFact.valueString + " " + modelFact.units
                                                    width:  charWidth.contentWidth * 20
                                                    height: contentHeight

                                                    MouseArea {
                                                        anchors.fill:		parent
														acceptedButtons:	Qt.LeftButton | Qt.RightButton

                                                        onClicked: {
															if (mouse.button == Qt.LeftButton) {
																editor.checked = true
																editor.focus = true
															} else if (mouse.button == Qt.RightButton) {
																controller.setRCToParam(modelData)
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
                                                        property ExclusiveGroup exclusiveGroup: exclusiveEditorGroup
                                                        onExclusiveGroupChanged: {
                                                            if (exclusiveGroup)
                                                                exclusiveGroup.bindCheckable(editor)
                                                        }
                                                    }
                                                }

                                                QGCLabel {
                                                    text: modelFact.shortDescription
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
