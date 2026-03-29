import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactControls
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Calculate Amps per Volt")
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
            text:                   qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
        }

        GridLayout {
            id:         gridLayout
            columns:    2

            QGCLabel { text: qsTr("Measured current:") }
            QGCTextField { id: measuredCurrent; numericValuesOnly: true }

            QGCLabel { text: qsTr("Vehicle current:") }
            QGCLabel { text: _batteryFactGroup.current.valueString }

            QGCLabel { text: qsTr("Amps per volt:") }
            FactLabel { fact: batParams.battAmpsPerVolt }
        }

        QGCButton {
            text: qsTr("Calculate")

            onClicked: {
                var measuredCurrentValue = parseFloat(measuredCurrent.text)
                if (measuredCurrentValue === 0 || isNaN(measuredCurrentValue)) {
                    return
                }
                var newAmpsPerVolt = (measuredCurrentValue * batParams.battAmpsPerVolt.value) / _batteryFactGroup.current.value
                if (newAmpsPerVolt != 0) {
                    batParams.battAmpsPerVolt.value = newAmpsPerVolt
                }
            }
        }
    }
}
