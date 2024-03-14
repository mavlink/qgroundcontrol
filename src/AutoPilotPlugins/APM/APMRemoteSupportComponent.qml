/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactControls
import QGroundControl.FactSystem

SetupPage {
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        Rectangle {
            id:                 backgroundRectangle
            width:              availableWidth
            height:             elementsRow.height * 1.5
            color:              qgcPal.windowShade

            GridLayout {
                id:               elementsRow
                columns:          2
                
                anchors.left:           parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.margins:        ScreenTools.defaultFontPixelWidth

                columnSpacing:          ScreenTools.defaultFontPixelWidth
                rowSpacing:             ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    visible:            QGroundControl.settingsManager.appSettings.forwardMavlinkAPMSupportHostName.visible
                    text:               qsTr("Host name:")
                }
                FactTextField {
                    id:                     mavlinkForwardingHostNameField
                    fact:                   QGroundControl.settingsManager.appSettings.forwardMavlinkAPMSupportHostName
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 40
                }
                QGCButton {
                    text:    qsTr("Connect")
                    enabled: !QGroundControl.linkManager.mavlinkSupportForwardingEnabled

                    onPressed: {
                        QGroundControl.linkManager.createMavlinkForwardingSupportLink()
                    }
                }
                QGCLabel {
                    visible:            QGroundControl.linkManager.mavlinkSupportForwardingEnabled
                    text:               qsTr("Forwarding traffic: Mavlink traffic will keep being forwarded until application restarts")
                }
            }
        }
    }
}
