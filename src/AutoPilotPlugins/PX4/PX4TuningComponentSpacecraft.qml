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

        PX4TuningComponentSpacecraftAll {
        }
    } // Component - pageComponent
} // SetupPage
