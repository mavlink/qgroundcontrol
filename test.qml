// Currently unused but leaving here for future conversion to QML

import QtQuick 2.0
import QtQuick.Controls 1.2
import QGroundControlFactControls 1.0
import QGroundControl.FactSystem 1.0

Item {
    width: 100
    height: 100

    FactTextInput {
        fact: parameterFacts.RC_MAP_THROTTLE
        validator: FactValidator { fact: parameterFacts.RC_MAP_THROTTLE }
    }
}
