import QtQuick 2.2
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

Rectangle {
    anchors.fill: parent
    color: "#222"

    Item {
        anchors.margins: 20
        anchors.fill: parent

        Rectangle { id: header; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.8;
            Text { id: titleText; anchors.centerIn: parent; font.pointSize: 24; text: "Vehicle Summary" }
        }

        Text { width: parent.width; height: parent.height - header.height - footer.height;
            anchors.top: header.bottom
            color: "white"
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            text: "No vehicle currently connected. Vehicle Setup is only available while vehicle is connected." }

        Rectangle { id: footer; anchors.bottom: parent.bottom; color: "lightblue"; radius: 10.0; width: parent.width; height: titleText.height + 20; opacity: 0.8;

            Button { id: firmwareButton; objectName: "firmwareButton";
                anchors.horizontalCenter: parent.horizontalCenter; anchors.verticalCenter: parent.verticalCenter;
                text: "Firmware Upgrade"
            }
        }
    }
}
