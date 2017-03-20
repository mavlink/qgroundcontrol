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

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Rectangle {
    id:         toolBar
    color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property bool _communicationLost:   _activeVehicle ? _activeVehicle.connectionLost : false

    signal showSettingsView
    signal showSetupView
    signal showPlanView
    signal showFlyView
    signal showAnalyzeView

    function checkSettingsButton() {

    }

    function checkSetupButton() {

    }

    function checkPlanButton() {

    }

    function checkFlyButton() {

    }

    function checkAnalyzeButton() {

    }

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        homeButton.checked = true
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

    // Easter egg mechanism
    MouseArea {
        anchors.fill: parent
        onClicked: {
            console.log("easter egg click", ++_clickCount)
            eggTimer.restart()
            if (_clickCount == 5) {
                QGroundControl.corePlugin.showAdvancedUI = true
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
    }

    Row {
        anchors.bottomMargin:   1
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 2
        anchors.fill:           parent
        spacing:                ScreenTools.defaultFontPixelWidth * 2.25

        ExclusiveGroup { id: mainActionGroup }

        QGCToolBarButton {
            id:                 homeButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/typhoonh/Home.svg"
            onClicked:          toolBar.showFlyView()
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/ModeIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
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
            source:             "/typhoonh/WIFIRSSIIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/BatteryIndicator.qml"
        }

        QGCToolBarButton {
            id:                 setupButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Gears.svg"
            onClicked:          toolBar.showSetupView()
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/toolbar/MessageIndicator.qml"
        }

        QGCToolBarButton {
            id:                 planButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Plan.svg"
            onClicked:          toolBar.showPlanView()
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/MissionIndicator.qml"
        }

    }

    Item {
        anchors.right:          parent.right
        height:                 parent.height * 0.33
        width:                  logoImage.width
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.66
        anchors.verticalCenter: parent.verticalCenter
        Image {
            id:                 logoImage
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            fillMode:           Image.PreserveAspectFit
            source:             _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
            property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
            property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length !== 0
            property string _brandImageIndoor:      _corePluginBranding ? QGroundControl.corePlugin.brandImageIndoor  : (_activeVehicle ? _activeVehicle.brandImageIndoor : "")
            property string _brandImageOutdoor:     _corePluginBranding ? QGroundControl.corePlugin.brandImageOutdoor : (_activeVehicle ? _activeVehicle.brandImageOutdoor : "")
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
