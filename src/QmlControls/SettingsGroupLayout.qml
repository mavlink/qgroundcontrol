import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

ColumnLayout {
    id:             control    
    spacing:        _margins / 2
    implicitWidth:  _contentLayout.implicitWidth + (_margins * 2)
    implicitHeight: _contentLayout.implicitHeight + (_margins * 2)

    default property alias contentItem: _contentLayout.data

    property alias contentSpacing: _contentLayout.spacing

    property string heading
    property string headingDescription

    property real _margins: ScreenTools.defaultFontPixelHeight / 2

    ColumnLayout {
        Layout.leftMargin:  _margins
        Layout.fillWidth:   true
        spacing:            0
        visible:            heading !== ""

        QGCLabel { 
            text:       heading
            font.bold:  true
        }

        QGCLabel { 
            Layout.fillWidth:   true
            text:               headingDescription
            wrapMode:           Text.WordWrap
            font.pointSize:     ScreenTools.smallFontPointSize
            visible:            headingDescription !== ""
        }
    }

    Rectangle {
        id:                 outerRect
        Layout.fillWidth:   true
        implicitWidth:      _contentLayout.implicitWidth + (_margins * 2)
        implicitHeight:     _contentLayout.implicitHeight + (_margins * 2)
        color:              "transparent"
        border.color:       Qt.darker(QGroundControl.globalPalette.text, 4)
        border.width:       1
        radius:             ScreenTools.defaultFontPixelHeight / 2

        Repeater {
            model: 0 // _contentLayout.children.length - still nto working correctly, also not sure I want this

            Rectangle {
                x:                  0
                y:                  _contentItem.y + _contentItem.height + _margins + (_margins / 2)
                width:              parent.width
                height:             1
                color:              outerRect.border.color
                visible:            index < _contentLayout.children.length - 1

                property var _contentItem: _contentLayout.children[index]
            }
        }
 
        ColumnLayout {
            id:                 _contentLayout
            x:                  _margins
            y:                  _margins
            width:              parent.width - (_margins * 2)
            spacing:            _margins
        }
    }
}
