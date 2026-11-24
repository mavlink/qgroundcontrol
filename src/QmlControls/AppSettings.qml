/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.AppSettings

Rectangle {
    id:     settingsView
    // color:  qgcPal.window
    color:  _cBackdrop
    z:      QGroundControl.zOrderTopMost

    readonly property real _defaultTextHeight:  ScreenTools.defaultFontPixelHeight
    readonly property real _defaultTextWidth:   ScreenTools.defaultFontPixelWidth
    readonly property real _horizontalMargin:   _defaultTextWidth / 2
    readonly property real _verticalMargin:     _defaultTextHeight / 2
    readonly property real _buttonHeight:       ScreenTools.isTinyScreen ? ScreenTools.defaultFontPixelHeight * 3 : ScreenTools.defaultFontPixelHeight * 2

    property bool _first: true

    // Theme colors (light and dark follow the Color Scheme setting)
    readonly property bool  _light:         qgcPal.globalTheme === QGCPalette.Light
    readonly property color _cContent:      _light ? "#FFFFFF" : "#202020"
    readonly property color _cBackdrop:     _light ? "#D9DADF" : "#000000"
    readonly property color _cSidebar:      _light ? "#ECEDF0" : "#404040"
    readonly property color _cLine:         _light ? "#CDCED4" : "#404040"

    readonly property color _cItemText:     _light ? "#1F2328" : "#E6EAF0"
    readonly property color _cItemHover:    _light ? "#E0E1E5" : "#222D3A"
    readonly property color _cItemSelected: _light ? "#D1D2D8" : "#2E3B4B"
    readonly property real  _frameMargin:   16
    readonly property real  _frameRadius:   18
    readonly property real  _sidebarW:      ScreenTools.defaultFontPixelWidth * 26

    // Back button only when this is the full-screen Application Settings page (not embedded in the footer)
    readonly property bool showBackButton: mainWindow.settingsPageOpen

    property bool _commingFromRIDSettings:  false

    ButtonGroup {
        id: settingsButtonGroup
    }

    function showSettingsPage(settingsPage) {
        for (var i = 0; i < settingsButtonGroup.buttons.length; i++) {
            var button = settingsButtonGroup.buttons[i]
            if (button.text === settingsPage) {
                button.clicked()
                break
            }
        }
    }

    // This need to block click event leakage to underlying map.
    DeadMouseArea {
        anchors.fill: parent
    }

    QGCPalette { id: qgcPal }

    Component.onCompleted: {
        //-- Default Settings
        if (globals.commingFromRIDIndicator) {
            rightPanel.source = "qrc:/qml/QGroundControl/AppSettings/RemoteIDSettings.qml"
            globals.commingFromRIDIndicator = false
        } else {
            rightPanel.source =  "qrc:/qml/QGroundControl/AppSettings/GeneralSettings.qml"
        }
    }


    SettingsPagesModel { id: settingsPagesModel }

    // Rounded page frame
    Rectangle {
        id:                 frame
        anchors.fill:       parent
        anchors.margins:    _frameMargin
        radius:             _frameRadius
        color:              _cContent
        border.color:       _cLine
        border.width:       1
    }

    // Sidebar background: rounded on the outer corners, square on the inner edge
    Rectangle {
        id:                 sidebarBg
        anchors.left:       frame.left
        anchors.top:        frame.top
        anchors.bottom:     frame.bottom
        anchors.margins:    1
        width:              _sidebarW
        radius:             _frameRadius - 1
        color:              _cSidebar
    }
    Rectangle {
        anchors.right:      sidebarBg.right
        anchors.top:        sidebarBg.top
        anchors.bottom:     sidebarBg.bottom
        width:              _frameRadius
        color:              _cSidebar
    }
    Rectangle {
        id:                 sidebarLine
        anchors.left:       sidebarBg.right
        anchors.top:        sidebarBg.top
        anchors.bottom:     sidebarBg.bottom
        width:              1
        color:              _cLine
    }

    // Back button at the top of the sidebar (will replace the "Exit Application Settings" bar)
    // Back button at the top of the sidebar
    Item {
        id:                 backRow
        visible:            settingsView.showBackButton
        anchors.top:        frame.top
        anchors.topMargin:  _frameMargin
        anchors.left:       frame.left
        anchors.leftMargin: 12
        width:              _sidebarW - 24
        height:             _buttonHeight

        Rectangle {
            anchors.fill:   parent
            radius:         10
            color:          backMouse.containsMouse ? settingsView._cItemHover : "transparent"
            // opacity:        backMouse.containsMouse ? 0.2 : 0
        }

        RowLayout {
            anchors.fill:       parent
            anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.75
            spacing:            ScreenTools.defaultFontPixelWidth

            // QGCLabel {
            //     text:           "‹"
            //     font.pointSize: ScreenTools.defaultFontPointSize * 1.9
            // }

            Item {
                Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 0.6
                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 0.9
                Layout.alignment:       Qt.AlignVCenter

                Canvas {
                    property color arrowColor: settingsView._cItemText
                    onArrowColorChanged: requestPaint()
                    anchors.fill: parent
                    onPaint: {
                        var ctx = getContext("2d")
                        ctx.reset()
                        ctx.lineWidth   = 2
                        ctx.lineCap     = "round"
                        ctx.lineJoin    = "round"
                        ctx.strokeStyle = arrowColor
                        ctx.beginPath()
                        ctx.moveTo(width - 2, 2)
                        ctx.lineTo(2, height / 2)
                        ctx.lineTo(width - 2, height - 2)
                        ctx.stroke()
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth:   true
                Layout.alignment: Qt.AlignVCenter
                verticalAlignment: Text.AlignVCenter
                text:               qsTr("Exit Settings")
            }
        }

        MouseArea {
            id:                 backMouse
            anchors.fill:       parent
            hoverEnabled:       true
            cursorShape:        Qt.PointingHandCursor
            onClicked: {
                // console.log("BACK clicked | exitTool is:", typeof mainWindow.exitTool)
                if (mainWindow.allowViewSwitch()) {
                    mainWindow.exitTool()
                }
            }
        }
    }

    Flickable {
        id:                 buttonList
        width:              _sidebarW - 24
        anchors.topMargin:  _verticalMargin
        anchors.top:        showBackButton ? backRow.bottom : frame.top
        anchors.bottom:     frame.bottom
        anchors.bottomMargin: _frameMargin
        anchors.leftMargin: 12
        anchors.left:       frame.left
        contentHeight:      buttonColumn.height + _verticalMargin
        flickableDirection: Flickable.VerticalFlick
        clip:               true
        boundsBehavior:     Flickable.StopAtBounds

        // Subtle slim scrollbar matching modern sidebar design
        ScrollBar.vertical: ScrollBar {
            id:             vScrollBar
            parent:         buttonList.parent
            anchors.top:    buttonList.top
            anchors.bottom: buttonList.bottom
            anchors.right:  sidebarLine.left
            anchors.rightMargin: 1
            width:          4
            policy:         ScrollBar.AsNeeded
            visible:        buttonList.contentHeight > buttonList.height

            contentItem: Rectangle {
                implicitWidth:  4
                radius:         2
                color:          settingsView._light ? "#C0C4CC" : "#4B5563"
                opacity:        vScrollBar.active || vScrollBar.hovered ? 0.8 : 0.2
                Behavior on opacity { NumberAnimation { duration: 150 } }
            }
        }

        ColumnLayout {
            id:         buttonColumn
            spacing:    6
            width:      buttonList.width

            property real _maxButtonWidth: 0

            Repeater {
                id:     buttonRepeater
                model:  settingsPagesModel

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 4
                    visible: pageVisible()

                    // Section header (e.g. Flight, Connections, Developer)
                    Item {
                        Layout.fillWidth:       true
                        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.5
                        visible:                model.category !== undefined && model.category !== "" && 
                                                (index === 0 || settingsPagesModel.get(index - 1).category !== model.category)

                        QGCLabel {
                            anchors.left:           parent.left
                            anchors.leftMargin:     ScreenTools.defaultFontPixelWidth * 0.5
                            anchors.bottom:         parent.bottom
                            anchors.bottomMargin:   2
                            text:                   model.category !== undefined ? model.category : ""
                            font.pointSize:         ScreenTools.defaultFontPointSize * 0.85
                            font.bold:              true
                            color:                  settingsView._light ? "#6B7280" : "#9CA3AF"
                        }
                    }

                    SettingsButton {
                        id:                 sideBtn
                        ButtonGroup.group:  settingsButtonGroup
                        Layout.fillWidth:   true
                        text:               name
                        icon.source:        iconUrl
                        padding:            ScreenTools.defaultFontPixelWidth * 0.75
                        textColor:          settingsView._cItemText

                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                if (rightPanel.source !== url) {
                                    rightPanel.source = url
                                }
                                checked = true
                            }
                        }

                        Component.onCompleted: {
                            if (globals.commingFromRIDIndicator) {
                                _commingFromRIDSettings = true
                            }
                            if(_first) {
                                _first = false
                                checked = true
                            }
                            if (_commingFromRIDSettings) {
                                checked = false
                                _commingFromRIDSettings = false
                                if (url == "qrc:/qml/QGroundControl/AppSettings/RemoteIDSettings.qml") {
                                    checked = true
                                }
                            }
                        }

                        background: Rectangle {
                            radius: 10
                            color:  sideBtn.checked || sideBtn.pressed ? (settingsView._light ? "#007AFF" : settingsView._cItemSelected)
                                    : sideBtn.hovered ? settingsView._cItemHover
                                    : "transparent"
                        }

                        contentItem: RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth

                            // Rounded icon badge with custom background color
                            Rectangle {
                                Layout.preferredWidth:  ScreenTools.defaultFontPixelHeight * 1.35
                                Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.35
                                Layout.alignment:       Qt.AlignVCenter
                                radius:                 7
                                color:                  model.badgeColor ? model.badgeColor : "#6B7280"

                                QGCColoredImage {
                                    anchors.centerIn:   parent
                                    source:             sideBtn.icon.source
                                    color:              "#FFFFFF"
                                    width:              parent.width * 0.62
                                    height:             parent.height * 0.62
                                    fillMode:           Image.PreserveAspectFit
                                }
                            }

                            QGCLabel {
                                Layout.fillWidth:    true
                                Layout.alignment:    Qt.AlignVCenter
                                text:                sideBtn.text
                                color:               (sideBtn.checked && settingsView._light) ? "#FFFFFF" : sideBtn.textColor
                                font.pointSize:      ScreenTools.defaultFontPointSize * 0.95
                                horizontalAlignment: QGCLabel.AlignLeft
                            }
                        }
                    }
                }
            }
        }
    }

    // Rectangle {
    //     id:                     divider
    //     anchors.topMargin:      _verticalMargin
    //     anchors.bottomMargin:   _verticalMargin
    //     anchors.leftMargin:     _horizontalMargin
    //     anchors.left:           buttonList.right
    //     anchors.top:            parent.top
    //     anchors.bottom:         parent.bottom
    //     width:                  1
    //     color:                  qgcPal.windowShade
    // }

    //-- Panel Contents
    Loader {
        id:                     rightPanel
        // anchors.leftMargin:     _horizontalMargin
        // anchors.rightMargin:    _horizontalMargin
        // anchors.topMargin:      _verticalMargin
        // anchors.bottomMargin:   _verticalMargin
        anchors.margins: 6
        anchors.left:           sidebarLine.right
        anchors.right:          frame.right
        anchors.top:            frame.top
        anchors.bottom:         frame.bottom
    }
}

