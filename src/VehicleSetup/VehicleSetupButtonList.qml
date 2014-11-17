// Currently unused but leaving here for future conversion to QML

import QtQuick 2.0
import QtQuick.Controls 1.2

Rectangle {
    SystemPalette { id: myPalette; colorGroup: SystemPalette.Active }

    color: myPalette.dark

    anchors.fill: parent
    id: buttonParent
    signal componentButtonClicked(variant varComponent)

Button {
    id: summaryButton
    objectName: "summaryButton"
    height: 50
    width: 150
    text: "Summary"
}

Button {
    id: firmwareButton
    objectName: "firmwareButton"
    height: 50
    width: 150
    anchors.top: summaryButton.bottom
    text: "Firmware"
}

ListModel {
    id: vehicleComponentModelTest
    ListElement {
        name: "Radio"
        setupComplete: true
        icon: "/Users/Don/repos/qgroundcontrol/files/images/px4/menu/remote.png"
    }
    ListElement {
        name: "Gyroscope"
        setupCompleted: false
        icon: "qrc:/files/images/px4/menu/remote.png"
    }
    ListElement {
        name: "Magnetometer"
        icon: "qrc:/files/images/px4/menu/remote.png"
    }
}

ListView {
    id: componentList
    width: 200; height: contentItem.childrenRect.height
    snapMode: ListView.NoSnap
    anchors.top: firmwareButton.bottom

    model: vehicleComponentModel


    delegate: Button {
        objectName: name + "SetupButton"
        height: 50
        width: 150
        text: name
        iconSource: icon

        onClicked: {
            console.log("Setup button clicked")
            buttonParent.componentButtonClicked(modelData)
        }

        Rectangle {
            width: 10
            height: 10
            color: setupComplete ? "green" : "red"
            border.color: "black"
        }
    }
}
}
