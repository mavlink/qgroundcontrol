import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Palette

ColumnLayout {
    id:                 control    
    spacing:            _margins / 2
    implicitWidth:      _contentLayout.implicitWidth + (_margins * 2)
    implicitHeight:     _contentLayout.implicitHeight + (_margins * 2)

    default property alias contentItem: _contentLayout.data

    property alias contentSpacing: _contentLayout.spacing

    property string defaultBorderColor  : QGroundControl.globalPalette.groupBorder
    property string outerBorderColor    : defaultBorderColor

    property string heading
    property string headingDescription
    property bool   showDividers:       true
    property bool   showBorder:         true

    property real _margins: ScreenTools.defaultFontPixelHeight / 2

    readonly property bool  _cardStyle: showBorder && (typeof mainWindow !== "undefined") && mainWindow.settingsPageOpen
    readonly property bool  _light:     QGroundControl.globalPalette.globalTheme === QGCPalette.Light
    readonly property color _cOuter:    _light ? "#E6E7EB" : "#404040"
    readonly property color _cInner:    _light ? "#FFFFFF" : "#202020"
    readonly property color _cLine:     _light ? "#E4E5E9" : "#606060"

    ColumnLayout {
        Layout.leftMargin:  control._cardStyle ? 22 : _margins
        Layout.topMargin: control._cardStyle ? 14 : 0
        Layout.bottomMargin: control._cardStyle ? 6 : 0
        Layout.fillWidth:   true
        spacing:            0
        visible:            heading !== ""

        QGCLabel { 
            text:           heading
            font.pointSize: ScreenTools.defaultFontPointSize + (control._cardStyle ? 3 : 1)
            font.bold:      true
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
        implicitWidth:      _contentLayout.implicitWidth + (showBorder ? _margins * 2 : 0)
        implicitHeight:     _contentLayout.implicitHeight + (showBorder ? _margins * 2: 0)
        Layout.leftMargin:      control._cardStyle ? 12 : 0
        Layout.rightMargin:     control._cardStyle ? 12 : 0
        Layout.bottomMargin:    control._cardStyle ? 12 : 0
        Layout.topMargin:       control._cardStyle && heading === "" ? 12 : 0
        color:                  _cardStyle ? _cInner : "transparent"
        border.color:           _cardStyle ? _cLine : outerBorderColor
        border.width:           _cardStyle ? (_light ? 0 : 1) : (showBorder ? 1 : 0)
        radius:                 _cardStyle ? 12 : ScreenTools.defaultFontPixelHeight / 2

        // gray outer card, drawn behind the inner box
        Rectangle {
            visible:    control._cardStyle
            z:          -1
            x:          -12
            y:          -outerRect.y
            width:      control.width
            height:     control.height
            radius:     20
            color:      control._cOuter
        }

        Repeater {
            model: showDividers? _contentLayout.children.length : 0

            Rectangle {
                x:                  showBorder ? _margins : 0
                y:                  _contentItem.y + _contentItem.height + _margins + (showBorder ? _margins : 0)
                width:              parent.width - (showBorder ? _margins * 2 : 0)
                height:             1
                color:              control._cardStyle ? control._cLine : QGroundControl.globalPalette.groupBorder
                visible:            _contentItem.visible && 
                                        _contentItem.width !== 0 && _contentItem.height !== 0 &&
                                        index < _contentLayout.children.length - 1

                property var _contentItem: _contentLayout.children[index]
            }
        }
 
        ColumnLayout {
            id:                 _contentLayout
            x:                  showBorder ? _margins : 0
            y:                  showBorder ? _margins : 0
            width:              parent.width - (showBorder ? _margins * 2 : 0)
            spacing:            _margins * 2
        }
    }
}
