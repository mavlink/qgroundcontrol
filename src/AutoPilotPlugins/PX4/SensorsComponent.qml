import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.AutoPilotPlugins.PX4

SetupPage {
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        SensorsSetup {
            width:      availableWidth
            height:     availableHeight
        }
    }
}
