import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactControls
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Calculate Voltage Divider")
    buttons:    Dialog.Close

    property int  batteryIndex: 1
    property var  _controller:       controller
    property var  _batteryFactGroup: controller.vehicle.getFactGroup("battery" + (batteryIndex - 1))

    BatteryParams {
        id:             batParams
        controller:     _controller
        batteryIndex:   parent.batteryIndex
    }

    ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight

        QGCLabel {
            Layout.preferredWidth:  gridLayout.width
            wrapMode:               Text.WordWrap
            text:                   qsTr("Measure battery voltage using an external voltmeter and enter the value below. Click Calculate to set the new voltage multiplier.")
        }

        GridLayout {
            id:         gridLayout
            columns:    2

            QGCLabel { text: qsTr("Measured voltage:") }
            QGCTextField { id: measuredVoltage; numericValuesOnly: true }

            QGCLabel { text: qsTr("Vehicle voltage:") }
            QGCLabel { text: _batteryFactGroup.voltage.valueString }

            QGCLabel { text: qsTr("Voltage divider:") }
            FactLabel { fact: batParams.battVoltageDivider }
        }

        QGCButton {
            text: qsTr("Calculate")

            onClicked: {
                var measuredVoltageValue = parseFloat(measuredVoltage.text)
                if (measuredVoltageValue === 0 || isNaN(measuredVoltageValue)) {
                    return
                }
                var newVoltageDivider = (measuredVoltageValue * batParams.battVoltageDivider.value) / _batteryFactGroup.voltage.value
                if (newVoltageDivider > 0) {
                    batParams.battVoltageDivider.value = newVoltageDivider
                }
            }
        }
    }
}
