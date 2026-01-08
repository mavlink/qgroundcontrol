import QtQuick
import QtQuick.Controls

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.AutoPilotPlugins.APM

ColumnLayout {
    property int instance: 1
    property real verticalSpacing: ScreenTools.defaultFontPixelHeight / 2
    property real horizontalSpacing: ScreenTools.defaultFontPixelWidth * 2

    id: control

    property real _sliderWidth: ScreenTools.defaultFontPixelWidth * 20
    property var _controller: gimbalParams.controller
    property var _rcRateFact: gimbalParams.rcRateFact

    function _servoChannelCount() {
        let servoIndex = 1
        while (_controller.parameterExists(-1, "SERVO" + servoIndex + "_FUNCTION")) {
            servoIndex++
        }
        return servoIndex - 1
    }

    function _rcChannelCount() {
        let rcIndex = 1
        while (_controller.parameterExists(-1, "RC" + rcIndex + "_OPTION")) {
            rcIndex++
        }
        return rcIndex - 1
    }

    function _servoChannelModel() {
        let model = [ qsTr("Disabled") ]
        let channelCount = _servoChannelCount()
        for (let i = 1; i <= channelCount; i++) {
            model.push(qsTr("Channel ") + i)
        }
        return model
    }

    function _rcChannelModel() {
        let model = [ qsTr("Disabled") ]
        let channelCount = _rcChannelCount()
        for (let i = 1; i <= channelCount; i++) {
            model.push(qsTr("Channel ") + i)
        }
        return model
    }

    APMGimbalParams { id: gimbalParams; instance: control.instance }

    RowLayout {
        Layout.fillWidth: false
        spacing: horizontalSpacing
        visible: gimbalParams.instanceCount > 0

        LabelledFactComboBox {
            label: qsTr("Gimbal Type")
            fact: gimbalParams.typeFact
            indexModel: false
        }

        LabelledFactComboBox {
            label: qsTr("Default Mode")
            fact: gimbalParams.defaultModeFact
            indexModel: false
            visible: fact !== null
        }
    }

    Loader {
        sourceComponent: gimbalParams.paramsAvailable ? configComponent : rebootRequiredComponent
    }

    Component {
        id: configComponent

        ColumnLayout {
            spacing: verticalSpacing

            SettingsGroupLayout {
                heading: qsTr("Neutral Position")

                RowLayout {
                    spacing: horizontalSpacing

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Pitch")
                        fact: gimbalParams.neutralYFact
                    }

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Yaw")
                        fact: gimbalParams.neutralZFact
                    }

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Roll")
                        fact: gimbalParams.neutralXFact
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Retracted Position")

                RowLayout {
                    spacing: horizontalSpacing

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Pitch")
                        fact: gimbalParams.retractYFact
                    }

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Yaw")
                        fact: gimbalParams.retractZFact
                    }

                    LabelledFactTextField {
                        Layout.fillWidth: true
                        label: qsTr("Roll")
                        fact: gimbalParams.retractXFact
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Axis Constraints")

                Repeater {
                    model: [
                        {
                            axisLabel: qsTr("Pitch"),
                            minFact: gimbalParams.pitchMinFact,
                            maxFact: gimbalParams.pitchMaxFact
                        },
                        {
                            axisLabel: qsTr("Yaw"),
                            minFact: gimbalParams.yawMinFact,
                            maxFact: gimbalParams.yawMaxFact
                        },
                        {
                            axisLabel: qsTr("Roll"),
                            minFact: gimbalParams.rollMinFact,
                            maxFact: gimbalParams.rollMaxFact
                        }
                    ]

                    ColumnLayout {
                        spacing: verticalSpacing

                        QGCLabel {
                            text: modelData.axisLabel
                        }

                        RowLayout {
                            spacing: horizontalSpacing

                            LabelledFactTextField {
                                Layout.fillWidth: true
                                label: qsTr("Min Angle")
                                fact: modelData.minFact
                            }

                            LabelledFactTextField {
                                Layout.fillWidth: true
                                label: qsTr("Max Angle")
                                fact: modelData.maxFact
                            }
                        }
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("RC Targetting")

                RowLayout {
                    spacing: horizontalSpacing

                    Repeater {
                        model: [
                            { comboLabel: qsTr("Pitch"), optionValue: 213 },
                            { comboLabel: qsTr("Yaw"), optionValue: 214 },
                            { comboLabel: qsTr("Roll"), optionValue: 212 }
                        ]

                        LabelledComboBox {
                            id: outputChannelCombo
                            label: modelData.comboLabel
                            model: _rcChannelModel()

                            Component.onCompleted: {
                                let maxRcChannel = _rcChannelCount()
                                for (var rcIndex = 1; rcIndex <= maxRcChannel; rcIndex++) {
                                    var parameterName = "RC" + rcIndex + "_OPTION"
                                    var functionFact = _controller.getParameterFact(-1, parameterName)
                                    if (functionFact.value == modelData.optionValue) {
                                        currentIndex = rcIndex
                                        return
                                    }
                                }
                                currentIndex = 0
                            }

                            onActivated: {
                                if (currentIndex == 0) {
                                    // Disabled selected
                                    let maxRcChannel = _rcChannelCount()
                                    for (let rcIndex = 1; rcIndex <= maxRcChannel; rcIndex++) {
                                        let parameterName = "RC" + rcIndex + "_OPTION"
                                        let functionFact = _controller.getParameterFact(-1, parameterName)
                                        if (functionFact.value == modelData.optionValue) {
                                            functionFact.rawValue = 0
                                            return
                                        }
                                    }
                                    return
                                }
                                let rcIndex = currentIndex
                                let parameterName = "RC" + rcIndex + "_OPTION"
                                _controller.getParameterFact(-1, parameterName).rawValue = modelData.optionValue
                            }
                        }
                    }
                }

                ColumnLayout {
                    spacing: 0

                    RowLayout {
                        spacing: horizontalSpacing

                        QGCRadioButton {
                            text: qsTr("Angle Control")
                            checked: !(_rcRateFact.rawValue > 0)
                            onClicked: _rcRateFact.rawValue = 0
                        }

                        QGCRadioButton {
                            text: qsTr("Rate Control")
                            checked: _rcRateFact.rawValue > 0
                            onClicked: _rcRateFact.rawValue = 90
                        }

                        LabelledFactTextField {
                            Layout.fillWidth: true
                            label: qsTr("Rate")
                            fact: _rcRateFact
                        }
                    }
                }
            }

            SettingsGroupLayout {
                heading: qsTr("Servo Controlled Gimbal")
                visible: gimbalParams.typeFact.rawValue === 1

                Repeater {
                    model: [
                        { axisLabel: qsTr("Pitch"), functionValue: 7, stabilizeFact: gimbalParams.pitchLeadFact },
                        { axisLabel: qsTr("Yaw"), functionValue: 6, stabilizeFact: null },
                        { axisLabel: qsTr("Roll"), functionValue: 8, stabilizeFact: gimbalParams.rollLeadFact }
                    ]

                    ColumnLayout {
                        spacing: verticalSpacing

                        property bool servoChannelValid: outputChannelCombo.currentIndex > 0
                        property int validServoChannel: servoChannelValid ? outputChannelCombo.currentIndex : 1
                        property string servoPrefix: "SERVO" + validServoChannel + "_"
                        property bool hasStabilizeParam: modelData.stabilizeFact !== null

                        RowLayout {
                            Layout.fillWidth: false
                            spacing: horizontalSpacing

                            QGCLabel {
                                text: modelData.axisLabel
                            }

                            FactCheckBox {
                                text: qsTr("Servo Reversed")
                                fact: _controller.getParameterFact(-1, servoPrefix + "REVERSED")
                                enabled: servoChannelValid
                            }
                        }

                        RowLayout {
                            Layout.fillWidth: hasStabilizeParam
                            spacing: horizontalSpacing

                            LabelledComboBox {
                                Layout.fillWidth: hasStabilizeParam
                                id: outputChannelCombo
                                label: qsTr("Output Channel")
                                model: _servoChannelModel()

                                Component.onCompleted: {
                                    let maxServoChannel = _servoChannelCount()
                                    for (var servoIndex = 1; servoIndex <= maxServoChannel; servoIndex++) {
                                        var parameterName = "SERVO" + servoIndex + "_FUNCTION"
                                        var functionFact = _controller.getParameterFact(-1, parameterName)
                                        if (functionFact.value == modelData.functionValue) {
                                            currentIndex = servoIndex
                                            return
                                        }
                                    }
                                    currentIndex = 0
                                }

                                onActivated: {
                                    if (currentIndex == 0) {
                                        // Disabled selected
                                        let maxServoChannel = _servoChannelCount()
                                        for (let servoIndex = 1; servoIndex <= maxServoChannel; servoIndex++) {
                                            let parameterName = "SERVO" + servoIndex + "_FUNCTION"
                                            let functionFact = _controller.getParameterFact(-1, parameterName)
                                            if (functionFact.value == modelData.functionValue) {
                                                functionFact.rawValue = 0
                                                return
                                            }
                                        }
                                        return
                                    }
                                    let servoIndex = currentIndex
                                    let parameterName = "SERVO" + servoIndex + "_FUNCTION"
                                    _controller.getParameterFact(-1, parameterName).rawValue = modelData.functionValue
                                }
                            }

                            LabelledFactTextField {
                                Layout.fillWidth: true
                                label: qsTr("Stabilization Lead")
                                fact: modelData.stabilizeFact
                                visible: hasStabilizeParam
                                enabled: servoChannelValid
                            }
                        }

                        RowLayout {
                            Layout.alignment: Qt.AlignHCenter
                            Layout.fillWidth: true
                            spacing: horizontalSpacing
                            enabled: servoChannelValid

                            LabelledFactTextField {
                                Layout.fillWidth: true
                                label: qsTr("Min PWM")
                                fact: _controller.getParameterFact(-1, servoPrefix + "MIN")
                            }

                            LabelledFactTextField {
                                Layout.fillWidth: true
                                label: qsTr("Max PWM")
                                fact: _controller.getParameterFact(-1, servoPrefix + "MAX")
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: rebootRequiredComponent

        QGCLabel {
            text: qsTr("Gimbal settings will be available after rebooting the vehicle.")
            visible: gimbalParams.typeFact.rawValue !== 0
        }
    }
}
