import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactControls
import QGroundControl.Controls

QGCPopupDialog {
    title:      qsTr("Calculate Amps per Volt")
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
            text:                   qsTr("Measure current draw using an external current meter and enter the value below. Click Calculate to set the new amps per volt value.")
        }

        QGCLabel {
            Layout.preferredWidth:  gridLayout.width
            wrapMode:               Text.WordWrap
            visible:                !_batteryFactGroup || _batteryFactGroup.current.value === 0
            text:                   qsTr("Vehicle current telemetry is not available. Connect to a vehicle with a powered battery to enable automatic calculation.")
            color:                  qgcPal.warningText
        }

        GridLayout {
            id:         gridLayout
            columns:    2

            QGCLabel { text: qsTr("Measured current:") }
            QGCTextField { id: measuredCurrent; numericValuesOnly: true }

            QGCLabel {
                text:    qsTr("Vehicle current:")
                visible: _batteryFactGroup && _batteryFactGroup.current.value !== 0
            }
            QGCLabel {
                text:    _batteryFactGroup ? _batteryFactGroup.current.valueString : ""
                visible: _batteryFactGroup && _batteryFactGroup.current.value !== 0
            }

            QGCLabel { text: qsTr("Amps per volt:") }
            FactLabel { fact: batParams.battAmpPerVolt }
        }

        QGCButton {
            text:    qsTr("Calculate And Set")
            enabled: _batteryFactGroup && _batteryFactGroup.current.value !== 0

            onClicked: {
                let measuredCurrentValue = parseFloat(measuredCurrent.text)
                if (measuredCurrentValue === 0 || isNaN(measuredCurrentValue)) {
                    return
                }
                let newAmpsPerVolt = (measuredCurrentValue * batParams.battAmpPerVolt.value) / _batteryFactGroup.current.value
                if (newAmpsPerVolt !== 0) {
                    batParams.battAmpPerVolt.value = newAmpsPerVolt
                }
            }
        }
    }
}
