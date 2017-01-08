import QtQuick          2.2
import QtQuick.Controls 1.2

import QGroundControl.Controls  1.0
import QGroundControl.Palette   1.0

/// Text control used for displaying text of Maps
QGCLabel {
    property var map

    QGCMapPalette { id: mapPal; lightColors: map.isSatelliteMap }

    color:      mapPal.text
    style:      Text.Outline
    styleColor: mapPal.textOutline
}
