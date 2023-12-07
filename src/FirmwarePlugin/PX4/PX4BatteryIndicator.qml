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

            IndicatorPageGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Low Battery")

                GridLayout {
                    columns: 2
                    columnSpacing: ScreenTools.defaultFontPixelHeight

                    QGCLabel { text: qsTr("Warning Level") }
                    FactTextField {
                        Layout.fillWidth:       true
                        fact:                   controller.getParameterFact(-1, "BAT_LOW_THR")
                    }

                    QGCLabel { text: qsTr("Failsafe Level") }
                    FactTextField {
                        Layout.fillWidth:       true
                        fact:                   controller.getParameterFact(-1, "BAT_CRIT_THR")
                    }

                    QGCLabel { text: qsTr("Failsafe Action") }
                    FactComboBox {
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  ScreenTools.implicitTextFieldWidth
                        fact:                   controller.getParameterFact(-1, "COM_LOW_BAT_ACT")
                        indexModel:             false
                        sizeToContents:         true
                    }

                    QGCLabel { text: qsTr("Emergency Level") }
                    FactTextField {
                        Layout.fillWidth:       true
                        fact:                   controller.getParameterFact(-1, "BAT_EMERGEN_THR")
                    }
                }
            }
        }
    }
}
