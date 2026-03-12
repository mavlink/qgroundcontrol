import QtQuick

import QGroundControl

/// Manages access to ArduPilot mount parameters
Item {
    /// The MNT#_ parameters this object manages
    property int instance: 1

    property var controller: FactPanelController {}

    /// Number of MNT#_ instances available, 0 indicates no gimbal support
    readonly property int instanceCount: _instanceCount

    /// True if gimbal parameters are available, False indicates MNT#_TYPE disabled, or reboot required to get params
    property bool paramsAvailable: false

    property Fact typeFact: controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "TYPE", false)
    property Fact defaultModeFact: controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "DEFLT_MODE", false)

    property Fact optionsFact: null
    property Fact neutralXFact: null
    property Fact neutralYFact: null
    property Fact neutralZFact: null
    property Fact retractXFact: null
    property Fact retractYFact: null
    property Fact retractZFact: null
    property Fact pitchMinFact: null
    property Fact pitchMaxFact: null
    property Fact rollMinFact: null
    property Fact rollMaxFact: null
    property Fact yawMinFact: null
    property Fact yawMaxFact: null
    property Fact rcRateFact: null
    property Fact pitchLeadFact: null
    property Fact rollLeadFact: null

    property int _instanceCount: _instancedParamCount("MNT#_TYPE")
    property string _prefixTemplate: "MNT#_"

    function _instancedParamCount(paramNameTemplate) {
        let instanceIndex = 1
        while (controller.parameterExists(-1, paramNameTemplate.replace("#", instanceIndex))) {
            instanceIndex++
        }
        return instanceIndex - 1
    }

    Component.onCompleted: {
        if (defaultModeFact === null) {
            paramsAvailable = false
        } else {
            optionsFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "OPTIONS")
            neutralXFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "NEUTRAL_X")
            neutralYFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "NEUTRAL_Y")
            neutralZFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "NEUTRAL_Z")
            retractXFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "RETRACT_X")
            retractYFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "RETRACT_Y")
            retractZFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "RETRACT_Z")
            pitchMinFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "PITCH_MIN")
            pitchMaxFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "PITCH_MAX")
            rollMinFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "ROLL_MIN")
            rollMaxFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "ROLL_MAX")
            yawMinFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "YAW_MIN")
            yawMaxFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "YAW_MAX")
            rcRateFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "RC_RATE")
            pitchLeadFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "LEAD_PTCH")
            rollLeadFact = controller.getParameterFact(-1, _prefixTemplate.replace("#", instance) + "LEAD_RLL")
            paramsAvailable = true
        }
    }
}
