/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick              2.3
import QtQuick.Controls     1.2
import QtQuick.Dialogs      1.2
import QtGraphicalEffects   1.0

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

import TyphoonHQuickInterface               1.0

Item {
    id: dlgRoot
    anchors.fill: parent
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 12
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 20
    property bool _validPassword:               passwordField.text === confirmField.text && passwordField.text.length > 7 && passwordField.text.length < 21

    function setMetric() {
        QGroundControl.settingsManager.unitsSettings.distanceUnits.rawValue = UnitsSettings.DistanceUnitsMeters
        QGroundControl.settingsManager.unitsSettings.areaUnits.rawValue = UnitsSettings.AreaUnitsSquareMeters
        QGroundControl.settingsManager.unitsSettings.speedUnits.rawValue = UnitsSettings.SpeedUnitsMetersPerSecond
    }

    function setTheDarkSide() {
        QGroundControl.settingsManager.unitsSettings.distanceUnits.rawValue = UnitsSettings.DistanceUnitsFeet
        QGroundControl.settingsManager.unitsSettings.areaUnits.rawValue = UnitsSettings.AreaUnitsSquareFeet
        QGroundControl.settingsManager.unitsSettings.speedUnits.rawValue = UnitsSettings.SpeedUnitsFeetPerSecond
    }

    MouseArea {
        anchors.fill:   parent
        onWheel:        { wheel.accepted = true; }
        onPressed:      { mouse.accepted = true; }
        onReleased:     { mouse.accepted = true; }
    }
    Rectangle {
        id:             initialSettingsDlgShadow
        anchors.fill:   initialSettingsDlgRect
        radius:         initialSettingsDlgRect.radius
        color:          qgcPal.window
        visible:        false
    }
    DropShadow {
        anchors.fill:       initialSettingsDlgShadow
        visible:            initialSettingsDlgRect.visible
        horizontalOffset:   4
        verticalOffset:     4
        radius:             32.0
        samples:            65
        color:              Qt.rgba(0,0,0,0.75)
        source:             initialSettingsDlgShadow
    }
    Rectangle {
        id:             initialSettingsDlgRect
        width:          mainWindow.width   * 0.65
        height:         initialSettingsDlgCol.height * 1.2
        radius:         ScreenTools.defaultFontPixelWidth
        color:          qgcPal.alertBackground
        border.color:   qgcPal.alertBorder
        border.width:   2
        anchors.centerIn: parent
        Column {
            id:                 initialSettingsDlgCol
            width:              initialSettingsDlgRect.width
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent
            QGCLabel {
                text:           qsTr("Initial System Settings")
                font.family:    ScreenTools.demiboldFontFamily
                font.pointSize: ScreenTools.largeFontPointSize
                color:          qgcPal.alertText
                anchors.horizontalCenter: parent.horizontalCenter
            }
            QGCLabel {
                text:           qsTr("(You can change these later at any time through <i>Settings</i>)")
                font.pointSize: ScreenTools.smallFontPointSize
                color:          qgcPal.alertText
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                width:  1
                height: ScreenTools.defaultFontPixelHeight
            }
            Row {
                visible:        TyphoonHQuickInterface.isDefaultPwd
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           qsTr("Link Password")
                    color:          qgcPal.alertText
                    width:          _labelWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:         passwordField
                    echoMode:   TextInput.Password
                    width:      _editFieldWidth
                    focus:      true
                    maximumLength:  20
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Row {
                visible:        TyphoonHQuickInterface.isDefaultPwd
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           qsTr("Confirm Password")
                    color:          qgcPal.alertText
                    width:          _labelWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCTextField {
                    id:         confirmField
                    echoMode:   TextInput.Password
                    width:      _editFieldWidth
                    focus:      true
                    maximumLength:  20
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            QGCLabel {
                text:           qsTr("(Password must be between 8 and 20 characters)")
                font.pointSize: ScreenTools.smallFontPointSize
                color:          qgcPal.alertText
                visible:        TyphoonHQuickInterface.isDefaultPwd && !_validPassword
                anchors.horizontalCenter: parent.horizontalCenter
            }
            Item {
                width:      1
                visible:    TyphoonHQuickInterface.isDefaultPwd
                height:     ScreenTools.defaultFontPixelHeight
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           qsTr("Color Scheme")
                    color:          qgcPal.alertText
                    width:          _labelWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
                FactComboBox {
                    width:          _editFieldWidth
                    fact:           QGroundControl.settingsManager.appSettings.indoorPalette
                    indexModel:     false
                    anchors.verticalCenter: parent.verticalCenter
                }
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCLabel {
                    text:           qsTr("Units")
                    color:          qgcPal.alertText
                    width:          _labelWidth
                    anchors.verticalCenter: parent.verticalCenter
                }
                QGCComboBox {
                    width:          _editFieldWidth
                    model:          unitOptions
                    currentIndex:   0
                    onActivated:  {
                        if(index === 0) {
                            setMetric()
                        } else {
                            setTheDarkSide()
                        }
                    }
                    anchors.verticalCenter: parent.verticalCenter
                    property var unitOptions: [qsTr("Metric"), qsTr("Imperial")]
                }
            }
            Item {
                width:  1
                height: ScreenTools.defaultFontPixelHeight
            }
            Row {
                spacing:        ScreenTools.defaultFontPixelWidth * 4
                anchors.horizontalCenter: parent.horizontalCenter
                QGCButton {
                    text:           qsTr("Accept")
                    width:          ScreenTools.defaultFontPixelWidth  * 16
                    height:         ScreenTools.defaultFontPixelHeight * 2
                    enabled:        TyphoonHQuickInterface.isDefaultPwd ? _validPassword : true
                    onClicked: {
                        restartConfirmation.open()
                    }
                    MessageDialog {
                        id:                 restartConfirmation
                        text:               qsTr("DataPilot will now restart")
                        standardButtons:    StandardButton.Ok
                        onAccepted: {
                            mainWindow.enableToolbar()
                            TyphoonHQuickInterface.firstRun = false
                            if(TyphoonHQuickInterface.isDefaultPwd && passwordField.text !== "") {
                                TyphoonHQuickInterface.setWiFiPassword(passwordField.text)
                            }
                            //-- We need to restart after the changes above
                            TyphoonHQuickInterface.restart()
                            rootLoader.sourceComponent = null
                        }
                    }
                }
            }
        }
    }
    Component.onCompleted: {
        rootLoader.width  = dlgRoot.width
        rootLoader.height = dlgRoot.height
        mainWindow.disableToolbar()
    }
}
