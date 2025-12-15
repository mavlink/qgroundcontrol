import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    anchors.fill: parent

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: 0
        spacing: ScreenTools.defaultFontPixelHeight * 0.4

        QGCTabBar {
            id: tabBar
            width: parent.width
            QGCTabButton { text: qsTr("About") }
            QGCTabButton { text: qsTr("Help") }
            QGCTabButton { text: qsTr("Guide") }
        }

        Loader {
            id: pageLoader
            Layout.fillWidth: true
            Layout.fillHeight: true
            source: tabBar.currentIndex === 0 ? "qrc:/qml/QGroundControl/AppSettings/AboutSettings.qml"
                                              : (tabBar.currentIndex === 1 ? "qrc:/qml/QGroundControl/AppSettings/HelpSettings.qml" : "qrc:/qml/QGroundControl/AppSettings/GuideSettings.qml")
        }
    }
}
