import QtQuick
import QtQuick.Controls

import QGroundControl.FactControls

Item {

    FactPanelController { id: controller; }

    // Use default component id
    TextInput {
        objectName: "testControl"
        text:       fact1.value

        property Fact fact1: controller.getParameterFact(-1, "RC_MAP_THROTTLE")

        onAccepted: fact1.value = text
    }

    // Use specific component id
    TextInput {
        text:       fact2.value

        property Fact fact2: controller.getParameterFact(1, "RC_MAP_THROTTLE")

        onAccepted: fact2.value = text
    }
}
