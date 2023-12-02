import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette

// Toolbar for Plan View
Rectangle {
    id:                 _root
    color:              qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)
    anchors.fill:       parent
    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }
    RowLayout {
        anchors.bottomMargin:   1
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 2
        anchors.fill:           parent
        spacing:                ScreenTools.defaultFontPixelWidth * 2
        QGCToolBarButton {
            id:                 settingsButton
            Layout.fillHeight:  true
            icon.source:        "/qmlimages/PaperPlane.svg"
            checked:            false
            onClicked: {
                checked = false
                mainWindow.showFlyView()
            }
        }
        Loader {
            source:             "PlanToolBarIndicators.qml"
            Layout.fillWidth:   true
            Layout.fillHeight:  true
        }
    }
}

