import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

RowLayout {
    property string labelText: "Label"
    property string valueText: "value"

    width: parent.width

    function translatedValue(textValue) {
        var text = (textValue === undefined || textValue === null) ? "" : textValue.toString().trim()
        switch (text) {
        case "Disabled":                   return qsTr("Disabled")
        case "Enabled":                    return qsTr("Enabled")
        case "Some disabled":              return qsTr("Some disabled")
        case "Manual":                     return qsTr("Manual")
        case "Auto":                       return qsTr("Auto")
        case "RTL":                        return qsTr("RTL")
        case "Hold":                       return qsTr("Hold")
        case "Always Hold":                return qsTr("Always Hold")
        case "Always RTL":                 return qsTr("Always RTL")
        case "Hold and Disarm":            return qsTr("Hold and Disarm")
        case "None":                       return qsTr("None")
        case "Unknown":                    return qsTr("Unknown")
        case "Undefined":                  return qsTr("Undefined")
        case "Rover":                      return qsTr("Rover")
        case "Analog Voltage and Current": return qsTr("Analog Voltage and Current")
        default:                           return textValue
        }
    }

    QGCLabel {
        id:     label
        text:   labelText
    }
    QGCLabel {
        text:                   translatedValue(valueText)
        elide:                  Text.ElideRight
        horizontalAlignment:    Text.AlignRight
        Layout.fillWidth:       true
    }
}
