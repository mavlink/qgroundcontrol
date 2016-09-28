import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0

Rectangle {
    id:     root
    height: _currentItem ? valuesRect.y + valuesRect.height + (_margin * 2) : titleBar.y - titleBar.height + _margin
    color:  _currentItem ? qgcPal.buttonHighlight : qgcPal.windowShade
    radius: _radius

    property var rallyPoint ///< RallyPoint object associated with editor
    property var controller ///< RallyPointController

    property bool   _currentItem:       rallyPoint ? rallyPoint == controller.currentRallyPoint : false
    property color  _outerTextColor:    _currentItem ? "black" : qgcPal.text

    readonly property real  _editFieldWidth:    Math.min(width - _margin * 2, ScreenTools.defaultFontPixelWidth * 12)
    readonly property real  _margin:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _radius:            ScreenTools.defaultFontPixelWidth / 2
    readonly property real  _titleHeight:       ScreenTools.defaultFontPixelHeight * 2

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    Item {
        id:                 titleBar
        anchors.margins:    _margin
        anchors.top:        parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        height:             _titleHeight

        MissionItemIndexLabel {
            id:                     indicator
            anchors.verticalCenter: parent.verticalCenter
            anchors.left:           parent.left
            label:                  "R"
            checked:                true
        }

        QGCLabel {
            anchors.leftMargin:     _margin
            anchors.left:           indicator.right
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("Rally Point")
            color:                  _outerTextColor
        }

        Image {
            id:                     hamburger
            anchors.rightMargin:    _margin
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            width:                  ScreenTools.defaultFontPixelWidth * 2
            height:                 width
            sourceSize.height:      height
            source:                 "qrc:/qmlimages/Hamburger.svg"

            MouseArea {
                anchors.fill:   parent
                onClicked:      hamburgerMenu.popup()

                Menu {
                    id: hamburgerMenu

                    MenuItem {
                        text:           qsTr("Delete")
                        onTriggered:    controller.removePoint(rallyPoint)
                    }
                }
            }
        }
    } // Item - titleBar

    Rectangle {
        id:                 valuesRect
        anchors.margins:    _margin
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        titleBar.bottom
        height:             valuesColumn.height + (_margin * 2)
        color:              qgcPal.windowShadeDark
        visible:            _currentItem
        radius:             _radius

        Column {
            id:                 valuesColumn
            anchors.margins:    _margin
            anchors.left:       parent.left
            anchors.right:      parent.right
            anchors.top:        parent.top
            spacing:            _margin

            Repeater {
                model: rallyPoint ? rallyPoint.textFieldFacts : 0

                Item {
                    width:  valuesColumn.width
                    height: textField.height

                    QGCLabel {
                        id:                 textFieldLabel
                        anchors.baseline:   textField.baseline
                        text:               modelData.name + ":"
                    }

                    FactTextField {
                        id:             textField
                        anchors.right:  parent.right
                        width:          _editFieldWidth
                        showUnits:      true
                        fact:           modelData
                    }
                }
            } // Repeater - text fields
        } // Column
    } // Rectangle
} // Rectangle
