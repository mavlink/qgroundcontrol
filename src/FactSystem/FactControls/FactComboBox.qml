import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.Controls 1.0

QGCComboBox {
    property Fact fact: Fact { }
    currentIndex: fact.value
    onActivated: fact.value = index
}
