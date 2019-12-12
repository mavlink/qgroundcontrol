import QtQuick 2.4
import QtQuick.Controls 1.1
import QtQuick.Controls.Styles 1.1
import QtQuick.Dialogs 1.1
import QtQuick.Window 2.1

ApplicationWindow {
    id: window
    visible: true
    width: 640
    height: 480
    x: 30
    y: 30
    color: "dodgerblue"

    Item {
        anchors.fill: parent

        Rectangle {
            color: Qt.rgba(1, 1, 1, 0.7)
            border.width: 1
            border.color: "white"
            anchors.bottomMargin: 15
            anchors.horizontalCenter: parent.horizontalCenter
            width : parent.width - 30
            height: parent.height - 30
            radius: 8

            Text {
                id: text1
                anchors.centerIn: parent
                text: "Hello World!"
                font.pointSize: 24
                visible: timer.tex1_visible
            }

            Text {
                id: text2
                anchors.centerIn: parent
                text: "This is qmlglsrc demo!"
                font.pointSize: 24
                visible: timer.tex2_visible
            }

            Timer {
                id: timer
                property int count: 0
                property int tex1_visible: 1
                property int tex2_visible: 0
                interval: 30; running: true; repeat: true
                onTriggered: {
                  count++;
                  if (count%2 == 0) {
                    tex1_visible = 1;
                    tex2_visible = 0;
                  }
                  else {
                    tex1_visible = 0;
                    tex2_visible = 1;
                  }
                }
            }
        }
    }
}
