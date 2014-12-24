import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0

Label {
    property Fact fact: Fact { value: "FactLabel" }
    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    color: palette.windowText

    text: fact.value
}
