import QtQuick                  2.2
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.2

import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0

/// Mission item edit control
Rectangle {
    property var    missionItem

    width:          _editFieldWidth + (ScreenTools.defaultFontPixelWidth * 10)
    height:         _valueColumn.y + _valueColumn.height + (radius / 2)
    border.width:   2
    border.color:   "white"
    color:          "white"
    radius:         ScreenTools.defaultFontPixelWidth

    readonly property real _editFieldWidth: ScreenTools.defaultFontPixelWidth * 13

    MissionItemIndexLabel {
        id:                 _label
        anchors.top:        parent.top
        anchors.right:      parent.right
        isCurrentItem:      missionItem.isCurrentItem
        label:              missionItem.sequenceNumber
    }

    QGCComboBox {
        id:                 _commandCombo
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.right:      _label.left
        anchors.top:        parent.top
        currentIndex:       missionItem.commandByIndex
        model:              missionItem.commandNames

        onActivated: missionItem.commandByIndex = index
    }

    Column {
        id:                 _coordinateColumn
        anchors.left:       parent.left
        anchors.right:      parent.right
        visible:            missionItem.specifiesCoordinate

    }

    QGCTextField {
        id:                 _latitudeField
        anchors.margins:    parent.radius / 2
        anchors.top:        _commandCombo.bottom
        anchors.right:      parent.right
        width:              _editFieldWidth
        text:               missionItem.coordinate.latitude
        visible:            missionItem.specifiesCoordinate

        onAccepted:         missionItem.coordinate.latitude = text
    }

    QGCTextField {
        id:                 _longitudeField
        anchors.margins:    parent.radius / 2
        anchors.top:        _latitudeField.bottom
        anchors.right:      parent.right
        width:              _editFieldWidth
        text:               missionItem.coordinate.longitude
        visible:            missionItem.specifiesCoordinate

        onAccepted:         missionItem.coordinate.longtitude = text
    }

    QGCTextField {
        id:                 _altitudeField
        anchors.margins:    parent.radius / 2
        anchors.top:        _longitudeField.bottom
        anchors.right:      parent.right
        width:              _editFieldWidth
        text:               missionItem.coordinate.altitude
        visible:            missionItem.specifiesCoordinate
        showUnits:          true
        unitsLabel:         "meters"

        onAccepted:         missionItem.coordinate.altitude = text
    }

    QGCLabel {
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.baseline:   _latitudeField.baseline
        color:              "black"
        text:               "Lat:"
        visible:            missionItem.specifiesCoordinate
    }

    QGCLabel {
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.baseline:   _longitudeField.baseline
        color:              "black"
        text:               "Long:"
        visible:            missionItem.specifiesCoordinate
    }

    QGCLabel {
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.baseline:   _altitudeField.baseline
        color:              "black"
        text:               "Alt:"
        visible:            missionItem.specifiesCoordinate
    }

    Column {
        id:                 _valueColumn
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        missionItem.specifiesCoordinate ? _altitudeField.bottom : _commandCombo.bottom

        Repeater {
            model: missionItem.valueLabels

            QGCLabel {
                color:  "black"
                text:   modelData
            }
        }
    }

    Column {
        anchors.margins:    parent.radius / 2
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.top:        _valueColumn.top

        Repeater {
            model: missionItem.valueStrings

            QGCLabel {
                width:                  _valueColumn.width
                color:                  "black"
                text:                   modelData
                horizontalAlignment:    Text.AlignRight
            }
        }
    }
}
