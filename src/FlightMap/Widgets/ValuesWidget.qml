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

import QtQuick          2.4
import QtQuick.Dialogs  1.2
import QtQuick.Layouts  1.2

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl               1.0

QGCFlickable {
    id:                 _root
    height:             Math.min(maxHeight, _smallFlow.y + _smallFlow.height)
    contentHeight:      _smallFlow.y + _smallFlow.height
    flickableDirection: Flickable.VerticalFlick
    clip:               true

    property var    qgcView
    property color  textColor
    property var    maxHeight

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle ? QGroundControl.multiVehicleManager.activeVehicle : QGroundControl.multiVehicleManager.disconnectedVehicle
    property real   _margins:       ScreenTools.defaultFontPixelWidth / 2

    QGCPalette { id:qgcPal; colorGroupEnabled: true }

    ValuesWidgetController {
        id: controller
    }

    function showPicker() {
        qgcView.showDialog(propertyPicker, qsTr("Value Widget Setup"), qgcView.showDialogDefaultWidth, StandardButton.Ok, true /*use available height*/ )
    }

    function listContains(list, value) {
        for (var i=0; i<list.length; i++) {
            if (list[i] === value) {
                return true
            }
        }
        return false
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      showPicker()
    }

    Column {
        id:         _largeColumn
        width:      parent.width
        spacing:    _margins

        Repeater {
            model: _activeVehicle ? controller.largeValues : 0

            Column {
                id:     valueColumn
                width:  _largeColumn.width

                property Fact fact: _activeVehicle.getFact(modelData.replace("Vehicle.", ""))
                property bool largeValue: _root.listContains(controller.altitudeProperties, fact.name)

                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    color:                  textColor
                    fontSizeMode:           Text.HorizontalFit
                    text:                   fact.shortDescription + (fact.units ? " (" + fact.units + ")" : "")
                }
                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    font.pixelSize:         ScreenTools.largeFontPixelSize * (largeValue ? 1.3 : 1.0)
                    font.weight:            largeValue ? Font.ExtraBold : Font.Normal
                    fontSizeMode:           Text.HorizontalFit
                    color:                  textColor
                    text:                   fact.valueString
                }
            }
        } // Repeater - Large
    } // Column - Large

    Flow {
        id:                 _smallFlow
        width:              parent.width
        anchors.topMargin:  _margins
        anchors.top:        _largeColumn.bottom
        layoutDirection:    Qt.LeftToRight
        spacing:            _margins

        Repeater {
            model: _activeVehicle ? controller.smallValues : 0

            Column {
                id:     valueColumn
                width:  (_root.width / 2) - (_margins / 2) - 0.1
                clip:   true

                property Fact fact: _activeVehicle.getFact(modelData.replace("Vehicle.", ""))

                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    font.pixelSize:         ScreenTools.smallFontPixelSize
                    fontSizeMode:           Text.HorizontalFit
                    color:                  textColor
                    text:                   fact.shortDescription
                }
                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    color:                  textColor
                    fontSizeMode:           Text.HorizontalFit
                    text:                   fact.enumOrValueString
                }
                QGCLabel {
                    width:                  parent.width
                    horizontalAlignment:    Text.AlignHCenter
                    font.pixelSize:         ScreenTools.smallFontPixelSize
                    fontSizeMode:           Text.HorizontalFit
                    color:                  textColor
                    text:                   fact.units
                }
            }
        } // Repeater - Small
    } // Flow

    Component {
        id: propertyPicker

        QGCViewDialog {
            id: _propertyPickerDialog

            QGCFlickable {
                anchors.fill:       parent
                contentHeight:      _loader.y + _loader.height
                flickableDirection: Flickable.VerticalFlick
                clip:               true

                QGCLabel {
                    id:     _label
                    text:   qsTr("Select the values you want to display:")
                }

                Loader {
                    id:                 _loader
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.topMargin:  _margins
                    anchors.top:        _label.bottom
                    sourceComponent:    factGroupList

                    property var factGroup:     _activeVehicle
                    property var factGroupName: "Vehicle"
                }
            }
        }
    }

    Component {
        id: factGroupList

        // You must push in the following properties from the Loader
        // property var factGroup
        // property string factGroupName

        Column {
            id:         _root
            spacing:    _margins

            QGCLabel {
                width:      parent.width
                wrapMode:   Text.WordWrap
                text:       factGroup ? factGroupName : qsTr("Vehicle must be connected to assign values.")
            }

            Repeater {
                model: factGroup ? factGroup.factNames : 0

                RowLayout {
                    spacing: _margins

                    property string propertyName: factGroupName + "." + modelData

                    function removeFromList(list, value) {
                        var newList = []
                        for (var i=0; i<list.length; i++) {
                            if (list[i] !== value) {
                                newList.push(list[i])
                            }
                        }
                        return newList
                    }

                    function addToList(list, value) {
                        var found = false
                        for (var i=0; i<list.length; i++) {
                            if (list[i] === value) {
                                found = true
                                break
                            }
                        }
                        if (!found) {
                            list.push(value)
                        }
                        return list
                    }

                    function updateValues() {
                        if (_addCheckBox.checked) {
                            if (_largeCheckBox.checked) {
                                controller.largeValues = addToList(controller.largeValues, propertyName)
                                controller.smallValues = removeFromList(controller.smallValues, propertyName)
                            } else {
                                controller.smallValues = addToList(controller.smallValues, propertyName)
                                controller.largeValues = removeFromList(controller.largeValues, propertyName)
                            }
                        } else {
                            controller.largeValues = removeFromList(controller.largeValues, propertyName)
                            controller.smallValues = removeFromList(controller.smallValues, propertyName)
                        }
                    }

                    QGCCheckBox {
                        id:         _addCheckBox
                        text:       factGroup.getFact(modelData).shortDescription
                        checked:    _largeCheckBox.checked || listContains(controller.smallValues, propertyName)
                        onClicked:  updateValues()
                        Layout.fillWidth:       true
                        Layout.minimumWidth:    ScreenTools.defaultFontPixelWidth * 20
                    }

                    QGCCheckBox {
                        id:         _largeCheckBox
                        text:       qsTr("Large")
                        checked:    listContains(controller.largeValues, propertyName)
                        enabled:    _addCheckBox.checked
                        onClicked:  updateValues()
                    }
                }
            }

            Item { height: 1; width: 1 }

            Repeater {
                model: factGroup ? factGroup.factGroupNames : 0

                Loader {
                    sourceComponent: factGroupList

                    property var    factGroup:      _root ? _root.parent.factGroup.getFactGroup(modelData) : undefined
                    property string factGroupName:  _root ? _root.parent.factGroupName + "." + modelData : undefined
                }
            }
        }
    }
}
