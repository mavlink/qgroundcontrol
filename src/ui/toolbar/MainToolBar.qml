/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Item {
    id:         toolBar

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        flyButton.checked = true
    }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          "black"
        visible:        qgcPal.globalTheme === QGCPalette.Light
    }

    //-- Setup can be invoked from c++ side
    Connections {
        target: setupWindow
        onVisibleChanged: {
            if(setupWindow.visible) {
                setupButton.checked = true
            }
        }
    }

    SettingsView
    {
        id: settingsView
        x: sideMenu.x + sideMenu.width
        y: toolBar.y + toolBar.height
        height: mainWindow.height - toolBar.height
        width: mainWindow.width - sideMenu.width
    }

    SideMenu {
        id: sideMenu
        onSetupMenuClicked: settingsView.visible = true
    }

    RowLayout {
        anchors.bottomMargin:   1
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 2
        anchors.fill:           parent
        spacing:                ScreenTools.defaultFontPixelWidth * 2

        ButtonGroup {
            buttons:            viewRow.children
        }

        //---------------------------------------------
        // Toolbar Row
        Row {
            id:                 viewRow
            Layout.fillHeight:  true
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            QGCToolBarButton {
                id:                 sideMenuButton
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                icon.source:        "/qmlimages/Menu.svg"

                Component.onCompleted: sideMenuButton.adjustImageSize(-25, -25)

                onClicked:
                {
                    checked = sideMenu.visible = sideMenu.triggerRoll()

                    if(!checked)
                        settingsView.visible = false
                }
            }

            Rectangle {
                anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              1
                color:              qgcPal.text
                visible:            activeVehicle
            }
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
        visible:        !largeProgressBar.visible
    }

    // Large parameter download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: activeVehicle ? activeVehicle.parameterManager.parametersReady : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
            color:          qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading Parameters")
            font.pointSize:     ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}
