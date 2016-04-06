import QtQuick                  2.5
import QtQuick.Controls         1.4

import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightMap     1.0

Item {
    id: _root
    clip: true
    height: valuesPage.height + pageIndicatorRow.anchors.topMargin + pageIndicatorRow.height

    property var    qgcView     ///< QGCView to use for showing dialogs
    property color  textColor
    property color  backgroundColor
    property var    maxHeight   ///< Maximum height that should be taken, smaller than this is ok

    property real   _margins:   ScreenTools.defaultFontPixelWidth / 2

    function showPicker() {
        valuesPage.showPicker()
    }

    ValuesWidget {
        id:         valuesPage
        width:      _root.width
        qgcView:    _root.qgcView
        textColor:  _root.textColor
        maxHeight:  _root.maxHeight
    }

    VibrationWidget {
        id:                 vibrationPage
        anchors.left:       valuesPage.right
        width:              _root.width
        textColor:          _root.textColor
        backgroundColor:    _root.backgroundColor
        maxHeight:          _root.maxHeight
    }

    Row {
        id:                         pageIndicatorRow
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        spacing:                    _margins

        Rectangle {
            id:             valuesPageIndicator
            height:         radius * 2
            width:          radius * 2
            radius:         2.5
            border.color:   textColor
            border.width:   1
            color:          textColor
        }

        Rectangle {
            id:             vibrationPageIndicator
            height:         radius * 2
            width:          radius * 2
            radius:         2.5
            border.color:   textColor
            border.width:   1
            color:          "transparent"
        }
    }

    MouseArea {
        anchors.fill: parent

        property real xDragStart
        property real xValuesPageSave

        onPressed: {
            if (mouse.button == Qt.LeftButton) {
                mouse.accepted = true
                xDragStart = mouse.x
                xValuesPageSave = valuesPage.x
            }
        }

        onPositionChanged: {
            valuesPage.x = xValuesPageSave + mouse.x - xDragStart
        }

        onReleased: {
            if (mouse.x < xDragStart) {
                if (xValuesPageSave == 0) {
                    valuesPage.x = -valuesPage.width
                    _root.height = Qt.binding(function() { return vibrationPage.height + pageIndicatorRow.anchors.topMargin + pageIndicatorRow.height } )
                    valuesPageIndicator.color = "transparent"
                    vibrationPageIndicator.color = textColor
                } else {
                    valuesPage.x = xValuesPageSave
                }
            } else {
                if (xValuesPageSave != 0) {
                    valuesPage.x = 0
                    _root.height = Qt.binding(function() { return valuesPage.height + pageIndicatorRow.anchors.topMargin + pageIndicatorRow.height } )
                    valuesPageIndicator.color = textColor
                    vibrationPageIndicator.color = "transparent"
                } else {
                    valuesPage.x = xValuesPageSave
                }
            }
        }
    }
}
