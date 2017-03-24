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
        settingsButton.checked = true
    }

    function checkSetupButton() {
        setupButton.checked = true
    }

    function checkPlanButton() {
        planButton.checked = true
    }

    function checkFlyButton() {
        homeButton.checked = true
    }

    function checkAnalyzeButton() {

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

    ExclusiveGroup { id: mainActionGroup }

    QGCToolBarButton {
        id:                 homeButton
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.leftMargin: 10
        exclusiveGroup:     mainActionGroup
        source:             "/typhoonh/Home.svg"
        onClicked:          toolBar.showFlyView()
    }

    Row {
        id:                     mainRow
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.bottomMargin:   1
        anchors.left:           homeButton.right
        anchors.leftMargin:     30
        spacing:                30 //-- Hard coded to fit the ST16 Screen

        QGCToolBarButton {
            id:                 setupButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Gears.svg"
            onClicked:          toolBar.showSetupView()
        }

        QGCToolBarButton {
            id:                 planButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Plan.svg"
            onClicked:          toolBar.showPlanView()
        }

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
            source:             "/typhoonh/MissionIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/toolbar/MessageIndicator.qml"
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
            source:             "/typhoonh/YGPSIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/BatteryIndicator.qml"
        }

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
            source:             "/typhoonh/RCIndicator.qml"
        }

        Loader {
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
            source:             "/typhoonh/WIFIRSSIIndicator.qml"
        }

    }

    Item {
        width:              logoImage.width
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.right:      parent.right
        anchors.rightMargin: 10
        anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
        Image {
            id:             logoImage
            height:         parent.height * 0.45
            fillMode:       Image.PreserveAspectFit
            source:         _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
            anchors.verticalCenter: parent.verticalCenter
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
