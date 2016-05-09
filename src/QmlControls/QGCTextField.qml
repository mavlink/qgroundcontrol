import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

TextField {
    id: root

    property bool showUnits: false
    property string unitsLabel: ""

    Component.onCompleted: {
        if (typeof qgcTextFieldforwardKeysTo !== 'undefined') {
            root.Keys.forwardTo = [qgcTextFieldforwardKeysTo]
        }
    }

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    textColor:          __qgcPal.textFieldText
    height:             ScreenTools.isMobile ? Math.max(25, Math.round(ScreenTools.defaultFontPixelHeight * 2)) : Math.max(25, Math.round(ScreenTools.defaultFontPixelHeight * 1.2))

    Label {
        id:             unitsLabelWidthGenerator
        text:           unitsLabel
        width:          contentWidth + parent.__contentHeight * 0.666
        visible:        false
        antialiasing:   true
        font.family:    ScreenTools.normalFontFamily
    }

    style: TextFieldStyle {
        font.pointSize: ScreenTools.defaultFontPointSize
        background: Item {
            id: backgroundItem

            Rectangle {
                anchors.fill:           parent
                anchors.bottomMargin:   -1
                color:                  "#44ffffff"
            }

            Rectangle {
                anchors.fill:           parent
                border.color:           control.activeFocus ? "#47b" : "#999"
                color:                  __qgcPal.textField
            }

            Text {
                id: unitsLabel

                anchors.top:    parent.top
                anchors.bottom: parent.bottom

                verticalAlignment:  Text.AlignVCenter
                horizontalAlignment:Text.AlignHCenter

                x:              parent.width - width
                width:          unitsLabelWidthGenerator.width

                text:           control.unitsLabel
                font.pointSize: ScreenTools.defaultFontPointSize
                font.family:    ScreenTools.normalFontFamily
                antialiasing:   true

                color:          control.textColor
                visible:        control.showUnits
            }
        }

        padding.right: control.showUnits ? unitsLabelWidthGenerator.width : control.__contentHeight * 0.333
    }

    onActiveFocusChanged: {
        if (!ScreenTools.isMobile && activeFocus) {
            selectAll()
        }
    }
}
