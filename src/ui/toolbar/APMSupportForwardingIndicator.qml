/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Layouts  1.11

import QGroundControl                       1.0
import QGroundControl.Controls              1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.Palette               1.0

//-------------------------------------------------------------------------
//-- Telemetry RSSI
Item {
    id:             _root
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          forwardingSupportIcon.width * 1.1

    property bool showIndicator: QGroundControl.linkManager.mavlinkSupportForwardingEnabled

    Component {
        id: forwardingSupportInfo
        
        Rectangle {
            width:  telemGrid.width  + ScreenTools.defaultFontPixelWidth  * 3
            height: telemGrid.height + ScreenTools.defaultFontPixelHeight * 2
            radius: ScreenTools.defaultFontPixelHeight * 0.5
            color:  qgcPal.window

            GridLayout {
                id:                 telemGrid
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                columnSpacing:      ScreenTools.defaultFontPixelWidth
                columns:            2
                anchors.centerIn:   parent

                QGCLabel { 
                    Layout.columnSpan: 2
                    text: qsTr("Mavlink traffic is being forwarded to a support server") 
                }

                QGCLabel { 
                    text: qsTr("Server name:")
                }
                QGCLabel { 
                    text: QGroundControl.settingsManager.appSettings.forwardMavlinkAPMSupportHostName.value
                }
            }
        }
    }

    Image {
        id:                 forwardingSupportIcon
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        width:              height
        sourceSize.height:  height
        source:             "/qmlimages/ForwardingSupportIconGreen.svg"
        fillMode:           Image.PreserveAspectFit
    }
    
    MouseArea {
        anchors.fill: parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, forwardingSupportInfo)
        }
    }
}
