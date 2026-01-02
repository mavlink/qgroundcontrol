import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

SetupPage {
    id:             tuningPage
    pageComponent:  pageComponent

    Component {
        id: pageComponent

        PX4TuningComponentPlaneAll {
            height: availableHeight
        }
    } // Component - pageComponent
} // SetupPage
