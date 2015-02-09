import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0

TextField {
    property Fact fact: Fact { value: 0 }
    property bool showUnits: false

    property var __qgcpal: QGCPalette { colorGroup: QGCPalette.Active }

    text: fact.valueString
    textColor: __qgcpal.text

    Label {
        id: unitsLabelWidthGenerator
        text: parent.fact.units
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
                color: __qgcpal.base
            }

            Text {
                id: unitsLabel

                anchors.top: parent.top
                anchors.bottom: parent.bottom

                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter

                x: parent.width - width
                width: unitsLabelWidthGenerator.width

                text: control.fact.units
                color: control.textColor
                visible: control.showUnits
            }
        }

        padding.right: control.showUnits ? unitsLabelWidthGenerator.width : control.__contentHeight/3
    }

    onEditingFinished: fact.value = text
}
