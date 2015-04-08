import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

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
