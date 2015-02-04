import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0

Label {
    property Fact fact: Fact { value: "FactLabel" }

    property var __qgcpal: QGCPalette { colorGroup: QGCPalette.Active }

    color: __qgcpal.windowText

    text: fact.valueString
}
