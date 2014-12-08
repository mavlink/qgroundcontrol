import QtQuick 2.0
import QtQuick.Controls 1.2
import QGroundControl.FactSystem 1.0

TextInput {
    property Fact fact
    text: fact.value
    font.family: "Helvetica"
    font.pointSize: 24
    color: "red"
    focus: true
    onAccepted: { fact.value = text; }
}