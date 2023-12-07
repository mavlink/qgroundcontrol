import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import Qt.labs.platform

Rectangle {
    id:             _root
    width:          80
    height:         20
    border.width:   1
    border.color:   "black"

    signal colorSelected(var color)

    ColorDialog {
        id: colorDialog
        onAccepted: {
            _root.colorSelected(colorDialog.color)
            colorDialog.close()
        }
    }

    MouseArea {
        anchors.fill: parent

        onClicked: {
            colorDialog.color = _root.color
            colorDialog.visible = true
        }
    }
}
