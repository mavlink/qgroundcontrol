import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:       true
    heading:                qsTr("Return to Launch")
    visible:                activeVehicle.multiRotor

    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property Fact rtlAltFact: controller.getParameterFact(-1, "RTL_ALT_M")
    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
    property bool _rtlAltIsMeters: controller.parameterExists(-1, "noremap.RTL_ALT_M")

    FactPanelController { id: controller }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth * 2

        QGCLabel {
            id:                 label
            Layout.fillWidth:   true
            text:               qsTr("Return At")
        }

        QGCComboBox {
            id:             returnAtCombo
            sizeToContents: true
            model:          [ qsTr("Current altitude"), qsTr("Specified altitude") ]

            function setCurrentIndex() {
                if (rtlAltFact.value === 0) {
                    returnAtCombo.currentIndex = 0
                } else {
                    returnAtCombo.currentIndex = 1
                }
            }

            Component.onCompleted: setCurrentIndex()

            onActivated: (index) => {
                if (index === 0) {
                    rtlAltFact.rawValue = 0
                } else {
                    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
                    rtlAltFact.rawValue = _rtlAltIsMeters ? 15 : 1500
                }
            }

            Connections {
                target:             rtlAltFact
                onRawValueChanged:  returnAtCombo.setCurrentIndex()
            }
        }

        FactTextField {
            fact:       rtlAltFact
            enabled:    rtlAltFact.rawValue !== 0
        }
    }
}
