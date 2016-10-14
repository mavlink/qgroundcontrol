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

Rectangle {
    id:     settingsView
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    readonly property real _defaultTextHeight:  ScreenTools.defaultFontPixelHeight
    readonly property real _defaultTextWidth:   ScreenTools.defaultFontPixelWidth
    readonly property real _horizontalMargin:   _defaultTextWidth / 2
    readonly property real _verticalMargin:     _defaultTextHeight / 2
    readonly property real _buttonHeight:       ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight * 2
    readonly property real _buttonWidth:        ScreenTools.defaultFontPixelWidth * 10

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        //-- Default to General Settings
        __rightPanel.source = "GeneralSettings.qml"
        _generalButton.checked = true
        panelActionGroup.current = _generalButton
    }

    QGCFlickable {
        id:                 buttonList
        width:              buttonColumn.width
        anchors.topMargin:  _verticalMargin
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.left:       parent.left
        contentHeight:      buttonColumn.height + _verticalMargin
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        ExclusiveGroup { id: panelActionGroup }

        Column {
            id:         buttonColumn
            width:      _maxButtonWidth
            spacing:    _verticalMargin

            property real _maxButtonWidth: 0

            Component.onCompleted: reflowWidths()

            function reflowWidths() {
                buttonColumn._maxButtonWidth = 0
                for (var i = 0; i < children.length; i++) {
                    buttonColumn._maxButtonWidth = Math.max(buttonColumn._maxButtonWidth, children[i].width)
                }
                for (var j = 0; j < children.length; j++) {
                    children[j].width = buttonColumn._maxButtonWidth
                }
            }

            QGCLabel {
                anchors.left:           parent.left
                anchors.right:          parent.right
                text:                   qsTr("Application Settings")
                wrapMode:               Text.WordWrap
                horizontalAlignment:    Text.AlignHCenter
                visible:                !ScreenTools.isShortScreen
            }

            QGCButton {
                id:             _generalButton
                height:         _buttonHeight
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

    Rectangle {
        id:                     divider
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.left:           buttonList.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        width:                  1
        color:                  qgcPal.windowShade
    }

    //-- Panel Contents
    Loader {
        id:                     __rightPanel
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
    }
}

