import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactSystem 1.0

TextField {
    property Fact fact: Fact { value: 0 }

    QGCPalette { id: palette; colorGroup: QGCPalette.Active }

    text: fact.value

    onAccepted: fact.value = text
}
