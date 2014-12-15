import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2
import QGroundControl.FactControls 1.0

Rectangle {
    anchors.fill: parent
    color: "#222"

    Image {
        anchors.fill: parent
        fillMode: Image.PreserveAspectFit
        smooth: true
        source: "octo_x.png"
    }

    Column {
        anchors.margins: 20

        anchors.fill: parent

        Rectangle { id: header; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.75
            Text { id: titleText; anchors.centerIn: parent; font.pointSize: 24; text: "Vehicle Summary" }
        }

        Flow {
            anchors.topMargin: 10
            anchors.bottomMargin: 10

            width: parent.width;
            height: parent.height - header.height - footer.height - 200
            spacing: 5
            //color: "#028000"

            Repeater {
                model: vehicleComponents
                SetupButton {
                    width: 200
                    height: 200
                    text: modelData.name
                    setupComplete: modelData.setupComplete
                    summaryModel: modelData.summaryItems
                }
            }
        }

        Row { id: footer; anchors.horizontalCenter: parent.horizontalCenter; width: parent.width; height: 50; opacity: 0.75
            Text { font.pointSize: 24; text: "Footer" }
        }
    }
}
