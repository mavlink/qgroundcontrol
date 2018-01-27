/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtMultimedia             5.5
import QtQuick.Layouts          1.2

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FactControls          1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.Controllers           1.0
import QGroundControl.SettingsManager       1.0

QGCView {
    id:                 _qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30
    property real _panelWidth:                  _qgcView.width * _internalWidthRatio
    property Fact _enableAirMapFact:            QGroundControl.settingsManager.appSettings.enableAirMap
    property bool _airMapEnabled:               _enableAirMapFact.rawValue

    readonly property real _internalWidthRatio:          0.8

    QGCPalette { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width
            Column {
                id:                 settingsColumn
                width:              _qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- General
                Item {
                    width:                      _panelWidth
                    height:                     generalLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.unitsSettings.visible
                    QGCLabel {
                        id:             generalLabel
                        text:           qsTr("General")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     generalCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:             generalCol
                        spacing:        ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        FactCheckBox {
                            text:       qsTr("Enable AirMap Services")
                            fact:       _enableAirMapFact
                            visible:    _enableAirMapFact.visible
                        }
                        QGCCheckBox {
                            text:       qsTr("Disable Telemetry")
                            checked:    false
                            enabled:    _airMapEnabled
                            onClicked:
                            {

                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Login / Registration
                Item {
                    width:                      _panelWidth
                    height:                     loginLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:             loginLabel
                        text:           qsTr("Login / Registration")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     loginGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    GridLayout {
                        id:                 loginGrid
                        columns:            2
                        rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                        anchors.centerIn:   parent
                        QGCLabel { text: qsTr("Email:") }
                        QGCTextField {
                            width:      _editFieldWidth
                            enabled:    _airMapEnabled
                        }
                        QGCLabel { text: qsTr("Password:") }
                        QGCTextField {
                            width:      _editFieldWidth
                            enabled:    _airMapEnabled
                        }
                        Item {
                            width:  1
                            height: 1
                            Layout.columnSpan: 2
                        }
                        QGCLabel {
                            text:   qsTr("Forgot Your AirMap Password?")
                            Layout.alignment:  Qt.AlignHCenter
                            Layout.columnSpan: 2
                        }
                        Item {
                            width:  1
                            height: 1
                            Layout.columnSpan: 2
                        }
                        QGCButton {
                            text:   qsTr("Register for an AirMap Account")
                            Layout.alignment:  Qt.AlignHCenter
                            Layout.columnSpan: 2
                            enabled:           _airMapEnabled
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Pilot Profile
                Item {
                    width:                      _panelWidth
                    height:                     profileLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:             profileLabel
                        text:           qsTr("Pilot Profile")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     profileGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    GridLayout {
                        id:                 profileGrid
                        columns:            2
                        columnSpacing:      ScreenTools.defaultFontPixelHeight * 2
                        rowSpacing:         ScreenTools.defaultFontPixelWidth  * 0.25
                        anchors.centerIn:   parent
                        QGCLabel { text: qsTr("Name:") }
                        QGCLabel { text: qsTr("John Doe") }
                        QGCLabel { text: qsTr("User Name:") }
                        QGCLabel { text: qsTr("joe36") }
                        QGCLabel { text: qsTr("Email:") }
                        QGCLabel { text: qsTr("jonh@doe.com") }
                        QGCLabel { text: qsTr("Phone:") }
                        QGCLabel { text: qsTr("+1 212 555 1212") }
                    }
                }
            }
        }
    }
}
