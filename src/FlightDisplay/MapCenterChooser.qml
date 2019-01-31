import QtQuick 2.0

Rectangle {
    id: viewChooser
    color: colorBG
    width: buttonWidth
    height: buttonHeight * 4
    z: parent.z + 4 // This is important. If this control is underneath the map, we don't get touch/click input

    // Free move of map or center on drone/landing pad
    readonly property int centerNONE:               0 // N/A
    readonly property int centerDRONE:              1 // Drone
    readonly property int centerLP:                 2 // Landing Pad
    property int    centerMode:                     centerLP

    // Button attributes
    readonly property int buttonWidth: 50
    readonly property int buttonHeight: 30
    readonly property int buttonFontSize: 10

    // Box colors
    readonly property string colorBG:       "black"
    readonly property string colorText:     "white"
    readonly property string colorClick:    "red"
    readonly property string colorSel:      "green"
    readonly property string colorBorder:   "gray"

    Rectangle {
        id: rectLabel
        width: buttonWidth; height: buttonHeight
        color: colorBG
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            color: colorText
            font.pointSize: buttonFontSize
            text: "Center"
        }
    }

    Rectangle {
        id: rectFree
        anchors.top: rectLabel.bottom
        width: buttonWidth; height: buttonHeight
        color: colorBG
        border { width: 1; color: colorBorder }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            color: colorText
            font.pointSize: buttonFontSize
            text: "Free"
        }
        MouseArea {
            id: mouseAreaFree
            anchors.fill: parent
            preventStealing: true
            onPressed:
            {
                parent.color = colorClick
            }
            onReleased:
            {
                centerMode = centerNONE
                rectFree.color = colorSel
                rectDrone.color = colorBG
                rectPad.color = colorBG
            }
        }
    }

    Rectangle {
        id: rectDrone
        anchors.top: rectFree.bottom
        width: buttonWidth; height: buttonHeight
        color: colorBG
        border { width: 1; color: colorBorder }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            color: colorText
            font.pointSize: buttonFontSize
            text: "Drone"
        }
        MouseArea {
            id: mouseAreaDrone
            anchors.fill: parent
            preventStealing: true
            onPressed:
            {
                parent.color = colorClick
            }
            onReleased:
            {
                centerMode = centerDRONE
                rectFree.color = colorBG
                rectDrone.color = colorSel
                rectPad.color = colorBG
            }
        }
    }

    Rectangle {
        id: rectPad
        anchors.top: rectDrone.bottom
        width: buttonWidth; height: buttonHeight
        color: colorSel
        border { width: 1; color: colorBorder }
        Text {
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            color: colorText
            font.pointSize: buttonFontSize
            text: "Pad"
        }
        MouseArea {
            id: mouseAreaPad
            anchors.fill: parent
            preventStealing: true
            onPressed:
            {
                parent.color = colorClick
            }
            onReleased:
            {
                centerMode = centerLP
                rectFree.color = colorBG
                rectDrone.color = colorBG
                rectPad.color = colorSel
            }
        }
    }
}
