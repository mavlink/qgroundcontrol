import QtQuick

import QGroundControl
import QGroundControl.Controls

/// The palette with the colour that marks each satellite constellation in the receiver's satellite views.
QGCPalette {
    id: root

    /// The colour of @a constellation, a GPSSatelliteModel.Constellation.
    function constellationColor(constellation: int): color {
        switch (constellation) {
        case GPSSatelliteModel.GPS:
            return root.colorGreen
        case GPSSatelliteModel.GLONASS:
            return root.colorRed
        case GPSSatelliteModel.Galileo:
            return root.colorBlue
        case GPSSatelliteModel.BeiDou:
            return root.colorOrange
        case GPSSatelliteModel.QZSS:
            return root.colorYellow
        case GPSSatelliteModel.SBAS:
            return root.colorGrey
        case GPSSatelliteModel.NavIC:
            return root.colorYellowGreen
        default:
            return root.textSecondary
        }
    }
}
