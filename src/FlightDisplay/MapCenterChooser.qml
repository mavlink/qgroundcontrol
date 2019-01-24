import QtQuick 2.0
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.4
import QtQuick.Layouts  1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
//import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
//import QGroundControl.ScreenTools   1.0

Item {

    id: viewChooser
    anchors.bottom: parent.bottom

    width: 100
    height: 100

    // Free move of map or center on drone/landing pad
    readonly property int centerNONE:               0 // N/A
    readonly property int centerDRONE:              1 // Drone
    readonly property int centerLP:                 2 // Landing Pad
    property int    centerMode:                     centerNONE

    ColumnLayout {
        spacing: 10
        //ExclusiveGroup { id: mapViewSelectorGroup }

        Rectangle {
            Text {
                text: "FREE"
            }
            width: 100
            height: 100
            color: "blue"
            border { width: 2; color: "black" }
        }

        Rectangle {
            Text {
                text: "DRONE"
            }
            width: 100
            height: 100
            color: "red"
            border { width: 2; color: "black" }
        }

        Rectangle {
            Text {
                text: "PAD"
            }
            width: 100
            height: 100
            color: "green"
            border { width: 2; color: "black" }
        }

        /*QGCRadioButton {
            id: buttonFree
            exclusiveGroup: mapViewSelectorGroup
            text:           qsTr("FREE")
            checked:        false
            color:          mapPal.text
            onClicked:      viewChooser.centerMode = viewChooser.centerNONE
        }

        QGCRadioButton {
            id: buttonDrone
            exclusiveGroup: mapViewSelectorGroup
            text:           qsTr("DRONE")
            checked:        true
            color:          mapPal.text
            onClicked:      viewChooser.centerMode = viewChooser.centerDRONE
        }

        QGCRadioButton {
            id: buttonPad
            exclusiveGroup: mapViewSelectorGroup
            text:           qsTr("PAD")
            checked:        false
            color:          mapPal.text
            onClicked:      viewChooser.centerMode = viewChooser.centerLP
        }*/


/*        RadioButton {
            text: qsTr("Drone")
            width: 100
            height: 100
            exclusiveGroup: tabPositionGroup
            onClicked: console.log("clicked:", button.text)
        }
        RadioButton {
            text: qsTr("Landing")
            width: 100
            height: 100
            exclusiveGroup: tabPositionGroup
            onClicked: console.log("clicked:", button.text)
        }*/
    }

}
