import QtQuick 2.7
import QtQuick.Controls 2.1
import QtQuick.Controls.Styles 1.4

Row {
    property string labelText: "Label"
    property string valueText: "value"

    width: parent.width

    QGCLabel {
        id:     label
        text:   labelText
    }
    QGCLabel {
        width:  parent.width - label.contentWidth
        text:   valueText
        horizontalAlignment: Text.AlignRight;
    }
}
