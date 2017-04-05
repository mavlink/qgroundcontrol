/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.3
import QtQuick.Controls 1.2
import QtQuick.Layouts  1.2

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

import TyphoonHQuickInterface               1.0

//-------------------------------------------------------------------------
//-- WIFI RSSI Indicator
Item {
    width:          ScreenTools.defaultFontPixelWidth * 8
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property bool _hasWifi:     TyphoonHQuickInterface.rssi > -100 && TyphoonHQuickInterface.rssi < 0
    property bool _isAP:        _hasWifi && !TyphoonHQuickInterface.isTyphoon

    Component {
        id: wifiRSSIInfo

        Rectangle {
            width:  rcrssiCol.width   + ScreenTools.defaultFontPixelWidth  * 3
            height: rcrssiCol.height  + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                 rcrssiCol
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                width:              Math.max(rcrssiGrid.width, rssiLabel.width)
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                anchors.centerIn:   parent

                QGCLabel {
                    id:             rssiLabel
                    text:           _hasWifi ? qsTr("Video/Telemetry RSSI Status") : qsTr("Video/Telemetry Link Data Unavailable")
                    font.family:    ScreenTools.demiboldFontFamily
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                GridLayout {
                    id:                 rcrssiGrid
                    visible:            _hasWifi
                    anchors.margins:    ScreenTools.defaultFontPixelHeight
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    columns:            2
                    anchors.horizontalCenter: parent.horizontalCenter

                    QGCLabel { text: qsTr("Connected to:"); visible: TyphoonHQuickInterface.connectedSSID !== ""; }
                    QGCLabel { text: TyphoonHQuickInterface.connectedSSID; visible: TyphoonHQuickInterface.connectedSSID !== ""; }
                    QGCLabel { text: qsTr("RSSI:") }
                    QGCLabel { text: TyphoonHQuickInterface.rssi + "dB" }
                }
            }

            Component.onCompleted: {
                var pos = mapFromItem(toolBar, centerX - (width / 2), toolBar.height)
                x = pos.x
                y = pos.y + ScreenTools.defaultFontPixelHeight
                if((x + width) >= toolBar.width) {
                    x = toolBar.width - width - ScreenTools.defaultFontPixelWidth;
                }
            }
        }
    }
    Row {
        spacing:        ScreenTools.defaultFontPixelWidth
        anchors.top:    parent.top
        anchors.bottom: _isAP ? wifiLabel.top : parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
        QGCColoredImage {
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            sourceSize.height:  height
            source:             "/typhoonh/img/wifi.svg"
            fillMode:           Image.PreserveAspectFit
            opacity:            _hasWifi ? 1 : 0.5
            color:              qgcPal.text
        }
        SignalStrength {
            anchors.verticalCenter: parent.verticalCenter
            size:                   parent.height * 0.5
            percent: {
                if(TyphoonHQuickInterface.rssi < 0) {
                    if(TyphoonHQuickInterface.rssi >= -50)
                        return 100;
                    if(TyphoonHQuickInterface.rssi <= -100)
                        return 0;
                    return 2 * (TyphoonHQuickInterface.rssi + 100);
                }
                return 0;
            }
        }
    }
    QGCLabel {
        id:         wifiLabel
        text:       "AP WIFI"
        visible:    _isAP
        color:      qgcPal.colorOrange
        anchors.bottom: parent.bottom
        anchors.horizontalCenter: parent.horizontalCenter
    }
    MouseArea {
        anchors.fill:   parent
        onClicked: {
            var centerX = mapToItem(toolBar, x, y).x + (width / 2)
            mainWindow.showPopUp(wifiRSSIInfo, centerX)
        }
    }
}
