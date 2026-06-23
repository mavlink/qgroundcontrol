import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SetupPage {
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        QGCGroupBox {
            title: qsTr("Remote Support")

            GridLayout {
                id:               elementsRow
                columns:          2

                columnSpacing:          ScreenTools.defaultFontPixelWidth
                rowSpacing:             ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    visible:            QGroundControl.settingsManager.mavlinkSettings.forwardMavlinkAPMSupportHostName.userVisible
                    text:               qsTr("Host name:")
                }
                FactTextField {
                    id:                     mavlinkForwardingHostNameField
                    fact:                   QGroundControl.settingsManager.mavlinkSettings.forwardMavlinkAPMSupportHostName
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
