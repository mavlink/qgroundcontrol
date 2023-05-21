/****************************************************************************
 *
 * (c) 2009-2023 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Palette       1.0
import QGroundControl.FactControls  1.0
import QGroundControl.FactSystem    1.0

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
