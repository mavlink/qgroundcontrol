import QtQuick          2.5
import QtQuick.Controls 1.2

import QGroundControl.FactSystem    1.0
import QGroundControl.FactControls  1.0
import QGroundControl.Controls      1.0
import QGroundControl.Controllers   1.0
import QGroundControl.Palette       1.0
import QGroundControl.ScreenTools   1.0

FactPanel {
    id:     panel
    width:  grid.width
    height: grid.height
    color:  qgcPal.windowShade

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }
    FactPanelController { id: controller; factPanel: panel }

    property Fact _mountRCInTilt:   controller.getParameterFact(-1, "MNT_RC_IN_TILT")
    property Fact _mountRCInRoll:   controller.getParameterFact(-1, "MNT_RC_IN_ROLL")
    property Fact _mountRCInPan:    controller.getParameterFact(-1, "MNT_RC_IN_PAN")
    property Fact _mountType:       controller.getParameterFact(-1, "MNT_TYPE")

    Grid {
        id:         grid
        rows:       4
        columns:    2
        spacing:    ScreenTools.defaultFontPixelWidth / 2

        QGCLabel { text: "Gimbal type:" }
        QGCLabel { text:  _mountType.enumStringValue }

        QGCLabel { text: "Tilt input channel:" }
        QGCLabel { text:  _mountRCInTilt.enumStringValue }

        QGCLabel { text: "Pan input channel:" }
        QGCLabel { text:  _mountRCInPan.enumStringValue }

        QGCLabel { text: "Roll input channel:" }
        QGCLabel { text:  _mountRCInRoll.enumStringValue }
    }
}
