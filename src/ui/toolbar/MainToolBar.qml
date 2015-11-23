/*=====================================================================

QGroundControl Open Source Ground Control Station

(c) 2009, 2015 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>

This file is part of the QGROUNDCONTROL project

    QGROUNDCONTROL is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    QGROUNDCONTROL is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with QGROUNDCONTROL. If not, see <http://www.gnu.org/licenses/>.

======================================================================*/

/**
 * @file
 *   @brief QGC Main Tool Bar
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick 2.5
import QtQuick.Controls 1.2
import QtQuick.Controls.Styles 1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Rectangle {
    id:     toolBar
    color:  opaqueBackground ? "#404040" : (isBackgroundDark ? Qt.rgba(0,0,0,0.75) : Qt.rgba(0,0,0,0.5))

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var  activeVehicle:        multiVehicleManager.activeVehicle
    property var  mainWindow:           null
    property bool isMessageImportant:   activeVehicle ? !activeVehicle.messageTypeNormal && !activeVehicle.messageTypeNone : false
    property bool isBackgroundDark:     true
    property bool opaqueBackground:     false

    property string formatedMessage:    activeVehicle ? activeVehicle.formatedMessage : ""

    /*
        Dev System (Mac OS)

        qml: Main Window Width:   1008
        qml: Toolbar height:      51.2
        qml: Default font:        12.8
        qml: Font (.75):          9.600000000000001
        qml: Font (.85):          10.88
        qml: Font 1.5):           19.200000000000003
        qml: Default Font Width:  8.328125
        qml: Default Font Height: 12.8
        qml: --
        qml: Real Font Height:    16
        qml: fontHRatio:          1
        qml: --
        qml: cellHeight:          38
        qml: tbFontSmall:         10
        qml: tbFontNormal:        12
        qml: tbFontLarge:         18
        qml: tbSpacing:           9.54

        Nexus 9

        qml: Main Window Width:   2048
        qml: Toolbar height:      90.9312
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.0625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          68
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           16.87552

        Nexus 7

        qml: Main Window Width:   1920
        qml: Toolbar height:      85.248
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.140625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          63
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           15.820800000000002

        Nexus 4

        qml: Main Window Width:   1196
        qml: Toolbar height:      79.65360000000001
        qml: Default font:        38
        qml: Font (.75):          28.5
        qml: Font (.85):          32.3
        qml: Font 1.5):           57
        qml: Default Font Width:  20.140625
        qml: Default Font Height: 38
        qml: --
        qml: Real Font Height:    38
        qml: fontHRatio:          2.375
        qml: --
        qml: cellHeight:          59
        qml: tbFontSmall:         23.75
        qml: tbFontNormal:        28.5
        qml: tbFontLarge:         42.75
        qml: tbSpacing:           9.85504

    */

    readonly property real  tbFontSmall:    10 * ScreenTools.fontHRatio
    readonly property real  tbFontNormal:   12 * ScreenTools.fontHRatio
    readonly property real  tbFontLarge:    18 * ScreenTools.fontHRatio

    readonly property var   colorGreen:     "#05f068"
    readonly property var   colorOrange:    "#f0ab06"
    readonly property var   colorRed:       "#fc4638"
    readonly property var   colorGrey:      "#7f7f7f"
    readonly property var   colorBlue:      "#636efe"
    readonly property var   colorWhite:     "#ffffff"

    MainToolBarController { id: _controller }

    onFormatedMessageChanged: {
        if(messageArea.visible) {
            messageText.append(formatedMessage)
            //-- Hack to scroll down
            messageFlick.flick(0,-500)
        }
    }

    function showMessageArea() {
        if(multiVehicleManager.activeVehicleAvailable) {
            messageText.text = activeVehicle.formatedMessages
            //-- Hack to scroll to last message
            for (var i = 0; i < activeVehicle.messageCount; i++)
                messageFlick.flick(0,-5000)
            activeVehicle.resetMessages()
        } else {
            messageText.text = "No Messages"
        }
        messageArea.visible = true
        mainWindow.setMapInteractive(false)
    }

    function showToolbarMessage(message) {
        toolBarMessage.text = message
        toolBarMessageArea.visible = true
    }

    function showMavStatus() {
         return (multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout === 0);
    }

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        flyButton.checked = true
    }

    Connections {
        target:         controller
        onShowFlyView:  { flyButton.checked   = true }
        onShowPlanView: { planButton.checked  = true }
        onShowSetupView:{ setupButton.checked = true }
    }

    Row {
        id:             viewRow
        height:         mainWindow.tbCellHeight
        spacing:        mainWindow.tbSpacing
        anchors.left:   parent.left
        anchors.leftMargin:     mainWindow.tbSpacing
        anchors.verticalCenter: parent.verticalCenter

        ExclusiveGroup { id: mainActionGroup }

        QGCToolBarButton {
            id:                 preferencesButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            source:             "/qmlimages/Hamburger.svg"
            onClicked: {
                mainWindow.showLeftMenu();
                preferencesButton.checked = false;
            }
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 setupButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Gears.svg"
            onClicked: {
                _controller.onSetupView();
            }
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 planButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Plan.svg"
            onClicked: {
                _controller.onPlanView();
            }
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

        QGCToolBarButton {
            id:                 flyButton
            width:              mainWindow.tbButtonWidth
            height:             mainWindow.tbCellHeight
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/PaperPlane.svg"
            onClicked: {
                _controller.onFlyView();
            }
        }

        Rectangle {
            height: mainWindow.tbCellHeight
            width:  1
            color: Qt.rgba(1,1,1,0.45)
        }

    }

    Item {
        id:                     vehicleIndicators
        visible:                showMavStatus() && !connectionStatus.visible
        height:                 mainWindow.tbCellHeight
        width:                  (toolBar.width - viewRow.width)
        anchors.left:           viewRow.right
        anchors.leftMargin:     mainWindow.tbSpacing * 2
        anchors.verticalCenter: parent.verticalCenter
        Loader {
            source:             multiVehicleManager.activeVehicleAvailable ? "MainToolBarIndicators.qml" : ""
            anchors.left:       parent.left
            anchors.verticalCenter:   parent.verticalCenter
        }
    }

    //-------------------------------------------------------------------------
    //-- System Message Area
    Rectangle {
        id:             messageArea
        width:          mainWindow.width  * 0.5
        height:         mainWindow.height * 0.5
        anchors.top:    parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        color:          Qt.rgba(0,0,0,0.75)
        visible:        false
        radius:         ScreenTools.defaultFontPixelHeight * 0.5
        Flickable {
            id:                 messageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            contentHeight:      messageText.height
            contentWidth:       messageText.width
            boundsBehavior:     Flickable.StopAtBounds
            pixelAligned:       true
            clip:               true
            TextEdit {
                id:         messageText
                readOnly:   true
                textFormat: TextEdit.RichText
                color:      "white"
            }
        }
        //-- Dismiss System Message
        Image {
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.top:        parent.top
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelHeight * 1.5
            height:             ScreenTools.defaultFontPixelHeight * 1.5
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    messageText.text    = ""
                    messageArea.visible = false
                    mainWindow.setMapInteractive(true)
                }
            }
        }
    }

    QGCLabel {
        id:             connectionStatus
        visible:        (_controller.connectionCount > 0 && multiVehicleManager.activeVehicleAvailable && activeVehicle.heartbeatTimeout != 0)
        text:           "CONNECTION LOST"
        font.pixelSize: tbFontLarge
        font.weight:    Font.DemiBold
        color:          colorRed
        anchors.left:           viewRow.right
        anchors.leftMargin:     mainWindow.tbSpacing * 2
        anchors.verticalCenter: parent.verticalCenter
    }

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          parent.width * _controller.progressBarValue
        color:          colorGreen
    }

    // Toolbar message area
    Rectangle {
        id:             toolBarMessageArea
        x:              toolBar.parent.width * 0.225
        y:              toolBar.parent.height - (ScreenTools.defaultFontPixelHeight * ScreenTools.fontHRatio * 6)
        width:          toolBar.parent.width * 0.55
        height:         ScreenTools.defaultFontPixelHeight * ScreenTools.fontHRatio * 6
        color:          Qt.rgba(0,0,0,0.65)
        visible:        false
        ScrollView {
            width:              toolBarMessageArea.width - toolBarMessageCloseButton.width
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            frameVisible:       false
            horizontalScrollBarPolicy:  Qt.ScrollBarAlwaysOff
            verticalScrollBarPolicy:    Qt.ScrollBarAlwaysOff
            QGCLabel {
                id:                 toolBarMessage
                width:              toolBarMessageArea.width - toolBarMessageCloseButton.width
                wrapMode:           Text.WordWrap
                color:              "#e4e428"
                lineHeightMode:     Text.ProportionalHeight
                lineHeight:         1.15
                anchors.margins:    mainWindow.tbSpacing
            }
        }
        QGCButton {
            id:                 toolBarMessageCloseButton
            primary:            true
            text:               "Close"
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            anchors.margins:    mainWindow.tbSpacing
            onClicked: {
                toolBarMessageArea.visible = false
                _controller.onToolBarMessageClosed()
            }
        }
    }

} // Rectangle
