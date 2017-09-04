/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.3
import QtQuick.Layouts      1.2
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2
import QtPositioning        5.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0
import QGroundControl.FlightMap             1.0

import TyphoonHQuickInterface               1.0

Rectangle {
    id:         toolBar
    color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.95) : Qt.rgba(0,0,0,0.75)
    visible:    !QGroundControl.videoManager.fullScreen

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle

    signal showSettingsView
    signal showSetupView
    signal showPlanView
    signal showFlyView
    signal showAnalyzeView
    signal armVehicle
    signal disarmVehicle

    function checkSettingsButton() {
        settingsButton.checked = true
    }

    function checkSetupButton() {
        setupButton.checked = true
    }

    function checkPlanButton() {
        TyphoonHQuickInterface.stopScan();
        planButton.checked = true
    }

    function checkFlyButton() {
        TyphoonHQuickInterface.stopScan();
        homeButton.checked = true
    }

    function checkAnalyzeButton() {
        TyphoonHQuickInterface.stopScan();
    }

    Component.onCompleted: {
        homeButton.checked = true
    }

    /// Bottom single pixel divider
    Rectangle {
        anchors.left:   parent.left
        anchors.right:  parent.right
        anchors.bottom: parent.bottom
        height:         1
        color:          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.35) : Qt.rgba(1,1,1,0.35)
    }

    ExclusiveGroup { id: mainActionGroup }

    //---------------------------------------------
    // Left Toolbar Row
    Row {
        anchors.bottomMargin:   1
        anchors.left:           parent.left
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth * (ScreenTools.isMobile ? 5.5 : 3.25)

        QGCToolBarButton {
            id:                 homeButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/typhoonh/img/Y-Logo.svg"
            onClicked:          toolBar.showFlyView()
        }

        QGCToolBarButton {
            id:                 planButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/typhoonh/img/waypoint.svg"
            onClicked:          toolBar.showPlanView()
        }

        QGCToolBarButton {
            id:                 setupButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/typhoonh/img/hamburger.svg"

            onClicked: {
                toolBar.showSetupView()
                // Easter egg mechanism
                _clickCount++
                eggTimer.restart()
                if (_clickCount == 5 && !QGroundControl.corePlugin.showAdvancedUI) {
                    advancedModeConfirmation.visible = true
                } else if (_clickCount == 7) {
                    QGroundControl.corePlugin.showTouchAreas = true
                }
            }

            property int _clickCount: 0

            Timer {
                id:             eggTimer
                interval:       1000
                onTriggered:    parent._clickCount = 0
            }

            MessageDialog {
                id:                 advancedModeConfirmation
                title:              qsTr("Advanced Mode")
                text:               QGroundControl.corePlugin.showAdvancedUIMessage
                standardButtons:    StandardButton.Yes | StandardButton.No

                onYes: {
                    QGroundControl.corePlugin.showAdvancedUI = true
                    visible = false
                }
            }
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/toolbar/MessageIndicator.qml"
        }

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

    }

    //-- Mode Indicator
    Loader {
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
        anchors.horizontalCenter: parent.horizontalCenter
        source:             "/typhoonh/ModeIndicator.qml"
    }

    //---------------------------------------------
    // Right Toolbar Row
    Row {
        anchors.bottomMargin:   1
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth * 2
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth * 2

        Rectangle {
            height:             parent.height * 0.75
            width:              1
            color:              qgcPal.text
            opacity:            0.5
            anchors.verticalCenter: parent.verticalCenter
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            visible:            ScreenTools.isMobile
            source:             "/typhoonh/RCIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/YGPSIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/BatteryIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            visible:            ScreenTools.isMobile
            source:             "/typhoonh/WIFIRSSIIndicator.qml"
        }

        Image {
            source:             qgcPal.globalTheme === QGCPalette.Light ? "/typhoonh/img/YuneecBrandImageBlack.svg" : "/typhoonh/img/YuneecBrandImage.svg"
            visible:            !ScreenTools.isMobile
            height:             parent.height * 0.35
            sourceSize.height:  height
            fillMode:           Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
        }

    }

    // Progress bar
    Rectangle {
        id:             progressBar
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          _activeVehicle ? _activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
    }

}
