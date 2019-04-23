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
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.Palette               1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Controllers           1.0

import Auterion.Widgets                     1.0

Item {
    id:                                     toolBar
    anchors.fill:                           parent
    property string sectionTitle:           qsTr("Fly")
    Row {
        id:                                 iconRow
        height:                             parent.height
        anchors.left:                       parent.left
        spacing:                            ScreenTools.defaultFontPixelWidth * 2
        AuterionIconButton {
            height:                         parent.height
            text:                           sectionTitle
            onClicked: {
                if(drawer.visible) {
                    drawer.close()
                } else {
                    drawer.open()
                }
                // Easter egg mechanism
                _clickCount++
                eggTimer.restart()
                if (_clickCount == 5) {
                    QGroundControl.corePlugin.showAdvancedUI = !QGroundControl.corePlugin.showAdvancedUI
                }
            }
            property int _clickCount: 0
            Timer {
                id:             eggTimer
                interval:       1000
                onTriggered:    parent._clickCount = 0
            }
        }
        Rectangle {
            width:                          1
            height:                         parent.height
            color:                          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.15) : Qt.rgba(1,1,1,0.15)
        }
        //-------------------------------------------------------------------------
        //-- Multi Vehicle Selector
        Loader {
            anchors.top:                    parent.top
            anchors.bottom:                 parent.bottom
            source:                         "/auterion/AuterionMultiVehicleSelector.qml"
            visible:                        activeVehicle
        }
        Rectangle {
            width:                          1
            height:                         parent.height
            color:                          qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(0,0,0,0.15) : Qt.rgba(1,1,1,0.15)
            visible:                        activeVehicle
        }
        //-------------------------------------------------------------------------
        //-- Flight Mode
        Loader {
            anchors.top:                    parent.top
            anchors.bottom:                 parent.bottom
            source:                         "/auterion/AuterionModeIndicator.qml"
            visible:                        activeVehicle
        }
    }
    //-------------------------------------------------------------------------
    //-- Arm/Disarm
    Loader {
        anchors.top:                        parent.top
        anchors.bottom:                     parent.bottom
        anchors.horizontalCenter:           parent.horizontalCenter
        source:                             "/auterion/AuterionArmedIndicator.qml"
        visible:                            activeVehicle
    }
    //-------------------------------------------------------------------------
    // Indicators
    Loader {
        source:                             "/auterion/AuterionMainToolBarIndicators.qml"
        anchors.left:                       iconRow.right
        anchors.leftMargin:                 ScreenTools.defaultFontPixelWidth * 2
        anchors.right:                      parent.right
        anchors.top:                        parent.top
        anchors.bottom:                     parent.bottom
    }
    //-------------------------------------------------------------------------
    // Parameter download progress bar
    Rectangle {
        anchors.bottom:                     parent.bottom
        height:                             ScreenTools.defaultFontPixelheight * 0.25
        width:                              activeVehicle ? activeVehicle.parameterManager.loadProgress * parent.width : 0
        color:                              qgcPal.colorGreen
    }
    //-------------------------------------------------------------------------
    // Bottom single pixel divider
    Rectangle {
        anchors.left:                       parent.left
        anchors.right:                      parent.right
        anchors.bottom:                     parent.bottom
        height:                             1
        color:                              "black"
        visible:                            qgcPal.globalTheme === QGCPalette.Light
    }
    //-------------------------------------------------------------------------
    //-- Navigation Drawer (Left to Right, on command or using touch gestures)
    Drawer {
        id:                                 drawer
        y:                                  0
        width:                              navButtonWidth
        height:                             mainWindow.height
        background: Rectangle {
            color:  qgcPal.globalTheme === QGCPalette.Light ? "white" : "#0B1629"
        }
        ButtonGroup {
            id:                             buttonGroup
            buttons:                        buttons.children
        }
        ColumnLayout {
            id:                             buttons
            anchors.top:                    parent.top
            anchors.topMargin:              ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:                   parent.left
            anchors.right:                  parent.right
            spacing:                        ScreenTools.defaultFontPixelHeight * 0.5
            AuterionToolBarButton {
                text:                       qsTr("Fly")
                icon.source:                "/auterion/img/vehicle.svg"
                Layout.fillWidth:           true
                onClicked: {
                    checked = true
                    drawer.close()
                    sectionTitle = text
                    mainWindow.showFlyView()
                }
            }
            Rectangle {
                Layout.alignment:           Qt.AlignVCenter
                width:                      parent.width
                height:                     1
                color:                      Qt.rgba(1,1,1,0.15)
            }
            AuterionToolBarButton {
                text:                       qsTr("Plan")
                icon.source:                "/auterion/img/plan.svg"
                Layout.fillWidth:           true
                onClicked: {
                    checked = true
                    drawer.close()
                    sectionTitle = text
                    mainWindow.showPlanView()
                }
            }
            Rectangle {
                Layout.alignment:           Qt.AlignVCenter
                width:                      parent.width
                height:                     1
                color:                      Qt.rgba(1,1,1,0.15)
            }
            AuterionToolBarButton {
                text:                       qsTr("Analyze")
                icon.source:                "/qmlimages/Analyze.svg"
                Layout.fillWidth:           true
                onClicked: {
                    checked = true
                    drawer.close()
                    sectionTitle = text
                    mainWindow.showAnalyzeView()
                }
            }
            Rectangle {
                Layout.alignment:           Qt.AlignVCenter
                width:                      parent.width
                height:                     1
                color:                      Qt.rgba(1,1,1,0.15)
            }
            AuterionToolBarButton {
                text:                       qsTr("Vehicle Setup")
                icon.source:                "/auterion/img/vehicle_settings.svg"
                Layout.fillWidth:           true
                onClicked: {
                    checked = true
                    drawer.close()
                    sectionTitle = text
                    mainWindow.showSetupView()
                }
            }
            Rectangle {
                Layout.alignment:           Qt.AlignVCenter
                width:                      parent.width
                height:                     1
                color:                      Qt.rgba(1,1,1,0.15)
            }
        }
        ColumnLayout {
            id:                             lowerButtons
            anchors.bottom:                 parent.bottom
            anchors.bottomMargin:           ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:                   parent.left
            anchors.right:                  parent.right
            spacing:                        ScreenTools.defaultFontPixelHeight * 0.5
            Rectangle {
                Layout.alignment:           Qt.AlignVCenter
                width:                      parent.width
                height:                     1
                color:                      Qt.rgba(1,1,1,0.15)
            }
            AuterionToolBarButton {
                id:                         settingsButton
                text:                       qsTr("Settings")
                icon.source:                "/auterion/img/settings.svg"
                Layout.fillWidth:           true
                onClicked: {
                    checked = true
                    buttonGroup.checkState = Qt.Unchecked
                    drawer.close()
                    sectionTitle = text
                    mainWindow.showSettingsView()
                }
            }
            Connections {
                target:                     buttonGroup
                onClicked:                  settingsButton.checked = false
            }
        }
    }
}
