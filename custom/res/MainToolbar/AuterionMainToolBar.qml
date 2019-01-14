/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.11
import QtQuick.Layouts      1.2
import QtQuick.Controls     1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

import Auterion.Widgets                     1.0

Rectangle {
    id:         toolBar
    visible:    !QGroundControl.videoManager.fullScreen
    color:      flyButton.checked ? Qt.rgba(0,0,0,0) : Qt.rgba(0,0,0,0.75)

    Rectangle {
        anchors.top:    parent.top
        anchors.right:  parent.right
        anchors.left:   parent.left
        height:         parent.height * 1.5
        visible:        flyButton.checked
        gradient: Gradient {
            GradientStop { position: 0;    color: Qt.rgba(0,0,0,0.85) }
            GradientStop { position: 0.25; color: Qt.rgba(0,0,0,0.65) }
            GradientStop { position: 1;    color: Qt.rgba(0,0,0,0) }
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property var  _activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle

    signal showSettingsView
    signal showSetupView
    signal showPlanView
    signal showFlyView
    signal showAnalyzeView
    signal armVehicle
    signal disarmVehicle
    signal vtolTransitionToFwdFlight
    signal vtolTransitionToMRFlight

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
        flyButton.checked = true
    }

    function checkAnalyzeButton() {
        analyzeButton.checked = true
    }

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        flyButton.checked = true
    }

    // Prevent all clicks from going through to lower layers
    DeadMouseArea {
        anchors.fill: parent
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

    //---------------------------------------------
    // Left Toolbar Row
    Row {
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth
        spacing:            ScreenTools.defaultFontPixelWidth * 2

        ExclusiveGroup { id: mainActionGroup }

        AuterionToolBarButton {
            id:                 settingsButton
            logo:               true
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/auterion/img/menu_logo.svg"
            onClicked:          toolBar.showSettingsView()
            visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
        }

        AuterionToolBarButton {
            id:                 flyButton
            text:               qsTr("Flight Mode")
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/auterion/img/menu_flight.svg"
            onClicked:          toolBar.showFlyView()
        }

        AuterionToolBarButton {
            id:                 planButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/auterion/img/menu_plan.svg"
            onClicked:          toolBar.showPlanView()
        }

        AuterionToolBarButton {
            id:                 setupButton
            text:               qsTr("Vehicle Settings")
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/auterion/img/menu_gear.svg"
            onClicked:          toolBar.showSetupView()
        }

        /* Disabled for now
        AuterionToolBarButton {
            id:                 analyzeButton
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            exclusiveGroup:     mainActionGroup
            source:             "/qmlimages/Analyze.svg"
            visible:            !ScreenTools.isMobile && QGroundControl.corePlugin.showAdvancedUI
            onClicked:          toolBar.showAnalyzeView()
        }
        */

    }

    AuterionLabel {
        text:                       qsTr("Waiting For Vehicle")
        level:                      0.75
        pointSize:                  ScreenTools.defaultFontPointSize
        visible:                    !_activeVehicle
        anchors.centerIn:           parent
    }

    Row {
        spacing:                    ScreenTools.defaultFontPixelWidth
        visible:                    _activeVehicle
        anchors.top:                parent.top
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        AuterionMenu {
            level:                  0.75
            text:                   qsTr("Mode:")
            pointSize:              ScreenTools.defaultFontPointSize
            model:                  _activeVehicle ? _activeVehicle.flightModes : [ ]
            anchors.verticalCenter: parent.verticalCenter
        }
        Loader {
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            source:                 "/auterion/AuterionArmedIndicator.qml"
        }
    }

    Row {
        id:                         indicatorRow
        anchors.top:                parent.top
        anchors.bottom:             parent.bottom
        anchors.right:              parent.right
        anchors.rightMargin:        ScreenTools.defaultFontPixelWidth
        spacing:                    ScreenTools.defaultFontPixelWidth * 2
        readonly property real buttonMargins: ScreenTools.defaultFontPixelHeight * 0.66
        Repeater {
            model:                  _activeVehicle ? _activeVehicle.toolBarIndicators : []
            Loader {
                anchors.top:            parent.top
                anchors.topMargin:      indicatorRow.buttonMargins
                anchors.bottom:         parent.bottom
                anchors.bottomMargin:   indicatorRow.buttonMargins
                source:                 modelData;
            }
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          _activeVehicle ? _activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
    }

}
