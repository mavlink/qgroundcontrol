import QtQuick 2.3
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4

import QGroundControl.Controls 1.0
import QGroundControl.FactSystem 1.0
import QGroundControl.Palette 1.0
import QGroundControl.ScreenTools 1.0

QGCComboBox {
    property Fact fact: Fact { }
    property bool indexModel: true  ///< true: model must be specifed, selected index is fact value, false: use enum meta data
    model: fact ? fact.enumStrings : []
    currentIndex: fact ? (indexModel ? fact.value : fact.enumIndex) : 0
    onActivated: {
        if (indexModel) {
            fact.value = index
        } else {
            fact.value = fact.enumValues[index]
        }
    }
    style: ButtonStyle {
        background: Rectangle {
            implicitWidth:  ScreenTools.implicitComboBoxWidth
            implicitHeight: ScreenTools.implicitComboBoxHeight
            color:          Qt.rgba(0,0,0,0)
        }
        label: Item {
            implicitWidth:  text.implicitWidth
            implicitHeight: text.implicitHeight
            baselineOffset: text.y + text.baselineOffset
            QGCLabel {
                id:                     text
                horizontalAlignment:    Text.AlignHCenter
                anchors.verticalCenter: parent.verticalCenter
                text:                   control.currentText
                color:                  control._qgcPal.buttonText
            }
        }
    }
}
