import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

ColumnLayout {
    id:     root

    required property var controller

    property Fact _ch7Opt:  controller.getParameterFact(-1, "RC7_OPTION")
    property Fact _ch8Opt:  controller.getParameterFact(-1, "RC8_OPTION")
    property Fact _ch9Opt:  controller.getParameterFact(-1, "RC9_OPTION")
    property Fact _ch10Opt: controller.getParameterFact(-1, "RC10_OPTION")
    property Fact _ch11Opt: controller.getParameterFact(-1, "RC11_OPTION")
    property Fact _ch12Opt: controller.getParameterFact(-1, "RC12_OPTION")

    readonly property int   _firstOptionChannel:    7
    readonly property int   _lastOptionChannel:     12

    property int    _autoTuneSwitchChannelIndex:    0
    readonly property int _autoTuneOption:          17

    Component.onCompleted: calcAutoTuneChannel()

    function calcAutoTuneChannel() {
        _autoTuneSwitchChannelIndex = 0
        for (let channel = _firstOptionChannel; channel <= _lastOptionChannel; channel++) {
            let optionFact = controller.getParameterFact(-1, "RC" + channel + "_OPTION")
            if (optionFact.value === _autoTuneOption) {
                _autoTuneSwitchChannelIndex = channel - _firstOptionChannel + 1
                break
            }
        }
    }

    function setChannelAutoTuneOption(channel) {
        for (let optionChannel = _firstOptionChannel; optionChannel <= _lastOptionChannel; optionChannel++) {
            let optionFact = controller.getParameterFact(-1, "RC" + optionChannel + "_OPTION")
            if (optionFact.value === _autoTuneOption) {
                optionFact.value = 0
            }
        }
        if (channel !== 0) {
            let optionFact = controller.getParameterFact(-1, "RC" + channel + "_OPTION")
            optionFact.value = _autoTuneOption
        }
    }

    Connections { target: _ch7Opt;  function onValueChanged() { calcAutoTuneChannel() } }
    Connections { target: _ch8Opt;  function onValueChanged() { calcAutoTuneChannel() } }
    Connections { target: _ch9Opt;  function onValueChanged() { calcAutoTuneChannel() } }
    Connections { target: _ch10Opt; function onValueChanged() { calcAutoTuneChannel() } }
    Connections { target: _ch11Opt; function onValueChanged() { calcAutoTuneChannel() } }
    Connections { target: _ch12Opt; function onValueChanged() { calcAutoTuneChannel() } }

    LabelledComboBox {
        Layout.fillWidth:       true
        comboBoxPreferredWidth: ScreenTools.defaultFontPixelWidth * 30
        label:                  qsTr("Channel for AutoTune switch:")
        model:                  [qsTr("None"), qsTr("Channel 7"), qsTr("Channel 8"), qsTr("Channel 9"), qsTr("Channel 10"), qsTr("Channel 11"), qsTr("Channel 12")]
        currentIndex:           _autoTuneSwitchChannelIndex

        onActivated: (index) => {
            let channel = index
            if (channel > 0) {
                channel += 6
            }
            setChannelAutoTuneOption(channel)
        }
    }
}
