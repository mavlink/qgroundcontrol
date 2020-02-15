/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Layouts  1.11
import QtQuick.Dialogs  1.3

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

Item {
    id: toolBar

    Component.onCompleted: {
        //-- TODO: Get this from the actual state
        flyButton.checked = true
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


    //-- Setup can be invoked from c++ side
    Connections {
        target: setupWindow
        onVisibleChanged: {
            if(setupWindow.visible) {
                setupButton.checked = true
            }
        }
    }

    QGCFlickable {
        anchors.fill:       parent
        contentWidth:       toolbarRow.width
        flickableDirection: Flickable.HorizontalFlick

        RowLayout {
            id:                     toolbarRow
            anchors.bottomMargin:   1
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            spacing:                ScreenTools.defaultFontPixelWidth / 2

            // Important Note: Toolbar buttons must manage their checked state manually in order to support
            // view switch prevention. There doesn't seem to be a way to make this work if they are in a
            // ButtonGroup.

            //---------------------------------------------
            // Toolbar Row
            RowLayout {
                id:                 buttonRow
                Layout.fillHeight:  true
                spacing:            0

                function clearAllChecks() {
                    for (var i=0; i<buttonRow.children.length; i++) {
                        if (buttonRow.children[i].toString().startsWith("QGCToolBarButton")) {
                            buttonRow.children[i].checked = false
                        }
                    }
                }

                QGCToolBarButton {
                    id:                 settingsButton
                    Layout.fillHeight:  true
                    icon.source:        "/res/QGCLogoWhite"
                    logo:               true
                    visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
                    onClicked: {
                        if (mainWindow.preventViewSwitch()) {
                            return
                        }
                        buttonRow.clearAllChecks()
                        checked = true
                        mainWindow.showSettingsView()
                    }
                }

                QGCToolBarButton {
                    id:                 setupButton
                    Layout.fillHeight:  true
                    icon.source:        "/qmlimages/Gears.svg"
                    onClicked: {
                        if (mainWindow.preventViewSwitch()) {
                            return
                        }
                        buttonRow.clearAllChecks()
                        checked = true
                        mainWindow.showSetupView()
                    }
                }

                QGCToolBarButton {
                    id:                 planButton
                    Layout.fillHeight:  true
                    icon.source:        "/qmlimages/Plan.svg"
                    onClicked: {
                        if (mainWindow.preventViewSwitch()) {
                            return
                        }
                        buttonRow.clearAllChecks()
                        checked = true
                        mainWindow.showPlanView()
                    }
                }

                QGCToolBarButton {
                    id:                 flyButton
                    Layout.fillHeight:  true
                    icon.source:        "/qmlimages/PaperPlane.svg"
                    onClicked: {
                        if (mainWindow.preventViewSwitch()) {
                            return
                        }
                        buttonRow.clearAllChecks()
                        checked = true
                        mainWindow.showFlyView()

                        // Easter Egg mechanism
                        _clickCount++
                        eggTimer.restart()
                        if (_clickCount == 5) {
                            if(!QGroundControl.corePlugin.showAdvancedUI) {
                                advancedModeConfirmation.open()
                            } else {
                                QGroundControl.corePlugin.showAdvancedUI = false
                            }
                        } else if (_clickCount == 7) {
                            QGroundControl.corePlugin.showTouchAreas = !QGroundControl.corePlugin.showTouchAreas
                        }
                    }

                    property int _clickCount: 0

                    Timer {
                        id:             eggTimer
                        interval:       1000
                        repeat:         false
                        onTriggered:    parent._clickCount = 0
                    }

                    MessageDialog {
                        id:                 advancedModeConfirmation
                        title:              qsTr("Advanced Mode")
                        text:               QGroundControl.corePlugin.showAdvancedUIMessage
                        standardButtons:    StandardButton.Yes | StandardButton.No
                        onYes: {
                            QGroundControl.corePlugin.showAdvancedUI = true
                            advancedModeConfirmation.close()
                        }
                    }
                }

                QGCToolBarButton {
                    id:                 analyzeButton
                    Layout.fillHeight:  true
                    icon.source:        "/qmlimages/Analyze.svg"
                    visible:            QGroundControl.corePlugin.showAdvancedUI
                    onClicked: {
                        if (mainWindow.preventViewSwitch()) {
                            return
                        }
                        buttonRow.clearAllChecks()
                        checked = true
                        mainWindow.showAnalyzeView()
                    }
                }

                Item {
                    Layout.fillHeight:  true
                    width:              ScreenTools.defaultFontPixelWidth / 2
                    visible:            activeVehicle
                }

                Rectangle {
                    Layout.margins:     ScreenTools.defaultFontPixelHeight / 2
                    Layout.fillHeight:  true
                    width:              1
                    color:              qgcPal.text
                    visible:            activeVehicle
                }

                Item {
                    Layout.fillHeight:  true
                    width:              ScreenTools.defaultFontPixelWidth / 2
                    visible:            activeVehicle
                }
            }

            Loader {
                id:                 toolbarIndicators
                Layout.fillHeight:  true
                source:             "/toolbar/MainToolBarIndicators.qml"
                visible:            activeVehicle && !communicationLost
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Branding Logo
    Image {
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.66
        visible:                activeVehicle && !communicationLost && x > (toolbarRow.x + toolbarRow.width + ScreenTools.defaultFontPixelWidth)
        fillMode:               Image.PreserveAspectFit
        source:                 _outdoorPalette ? _brandImageOutdoor : _brandImageIndoor
        mipmap:                 true

        property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
        property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length != 0
        property string _userBrandImageIndoor:  QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor.value
        property string _userBrandImageOutdoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor.value
        property bool   _userBrandingIndoor:    _userBrandImageIndoor.length != 0
        property bool   _userBrandingOutdoor:   _userBrandImageOutdoor.length != 0
        property string _brandImageIndoor:      _userBrandingIndoor ?
                                                    _userBrandImageIndoor : (_userBrandingOutdoor ?
                                                        _userBrandImageOutdoor : (_corePluginBranding ?
                                                            QGroundControl.corePlugin.brandImageIndoor : (activeVehicle ?
                                                                activeVehicle.brandImageIndoor : ""
                                                            )
                                                        )
                                                    )
        property string _brandImageOutdoor:     _userBrandingOutdoor ?
                                                    _userBrandImageOutdoor : (_userBrandingIndoor ?
                                                        _userBrandImageIndoor : (_corePluginBranding ?
                                                            QGroundControl.corePlugin.brandImageOutdoor : (activeVehicle ?
                                                                activeVehicle.brandImageOutdoor : ""
                                                            )
                                                        )
                                                    )
    }

    // Small parameter download progress bar
    Rectangle {
        anchors.bottom: parent.bottom
        height:         toolBar.height * 0.05
        width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
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

        property bool _initialDownloadComplete: activeVehicle ? activeVehicle.parameterManager.parametersReady : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
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


    //-------------------------------------------------------------------------
    //-- Waiting for a vehicle
    QGCLabel {
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.right:          parent.right
        anchors.verticalCenter: parent.verticalCenter
        text:                   qsTr("Waiting For Vehicle Connection")
        font.pointSize:         ScreenTools.mediumFontPointSize
        font.family:            ScreenTools.demiboldFontFamily
        color:                  qgcPal.colorRed
        visible:                !activeVehicle
    }

    //-------------------------------------------------------------------------
    //-- Connection Status
    Row {
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        layoutDirection:        Qt.RightToLeft
        spacing:                ScreenTools.defaultFontPixelWidth
        visible:                activeVehicle && communicationLost

        QGCButton {
            id:                     disconnectButton
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("Disconnect")
            primary:                true
            onClicked:              activeVehicle.disconnectInactiveVehicle()
        }

        QGCLabel {
            id:                     connectionLost
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("COMMUNICATION LOST")
            font.pointSize:         ScreenTools.largeFontPointSize
            font.family:            ScreenTools.demiboldFontFamily
            color:                  qgcPal.colorRed
        }
    }

}
