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
    id: _root

    property alias indicatorSource:     indicatorLoader.source
    property alias showModeIndicators:  indicatorLoader.showModeIndicators

    // FIXME: Reaching up for communicationLost?

    property var _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

    Component.onCompleted: _viewButtonClicked(flyButton)

    function _viewButtonClicked(button) {
        if (mainWindow.preventViewSwitch()) {
            return false
        }
        viewButtonSelectRow.visible = false
        buttonSelectHideTimer.stop()
        currentButton.icon.source = button.icon.source
        currentButton.logo = button.logo
        return true
    }

    //-- Setup can be invoked from c++ side
    Connections {
        target: setupWindow
        onVisibleChanged: {
            if (setupWindow.visible) {
                _viewButtonClicked(setupButton)
            }
        }
    }

    QGCPalette { id: qgcPal }

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
        id:                     viewButtonRow
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        spacing:                ScreenTools.defaultFontPixelWidth / 2

        QGCToolBarButton {
            id:                 currentButton
            Layout.fillHeight:  true

            onClicked: {
                viewButtonSelectRow.visible = !viewButtonSelectRow.visible
                if (viewButtonSelectRow.visible) {
                    buttonSelectHideTimer.start()
                } else {
                    buttonSelectHideTimer.stop()
                }
            }
        }

        //---------------------------------------------
        // Toolbar Row
        RowLayout {
            id:                 viewButtonSelectRow
            Layout.fillHeight:  true
            spacing:            0
            visible:            false

            Timer {
                id:             buttonSelectHideTimer
                interval:       5000
                repeat:         false
                onTriggered:    viewButtonSelectRow.visible = false
            }

            Rectangle {
                Layout.margins:     ScreenTools.defaultFontPixelHeight / 2
                Layout.fillHeight:  true
                width:              1
                color:              qgcPal.text
            }

            QGCToolBarButton {
                id:                 settingsButton
                Layout.fillHeight:  true
                icon.source:        "/res/QGCLogoFull"
                logo:               true
                visible:            currentButton.icon.source !== icon.source && !QGroundControl.corePlugin.options.combineSettingsAndSetup
                onClicked: {
                    if (_viewButtonClicked(this)) {
                        mainWindow.showSettingsView()
                    }
                }
            }

            QGCToolBarButton {
                id:                 setupButton
                Layout.fillHeight:  true
                icon.source:        "/qmlimages/Gears.svg"
                visible:            currentButton.icon.source !== icon.source
                onClicked: {
                    if (_viewButtonClicked(this)) {
                        mainWindow.showSetupView()
                    }
                }
            }

            QGCToolBarButton {
                id:                 planButton
                Layout.fillHeight:  true
                icon.source:        "/qmlimages/Plan.svg"
                visible:            currentButton.icon.source !== icon.source
                onClicked: {
                    if (_viewButtonClicked(this)) {
                        mainWindow.showPlanView()
                    }
                }
            }

            QGCToolBarButton {
                id:                 flyButton
                Layout.fillHeight:  true
                icon.source:        "/qmlimages/PaperPlane.svg"
                visible:            currentButton.icon.source !== icon.source
                onClicked: {
                    if (_viewButtonClicked(this)) {
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
                visible:            currentButton.icon.source !== icon.source && QGroundControl.corePlugin.showAdvancedUI
                onClicked: {
                    if (_viewButtonClicked(this)) {
                        mainWindow.showAnalyzeView()
                    }
                }
            }
        }
    }

    Rectangle {
        id:                 separator1
        anchors.margins:    ScreenTools.defaultFontPixelHeight / 2
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       viewButtonRow.right
        width:              1
        color:              qgcPal.text
    }

    QGCFlickable {
        id:                     toolsFlickable
        anchors.leftMargin:     ScreenTools.defaultFontPixelHeight / 2
        anchors.left:           separator1.right
        anchors.bottomMargin:   1
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          connectionStatus.visible ? connectionStatus.left : parent.right
        contentWidth:           indicatorLoader.x + indicatorLoader.width
        flickableDirection:     Flickable.HorizontalFlick
        clip:                   !valueArea.settingsUnlocked

        HorizontalFactValueGrid {
            id:                     valueArea
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            userSettingsGroup:      toolbarUserSettingsGroup
            defaultSettingsGroup:   toolbarDefaultSettingsGroup

            QGCMouseArea {
                anchors.fill:   parent
                visible:        !parent.settingsUnlocked
                onClicked:      parent.settingsUnlocked = true
            }
        }

        Rectangle {
            id:                     separator2
            anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
            anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight / 2 - 1
            anchors.top:            parent.top
            anchors.bottom:         parent.bottom
            anchors.left:           valueArea.right
            width:                  1
            color:                  qgcPal.text
        }

        Loader {
            id:                 indicatorLoader
            anchors.leftMargin: ScreenTools.defaultFontPixelHeight / 2
            anchors.left:       separator2.right
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "qrc:/toolbar/MainToolBarIndicators.qml"

            property bool showModeIndicators: true
        }
    }

    //-------------------------------------------------------------------------
    //-- Branding Logo
    Image {
        anchors.right:          parent.right
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.66
        visible:                _activeVehicle && !communicationLost && x > (toolsFlickable.x + toolsFlickable.contentWidth + ScreenTools.defaultFontPixelWidth)
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
        height:         _root.height * 0.05
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
        visible:                !_activeVehicle
    }

    //-------------------------------------------------------------------------
    //-- Connection Status
    Row {
        id:                     connectionStatus
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        layoutDirection:        Qt.RightToLeft
        spacing:                ScreenTools.defaultFontPixelWidth
        visible:                _activeVehicle && communicationLost

        QGCButton {
            id:                     disconnectButton
            anchors.verticalCenter: parent.verticalCenter
            text:                   qsTr("Disconnect")
            primary:                true
            onClicked:              _activeVehicle.disconnectInactiveVehicle()
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
