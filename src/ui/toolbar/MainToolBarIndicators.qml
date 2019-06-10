/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

Item {

    //-------------------------------------------------------------------------
    // Easter egg mechanism
    MouseArea {
        anchors.fill: parent
        onClicked: {
            _clickCount++
            eggTimer.restart()
            if (_clickCount == 5) {
                if(QGroundControl.corePlugin.showAdvancedUI) {
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

    //-------------------------------------------------------------------------
    //-- Waiting for a vehicle
    QGCLabel {
        id:                     waitForVehicle
        anchors.centerIn:       parent
        text:                   qsTr("Waiting For Vehicle Connection")
        font.pointSize:         ScreenTools.mediumFontPointSize
        font.family:            ScreenTools.demiboldFontFamily
        color:                  qgcPal.colorRed
        visible:                !activeVehicle
    }

    //-------------------------------------------------------------------------
    //-- Toolbar Indicators
    Row {
        id:                 indicatorRow
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        anchors.left:       parent.left
        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 2
        spacing:            ScreenTools.defaultFontPixelWidth * 1.5
        visible:            activeVehicle && !communicationLost
        Repeater {
            model:      activeVehicle ? activeVehicle.toolBarIndicators : []
            Loader {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.66
                source:             modelData;
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
        visible:                activeVehicle && !communicationLost && x > (indicatorRow.x + indicatorRow.width + ScreenTools.defaultFontPixelWidth)
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

    //-------------------------------------------------------------------------
    //-- Connection Status
    Row {
        anchors.fill:       parent
        layoutDirection:    Qt.RightToLeft
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            activeVehicle && communicationLost

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
