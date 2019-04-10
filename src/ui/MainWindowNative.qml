/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Window   2.2
import QtQuick.Dialogs  1.2
import QtQuick.Controls 2.2
import QtQuick.Layouts  1.0

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0

/// Native QML top level window
ApplicationWindow {
    id:         mainWindow
    width:      1024
    height:     768
    visible:    true

    onClosing: {
        //if (!_forceClose) {
        //    mainWindowInner.item.attemptWindowClose()
        //    close.accepted = false
        //}
    }

    Component.onCompleted: {
        toolbarIndicators.source = _toolbarIndicators
    }

    property bool   _forceClose:            false

    readonly property real      _topBottomMargins:      ScreenTools.defaultFontPixelHeight * 0.5
    readonly property string    _settingsViewSource:    "AppSettings.qml"
    readonly property string    _setupViewSource:       "SetupView.qml"
    readonly property string    _planViewSource:        "PlanView.qml"
    readonly property string    _toolbarIndicators:     "/toolbar/MainToolBarIndicators.qml"
    readonly property string    _analyzeViewSource:     !ScreenTools.isMobile ? "AnalyzeView.qml" : "MavlinkConsolePage.qml"

    //-------------------------------------------------------------------------
    //-- Global Scope Variables

    property var                activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    readonly property string    navButtonWidth:         ScreenTools.defaultFontPixelWidth * 24

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    //-------------------------------------------------------------------------
    //-- Global Scope Functions
    function showMessage(message) {

    }

    function disableToolbar() {
        drawer.enabled = false
    }

    function enableToolbar() {
        drawer.enabled = true
    }

    function viewSwitch(isPlanView) {
        enableToolbar()
        drawer.close()
        if(isPlanView) {

        } else {
            if(toolbarIndicators.source !== _toolbarIndicators) {
                toolbarIndicators.source = _toolbarIndicators
            }
        }
    }

    function showFlyView() {
        viewSwitch(false)
        mainWindowInner.source = ""
        if(toolbarIndicators.source !== _toolbarIndicators) {
            toolbarIndicators.source = _toolbarIndicators
        }
    }

    function showPlanView() {
        viewSwitch(true)
        if (mainWindowInner.source !== _planViewSource) {
            mainWindowInner.source  = _planViewSource
        }
    }

    function showAnalyzeView() {
        viewSwitch(false)
        if (mainWindowInner.source !== _analyzeViewSource) {
            mainWindowInner.source  = _analyzeViewSource
        }
    }

    function showSetupView() {
        viewSwitch(false)
        if (mainWindowInner.source !== _setupViewSource) {
            mainWindowInner.source  = _setupViewSource
        }
    }

    function showSettingsView() {
        viewSwitch(false)
        if (mainWindowInner.source !== _settingsViewSource) {
            mainWindowInner.source  = _settingsViewSource
        }
    }

    //-------------------------------------------------------------------------
    //-- Main, full window background
    background: Item {
        id:             rootBackground
        anchors.fill:   parent
        FlightDisplayView {
            id:             flightView
            anchors.fill:   parent
        }
    }

    //-------------------------------------------------------------------------
    //-- Global ToolBar
    header: ToolBar {
        height:         ScreenTools.toolbarHeight
        visible:        !QGroundControl.videoManager.fullScreen
        background:     Rectangle {
            color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)
        }
        RowLayout {
            anchors.fill:               parent
            Rectangle {
                height:                 parent.height
                width:                  height
                color:                  qgcPal.brandingPurple
                QGCColoredImage {
                    anchors.centerIn:       parent
                    height:                 ScreenTools.defaultFontPixelHeight * 2
                    width:                  height
                    sourceSize.height:      parent.height
                    fillMode:               Image.PreserveAspectFit
                    source:                 "/res/QGCLogoWhite"
                    color:                  "white"
                }
                MouseArea {
                    anchors.fill:       parent
                    onClicked:{
                        if(drawer.visible) {
                            drawer.close()
                        } else {
                            drawer.open()
                        }
                    }
                }
            }
            Loader {
                id:                 toolbarIndicators
                height:             parent.height
                Layout.fillWidth:   true
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Navigation Drawer (Left to Right, on command or using touch gestures)
    Drawer {
        id:         drawer
        y:          header.height
        width:      navButtonWidth
        height:     mainWindow.height - header.height
        background: Rectangle {
            color:  qgcPal.globalTheme === QGCPalette.Light ? "white" : "black"
        }
        ButtonGroup {
            buttons: buttons.children
        }
        ColumnLayout {
            id:                     buttons
            anchors.top:            parent.top
            anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:           parent.left
            anchors.right:          parent.right
            spacing:                ScreenTools.defaultFontPixelHeight * 0.5
            QGCToolBarButton {
                text:               "Fly"
                icon.source:        "/qmlimages/PaperPlane.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showFlyView()
                }
            }
            QGCToolBarButton {
                text:               "Plan"
                icon.source:        "/qmlimages/Plan.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showPlanView()
                }
            }
            QGCToolBarButton {
                text:               "Analyze"
                icon.source:        "/qmlimages/Analyze.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showAnalyzeView()
                }
            }
            QGCToolBarButton {
                text:               "Vehicle Setup"
                icon.source:       "/qmlimages/Gears.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showSetupView()
                }
            }
            QGCToolBarButton {
                text:               "Settings"
                icon.source:        "/qmlimages/Gears.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showSettingsView()
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Current content
    Loader {
        id:             mainWindowInner
        anchors.fill:   parent
    }
}

