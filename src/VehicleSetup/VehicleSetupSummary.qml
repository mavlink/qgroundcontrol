// Currently unused but leaving here for future conversion to QML

import QtQuick 2.0
import QtQuick.Controls 1.2

Item {
    width: parent.width
    height: parent.height

    ListModel {
        id: vehicleComponentModelTest
        ListElement {
            name: "Gyroscope"
            description: "Long description"
            setupComplete: false
        }
        ListElement {
            name: "Magnetometer"
            description: "Long description"
            setupComplete: false
        }
        ListElement {
            name: "Radio"
            description: "Long description"
            setupComplete: true
        }
    }

    ListView {
        id: componentList
        anchors.fill: parent
        model: vehicleComponentModel

        section.property: "setupComplete"
        section.criteria: ViewSection.FullString
        section.delegate: Text { text: section == "true" ? "Setup complete" : "Requires setup" }

        delegate: Item {
            width: 180; height: 40
            Column {
                Text { text: "Component:" + model.name }
                Text { text: model.description }
            }
        }
    }
}
