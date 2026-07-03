import QtQuick
import QtQuick.Shapes 2.15

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property int activeSettingsId: 0


    QGCPalette { id: qgcPalette}

    Rectangle {
        id: background
        anchors.fill: parent
        radius: 5
        color: qgcPalette.window
        border.width: 1
        border.color: qgcPalette.buttonBorder
    }
    
    Item {
        id: settingsContainer
        anchors.fill: parent
        anchors.margins: ScreenTools.defaultFontPixelHeight / 2

        Item {
            id: settingsTitleContainer
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 50

            Rectangle {
                anchors.fill: parent
                color: "blue"
            }

            QGCLabel {
                id: settingsLabel
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("General")
                //font.bold:  true
                font.pointSize: ScreenTools.largeFontPointSize
            
            }
        }

        Item {
            id: idk
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.top: settingsTitleContainer.bottom
            anchors.bottom: parent.bottom

            Item {
                id: settingsCategoryContainer 
                anchors.left: parent.left
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                width: 200

                Rectangle {
                    anchors.fill: parent
                    color: "red"
                }
            }

            Item {
                id: settingsSeperator 
                width: 30
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: settingsCategoryContainer.right
                

                Rectangle {
                    anchors.fill: parent
                    color: "purple"
                }

            }

            Item {
                id: settingsContentContainer 
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                anchors.left: settingsSeperator.right

                Rectangle {
                    anchors.fill: parent
                    color: "yellow"
                }
            }
        }

        


    }
}