import QtQuick          2.3
import QtQuick.Controls 1.2

import QGroundControl.FactSystem 1.0
import QGroundControl.FactControls 1.0
import QGroundControl.Controls 1.0
import QGroundControl.Palette 1.0

Item {
    anchors.fill:   parent

    FactPanelController { id: controller; }

    property Fact _rc5Function:         controller.getParameterFact(-1, "SERVO5_FUNCTION")
    property Fact _rc6Function:         controller.getParameterFact(-1, "SERVO6_FUNCTION")
    property Fact _rc7Function:         controller.getParameterFact(-1, "SERVO7_FUNCTION")
    property Fact _rc8Function:         controller.getParameterFact(-1, "SERVO8_FUNCTION")
    property Fact _rc9Function:         controller.getParameterFact(-1, "SERVO9_FUNCTION")
    property Fact _rc10Function:        controller.getParameterFact(-1, "SERVO10_FUNCTION")
    property Fact _rc11Function:        controller.getParameterFact(-1, "SERVO11_FUNCTION")
    property Fact _rc12Function:        controller.getParameterFact(-1, "SERVO12_FUNCTION")
    property Fact _rc13Function:        controller.getParameterFact(-1, "SERVO13_FUNCTION")
    property Fact _rc14Function:        controller.getParameterFact(-1, "SERVO14_FUNCTION")

    readonly property int   _rcFunctionRCIN9:               59
    readonly property int   _rcFunctionRCIN10:              60
    readonly property int   _firstLightsOutChannel:         5
    readonly property int   _lastLightsOutChannel:          14

    Component.onCompleted: {
        calcLightOutValues()
    }

    /// Light output channels are stored in SERVO#_FUNCTION parameters. We need to loop through those
    /// to find them and setup the ui accordindly.
    function calcLightOutValues() {
        lightsLoader.lights1OutIndex = 0
        lightsLoader.lights2OutIndex = 0
        for (var channel=_firstLightsOutChannel; channel<=_lastLightsOutChannel; channel++) {
            var functionFact = controller.getParameterFact(-1, "SERVO" + channel + "_FUNCTION")
            if (functionFact.value == _rcFunctionRCIN9) {
                lightsLoader.lights1OutIndex = channel - 4
            } else if (functionFact.value == _rcFunctionRCIN10) {
                lightsLoader.lights2OutIndex = channel - 4
            }
        }
    }

    // Whenever any SERVO#_FUNCTION parameters chagnes we need to go looking for light output channels again
    Connections { target: _rc5Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc6Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc7Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc8Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc9Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc10Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc11Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc12Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc13Function; onValueChanged: calcLightOutValues() }
    Connections { target: _rc14Function; onValueChanged: calcLightOutValues() }

    ListModel {
        id: lightsOutModel
        ListElement { text: qsTr("Disabled"); value: 0 }
        ListElement { text: qsTr("Channel 5"); value: 5 }
        ListElement { text: qsTr("Channel 6"); value: 6 }
        ListElement { text: qsTr("Channel 7"); value: 7 }
        ListElement { text: qsTr("Channel 8"); value: 8 }
        ListElement { text: qsTr("Channel 9"); value: 9 }
        ListElement { text: qsTr("Channel 10"); value: 10 }
        ListElement { text: qsTr("Channel 11"); value: 11 }
        ListElement { text: qsTr("Channel 12"); value: 12 }
        ListElement { text: qsTr("Channel 13"); value: 13 }
        ListElement { text: qsTr("Channel 14"); value: 14 }
    }

    Loader {
        id:                 lightsLoader

        property int    lights1OutIndex:         0
        property int    lights2OutIndex:         0
        property int    lights1Function:         _rcFunctionRCIN9
        property int    lights2Function:         _rcFunctionRCIN10
    }

    Column {
        anchors.fill:       parent

        VehicleSummaryRow {
            labelText:  qsTr("Lights Output 1")
            valueText:  lightsOutModel.get(lightsLoader.lights1OutIndex).text
        }

        VehicleSummaryRow {
            labelText:  qsTr("Lights Output 2")
            valueText:  lightsOutModel.get(lightsLoader.lights2OutIndex).text
        }
    }
}
