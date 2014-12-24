import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0

TextField {
    property Fact fact: Fact { value: 0 }
    property bool showUnits: false

    text: fact.valueString

    Label {
        id: unitsLabelWidthGenerator
        text: parent.fact.units
        width: contentWidth + ((parent.__contentHeight/3)*2)
        visible: false
    }

    style: TextFieldStyle {
        background: Item {
            id: backgroundItem
            property real unitsLabelWidth: 10
            Rectangle {
                anchors.fill: parent
                anchors.bottomMargin: -1
                color: "#44ffffff"
            }
            Rectangle {
                id: baserect
                gradient: Gradient {
                    GradientStop {color: "#e0e0e0" ; position: 0}
                    GradientStop {color: "#fff" ; position: 0.1}
                    GradientStop {color: "#fff" ; position: 1}
                }
                anchors.fill: parent
                border.color: control.activeFocus ? "#47b" : "#999"
            }
            Label {
                id: unitsLabel
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: unitsLabelWidthGenerator.width
                x: parent.width - width
                verticalAlignment: Text.AlignVCenter
                horizontalAlignment: Text.AlignHCenter
                text: control.fact.units
                visible: control.showUnits
            }
        }
        padding.right: control.showUnits ? unitsLabelWidthGenerator.width : control.__contentHeight/3
    }

    onEditingFinished: fact.value = text
}
