/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick              2.5
import QtQuick.Controls     1.4

import QGroundControl               1.0
import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controllers   1.0


// Mixer Tuning setup page
SetupPage {
    id:             tuningPage
    pageComponent:  tuningPageComponent

    Component {
        id: tuningPageComponent

        Column {
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; factPanel: tuningPage.viewPanel }

            MixersComponentController {
                id:         mixers
                factPanel:  panel
            }

            QGCPalette { id: palette; colorGroupEnabled: true }

            QGCLabel { text: qsTr("Group") }

            QGCTextField {
                id:                 groupText
                text:               "0"
            }

            QGCLabel { text: qsTr("Mixer") }

            QGCTextField {
                id:                 mixerText
                text:               "0"
            }

//id:getMixersCountButton
            QGCButton {
                id:getMixersCountButton
                text: qsTr("Request mixer count")}
            QGCButton { text: qsTr("Request submixer count")}
            QGCButton { text: qsTr("Request mixer type")}
            QGCButton { text: qsTr("Request parameter")}
            QGCButton { text: qsTr("Request all")}
            QGCButton { text: qsTr("Set parameter")}
        } // Column
    } // Component
} // SetupView
