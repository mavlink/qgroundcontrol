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
    visible:    !QGroundControl.videoManager.fullScreen

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

    RowLayout {
        anchors.bottomMargin:   1
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth / 2
        anchors.fill:           parent
        spacing:                ScreenTools.defaultFontPixelWidth * 2

        //---------------------------------------------
        // Toolbar Row
        Row {
            id:                 viewRow
            Layout.fillHeight:  true
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            ExclusiveGroup { id: mainActionGroup }

            QGCToolBarButton {
                id:                 settingsButton
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                exclusiveGroup:     mainActionGroup
                source:             "/res/QGCLogoWhite"
                logo:               true
                onClicked:          toolBar.showSettingsView()
                visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
            }

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

            QGCToolBarButton {
                id:                 flyButton
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                exclusiveGroup:     mainActionGroup
                source:             "/qmlimages/PaperPlane.svg"
                onClicked:          toolBar.showFlyView()
            }

            QGCToolBarButton {
                id:                 analyzeButton
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                exclusiveGroup:     mainActionGroup
                source:             "/qmlimages/Analyze.svg"
                visible:            !ScreenTools.isMobile && QGroundControl.corePlugin.showAdvancedUI
                onClicked:          toolBar.showAnalyzeView()
            }

            Rectangle {
                anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                width:              1
                color:              qgcPal.text
                visible:            _activeVehicle
            }
        }

        //-------------------------------------------------------------------------
        //-- Vehicle Selector
        QGCButton {
            id:                     vehicleSelectorButton
            width:                  ScreenTools.defaultFontPixelHeight * 8
            text:                   "Vehicle " + (_activeVehicle ? _activeVehicle.id : "None")
            visible:                QGroundControl.multiVehicleManager.vehicles.count > 1
            Layout.alignment:       Qt.AlignVCenter

            menu: vehicleMenu

            Menu {
                id: vehicleMenu
            }

            Component {
                id: vehicleMenuItemComponent

                MenuItem {
                    onTriggered: QGroundControl.multiVehicleManager.activeVehicle = vehicle

                    property int vehicleId: Number(text.split(" ")[1])
                    property var vehicle:   QGroundControl.multiVehicleManager.getVehicleById(vehicleId)
                }
            }

            property var vehicleMenuItems: []

            function updateVehicleMenu() {
                var i;
                // Remove old menu items
                for (i = 0; i < vehicleMenuItems.length; i++) {
                    vehicleMenu.removeItem(vehicleMenuItems[i])
                }
                vehicleMenuItems.length = 0

                // Add new items
                for (i = 0; i < QGroundControl.multiVehicleManager.vehicles.count; i++) {
                    var vehicle = QGroundControl.multiVehicleManager.vehicles.get(i)
                    var menuItem = vehicleMenuItemComponent.createObject(null, { "text": "Vehicle " + vehicle.id })
                    vehicleMenuItems.push(menuItem)
                    vehicleMenu.insertItem(i, menuItem)
                }
            }

            Component.onCompleted: updateVehicleMenu()

            Connections {
                target:         QGroundControl.multiVehicleManager.vehicles
                onCountChanged: vehicleSelectorButton.updateVehicleMenu()
            }
        }

        MainToolBarIndicators {
            Layout.fillWidth:   true
            Layout.fillHeight:  true
            Layout.margins:     ScreenTools.defaultFontPixelHeight * 0.66
        }
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          _activeVehicle ? _activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:          qgcPal.colorGreen
        visible:        !largeProgressBar.visible
    }

    // Large parameter download progress bar
    Rectangle {
        id:             largeProgressBar
        anchors.bottom: parent.bottom
        anchors.left:   parent.left
        anchors.right:  parent.right
        height:         parent.height
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: _activeVehicle ? _activeVehicle.parameterManager.parametersReady : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _activeVehicle ? _activeVehicle.parameterManager.loadProgress * parent.width : 0
            color:          qgcPal.colorGreen
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading Parameters")
            font.pointSize:     ScreenTools.largeFontPointSize
        }

        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth / 2
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}
