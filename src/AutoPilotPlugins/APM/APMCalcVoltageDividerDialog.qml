import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactControls
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Calculate Voltage Multiplier")
    buttons:    Dialog.Close

    property int  batteryIndex: 0
    property var  _controller:       controller
    property var  _batteryFactGroup: controller.vehicle.getFactGroup("battery" + batteryIndex)

    APMBatteryParams {
        id:             batParams
        controller:     _controller
        batteryIndex:   parent.batteryIndex
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            Layout.preferredWidth:  gridLayout.width
            wrapMode:               Text.WordWrap
            text:                   qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new adjusted voltage multiplier.")
        }

        QGCLabel {
            Layout.preferredWidth:  gridLayout.width
            wrapMode:               Text.WordWrap
            visible:                !_batteryFactGroup || _batteryFactGroup.voltage.value === 0
            text:                   qsTr("Vehicle voltage telemetry is not available. Connect to a vehicle with a powered battery to enable automatic calculation.")
            color:                  qgcPal.warningText
        }

        GridLayout {
            id:         gridLayout
            columns:    2

            QGCLabel { text: qsTr("Measured voltage:") }
            QGCTextField { id: measuredVoltage; numericValuesOnly: true }

            QGCLabel {
                text:    qsTr("Vehicle voltage:")
                visible: _batteryFactGroup && _batteryFactGroup.voltage.value !== 0
            }
            QGCLabel {
                text:    _batteryFactGroup ? _batteryFactGroup.voltage.valueString : ""
                visible: _batteryFactGroup && _batteryFactGroup.voltage.value !== 0
            }

            QGCLabel { text: qsTr("Voltage multiplier:") }
            FactLabel { fact: batParams.battVoltMult }
        }

        QGCButton {
            text:    qsTr("Calculate And Set")
            enabled: _batteryFactGroup && _batteryFactGroup.voltage.value !== 0

            onClicked: {
                let measuredVoltageValue = parseFloat(measuredVoltage.text)
                if (measuredVoltageValue === 0 || isNaN(measuredVoltageValue)) {
                    return
                }
                let newVoltageMultiplier = (measuredVoltageValue * batParams.battVoltMult.value) / _batteryFactGroup.voltage.value
                if (newVoltageMultiplier > 0) {
                    batParams.battVoltMult.value = newVoltageMultiplier
                }
            }
        }
    }
}
