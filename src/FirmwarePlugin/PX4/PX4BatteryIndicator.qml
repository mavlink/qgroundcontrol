/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls
import MAVLink

BatteryIndicator {
    waitForParameters: true

    expandedPageComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            FactPanelController { id: controller }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Low Battery")

                LabelledFactSlider {
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                    label:                  qsTr("Warning Level")
                    fact:                   controller.getParameterFact(-1, "BAT_LOW_THR")
                }   

                LabelledFactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("Failsafe Level")
                    fact:               controller.getParameterFact(-1, "BAT_CRIT_THR")
                }

                LabelledFactSlider {
                    Layout.fillWidth:   true
                    label:              qsTr("Emergency Level")
                    fact:               controller.getParameterFact(-1, "BAT_EMERGEN_THR")
                }

                LabelledFactComboBox {
                    label:              qsTr("Failsafe Action")
                    fact:               controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
                    indexModel:         false
                }
            }
        }
    }
}
