/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Setup View
///     @author Don Gagne <don@thegagnes.com>

import QtQuick          2.3
import QtQuick.Window   2.2
import QtQuick.Controls 1.2

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.ScreenTools   1.0

Rectangle {
    id:     setupView
    color:  qgcPal.window
    z:      QGroundControl.zOrderTopMost

    ExclusiveGroup { id: setupButtonGroup }

    readonly property real  _defaultTextHeight:     ScreenTools.defaultFontPixelHeight
    readonly property real  _defaultTextWidth:      ScreenTools.defaultFontPixelWidth
    readonly property real  _horizontalMargin:      _defaultTextWidth / 2
    readonly property real  _verticalMargin:        _defaultTextHeight / 2
    readonly property real  _buttonWidth:           _defaultTextWidth * 18

    property int _curIndex: 0

    GeoTagController {
        id: geoController
    }

    LogDownloadController {
        id: logController
    }

    QGCFlickable {
        id:                 buttonScroll
        width:              buttonColumn.width
        anchors.topMargin:  _defaultTextHeight / 2
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.leftMargin: _horizontalMargin
        anchors.left:       parent.left
        contentHeight:      buttonColumn.height
        flickableDirection: Flickable.VerticalFlick
        clip:               true

        Column {
            id:         buttonColumn
            width:      _maxButtonWidth
            spacing:    _defaultTextHeight / 2

            property real _maxButtonWidth: 0

            Component.onCompleted: reflowWidths()

            // I don't know why this does not work
            Connections {
                target:         QGroundControl.settingsManager.appSettings.appFontPointSize
                onValueChanged: buttonColumn.reflowWidths()
            }

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
                text:                   qsTr("Analyze")
                wrapMode:               Text.WordWrap
                horizontalAlignment:    Text.AlignHCenter
                visible:                !ScreenTools.isShortScreen
            }

            Repeater {
                id:                     buttonRepeater
                model:                  QGroundControl.corePlugin ? QGroundControl.corePlugin.analyzePages : []
                Component.onCompleted:  itemAt(0).checked = true
                SubMenuButton {
                    id:                 subMenu
                    imageResource:      modelData.icon
                    setupIndicator:     false
                    exclusiveGroup:     setupButtonGroup
                    text:               modelData.title
                    property var window:    analyzeWidgetWindow
                    property var loader:    analyzeWidgetLoader
                    onClicked: {
                        _curIndex = index
                        panelLoader.source = modelData.url
                        checked = true
                    }
                    Window {
                        id:             analyzeWidgetWindow
                        width:          ScreenTools.defaultFontPixelWidth  * 100
                        height:         ScreenTools.defaultFontPixelHeight * 40
                        visible:        false
                        title:          modelData.title
                        Rectangle {
                            color:      qgcPal.window
                            anchors.fill:  parent
                            Loader {
                                id:             analyzeWidgetLoader
                                anchors.fill:   parent
                            }
                        }
                        onClosing: {
                            analyzeWidgetWindow.visible = false
                            analyzeWidgetLoader.source = ""
                            _curIndex = index
                            panelLoader.source = modelData.url
                            subMenu.visible = true
                            subMenu.checked = true
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id:                     divider
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.left:           buttonScroll.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        width:                  1
        color:                  qgcPal.windowShade
    }

    Connections {
        target:                 panelLoader.item
        onPopout: {
            buttonRepeater.itemAt(_curIndex).window.visible = true
            var source = panelLoader.source
            panelLoader.source = ""
            buttonRepeater.itemAt(_curIndex).loader.source = source
            buttonRepeater.itemAt(_curIndex).visible = false
            buttonRepeater.itemAt(_curIndex).loader.item.poped = true
        }
    }

    Loader {
        id:                     panelLoader
        anchors.topMargin:      _verticalMargin
        anchors.bottomMargin:   _verticalMargin
        anchors.leftMargin:     _horizontalMargin
        anchors.rightMargin:    _horizontalMargin
        anchors.left:           divider.right
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        source:                 "LogDownloadPage.qml"
    }
}
