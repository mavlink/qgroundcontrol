import QtQuick 2.2

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

Item {
    id: legendRoot

    property var    _qgcPal:            QGCPalette { colorGroupEnabled: enabled }
    property real   _rotorRadius:       legendRoot.height / 16

    readonly property string    _cwColor:               "green"
    readonly property string    _ccwColor:              "blue"

    Item {
        id:             cwItem
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         legendRoot.height / 2

        Rectangle {
            id:                     cwRotor
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  _rotorRadius * 2
            height:                 _rotorRadius * 2
            radius:                 _rotorRadius
            color:                  _cwColor
        }

        QGCLabel {
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.left:       cwRotor.right
            anchors.right:      parent.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            verticalAlignment:  Text.AlignVCenter
            wrapMode:           Text.WordWrap
            text:               qsTr("Clockwise rotation, use pusher propellor")
        }
    }

    Item {
        id:             ccwItem
        anchors.top:    cwItem.bottom
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right

        Rectangle {
            id:                     ccwRotor
            anchors.left:           parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:                  _rotorRadius * 2
            height:                 _rotorRadius * 2
            radius:                 _rotorRadius
            color:                  _ccwColor
        }

        QGCLabel {
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth
            anchors.left:       ccwRotor.right
            anchors.right:      parent.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            verticalAlignment:  Text.AlignVCenter
            wrapMode:           Text.WordWrap
            text:               qsTr("Counter-Clockwise rotation, use normal propellor")
        }
    }
} // Item
