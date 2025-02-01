/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette

//-------------------------------------------------------------------------
//-- Telemetry RSSI
Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          forwardingSupportIcon.width * 1.1

    property bool showIndicator: QGroundControl.linkManager.mavlinkSupportForwardingEnabled

    Component {
        id: forwardingSupportInfoPage
        
        ToolIndicatorPage {
            contentComponent: SettingsGroupLayout {
                QGCLabel { text: qsTr("Mavlink traffic is being forwarded to a support server") }

                LabelledLabel { 
                    label:      qsTr("Server name:")
                    labelText:  QGroundControl.settingsManager.appSettings.forwardMavlinkAPMSupportHostName.value
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
        onClicked:      mainWindow.showIndicatorDrawer(forwardingSupportInfoPage, control)
    }
}
