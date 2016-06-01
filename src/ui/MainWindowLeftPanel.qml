/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.5
import QtQuick.Controls 1.2
import QtPositioning    5.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.ScreenTools   1.0


//-- Left Menu
Item {
    id:             settingsMenu
    anchors.fill:   parent

    readonly property real __closeButtonSize:   ScreenTools.defaultFontPixelHeight * 1.5
    readonly property real _margins:            ScreenTools.defaultFontPixelHeight * 0.5
    readonly property real _buttonHeight:       ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight * 2

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        //-- Default to General Settings
        __rightPanel.source = "GeneralSettings.qml"
        _generalButton.checked = true
        panelActionGroup.current = _generalButton
    }

    // This covers the screen with a transparent section
    Rectangle {
        id:             __transparentSection
        height:         parent.height - toolBar.height
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        opacity:        0.0
        color:          qgcPal.window
        visible:        __rightPanel.source == ""
    }

    //-- Top Separator
    Rectangle {
        id:             __topSeparator
        width:          parent.width
        height:         1
        y:              toolBar.height
        anchors.left:   parent.left
        color:          QGroundControl.isDarkStyle ? "#909090" : "#7f7f7f"
    }

    // This is the menu dialog panel which is anchored to the left edge
    Rectangle {
        id:             __leftMenu
        width:          ScreenTools.defaultFontPixelWidth * 16
        anchors.left:   parent.left
        anchors.top:    __topSeparator.bottom
        anchors.bottom: parent.bottom
        color:          qgcPal.windowShadeDark

        QGCFlickable {
            anchors.fill:       parent
            contentHeight:      buttonColumn.height + _margins
            flickableDirection: Flickable.VerticalFlick
            clip:               true

            ExclusiveGroup { id: panelActionGroup }

            Column {
                id:                     buttonColumn
                anchors.leftMargin:     _margins
                anchors.rightMargin:    _margins
                anchors.left:           parent.left
                anchors.right:          parent.right
                anchors.topMargin:      _margins
                anchors.top:            parent.top
                spacing:                ScreenTools.defaultFontPixelHeight * 0.5

                QGCLabel {
                    text:           qsTr("Preferences")
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                QGCButton {
                    id:             _generalButton
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("General")
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "GeneralSettings.qml") {
                            __rightPanel.source = "GeneralSettings.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Comm Links")
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "LinkSettings.qml") {
                            __rightPanel.source = "LinkSettings.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Offline Maps")
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "OfflineMap.qml") {
                            __rightPanel.source = "OfflineMap.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("MavLink")
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "MavlinkSettings.qml") {
                            __rightPanel.source = "MavlinkSettings.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Console")
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "QGroundControl/Controls/AppMessages.qml") {
                            __rightPanel.source = "QGroundControl/Controls/AppMessages.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Mock Link")
                    visible:        ScreenTools.isDebug
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "MockLink.qml") {
                            __rightPanel.source = "MockLink.qml"
                        }
                        checked = true
                    }
                }

                QGCButton {
                    height:         _buttonHeight
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    text:           qsTr("Debug")
                    visible:        ScreenTools.isDebug
                    exclusiveGroup: panelActionGroup
                    onClicked: {
                        if(__rightPanel.source != "DebugWindow.qml") {
                            __rightPanel.source = "DebugWindow.qml"
                        }
                        checked = true
                    }
                }
            }
        }
    }

    //-- Vertical Separator
    Rectangle {
        id:             __verticalSeparator
        width:          1
        height:         parent.height - toolBar.height
        anchors.left:   __leftMenu.right
        anchors.bottom: parent.bottom
        color:          QGroundControl.isDarkStyle ? "#909090" : "#7f7f7f"
    }

    //-- Main Setting Display Area
    Rectangle {
        id:             settingDisplayArea
        anchors.left:   __verticalSeparator.right
        width:          mainWindow.width - __leftMenu.width - __verticalSeparator.width
        height:         parent.height - toolBar.height - __topSeparator.height
        anchors.bottom: parent.bottom
        visible:        __rightPanel.source != ""
        color:          qgcPal.window
        //-- Panel Contents
        Loader {
            id:             __rightPanel
            anchors.fill:   parent
        }
    }
}
