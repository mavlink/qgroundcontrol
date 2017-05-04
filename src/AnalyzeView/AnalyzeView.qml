/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/// @file
///     @brief Setup View
///     @author Don Gagne <don@thegagnes.com>

import QtQuick          2.3
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

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    ExclusiveGroup { id: setupButtonGroup }

    readonly property real  _defaultTextHeight:     ScreenTools.defaultFontPixelHeight
    readonly property real  _defaultTextWidth:      ScreenTools.defaultFontPixelWidth
    readonly property real  _horizontalMargin:      _defaultTextWidth / 2
    readonly property real  _verticalMargin:        _defaultTextHeight / 2
    readonly property real  _buttonWidth:           _defaultTextWidth * 18

    GeoTagController {
        id: geoController
    }

    LogDownloadController {
        id: logController
	}

    MavlinkConsoleController {
        id: conController
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
                id: buttonRepeater

                model: ListModel {
                    ListElement {
                        buttonImage:        "/qmlimages/LogDownloadIcon"
                        buttonText:         qsTr("Log Download")
                        pageSource:         "LogDownloadPage.qml"
                    }
                    ListElement {
                        buttonImage:        "/qmlimages/GeoTagIcon"
                        buttonText:         qsTr("GeoTag Images")
                        pageSource:         "GeoTagPage.qml"
                    }
                    ListElement {
                        buttonImage:        "/qmlimages/MavlinkConsoleIcon"
                        buttonText:         qsTr("Mavlink Console")
                        pageSource:         "MavlinkConsolePage.qml"
                    }
                }

                Component.onCompleted: itemAt(0).checked = true

                SubMenuButton {
                    imageResource:      buttonImage
                    setupIndicator:     false
                    exclusiveGroup:     setupButtonGroup
                    text:               buttonText
                    onClicked:          panelLoader.source = pageSource
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
