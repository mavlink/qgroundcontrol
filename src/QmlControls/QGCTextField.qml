import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.Palette 1.0

TextField {
    property bool showUnits: false
    property string unitsLabel: ""

    property var __qgcPal: QGCPalette { colorGroupEnabled: enabled }

    textColor: __qgcPal.textFieldText

    Label {
        id: unitsLabelWidthGenerator
        text: unitsLabel
        width: contentWidth + ((parent.__contentHeight/3)*2)
        visible: false
    }

    style: TextFieldStyle {
        background: Item {
            id: backgroundItem

            Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: -1
                color: "#44ffffff"
            }

            Rectangle {
                anchors.fill: parent

                border.color: control.activeFocus ? "#47b" : "#999"
                color: __qgcPal.textField
            }

            Text {
                id: unitsLabel

                anchors.top: parent.top
                anchors.bottom: parent.bottom

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

                x: parent.width - width
                width: unitsLabelWidthGenerator.width

                text: control.unitsLabel
                color: control.textColor
                visible: control.showUnits
            }
        }

        padding.right: control.showUnits ? unitsLabelWidthGenerator.width : control.__contentHeight/3
    }
}
