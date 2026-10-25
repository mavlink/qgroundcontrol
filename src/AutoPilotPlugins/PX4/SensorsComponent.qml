import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.AutoPilotPlugins.PX4

SetupPage {
    id:             sensorsPage
    pageComponent:  pageComponent
    Component {
        id: pageComponent
        SensorsSetup {
            width:      availableWidth

            onScrollToTopRequested: sensorsPage.scrollToTop()
        }
    }
}
