import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.Controls

/// Marker for displaying an ADS-B vehicle location on the map
MapQuickItem {
    id: _root

    property var    map
    property double altitude:   Number.NaN  ///< NAN to not show
    property string callsign:   ""          ///< Vehicle callsign
    property double heading:    Number.NaN  ///< Vehicle heading, NAN for none
    property real   size:       ScreenTools.defaultFontPixelHeight * 3
    property bool   alert:      false       ///< Collision alert

    anchorPoint.x:  vehicleItem.width  / 2
    anchorPoint.y:  vehicleItem.height / 2
    visible:        coordinate.isValid

    sourceItem: Item {
        id:     vehicleItem
        width:  vehicleIcon.width
        height: vehicleIcon.height

        Image {
            id:                 vehicleIcon
            source:             alert ? "/qmlimages/AlertAircraft.svg" : "/qmlimages/AwarenessAircraft.svg"
            mipmap:             true
            width:              _root.size
            sourceSize.width:   _root.size
            fillMode:           Image.PreserveAspectFit
            transform: Rotation {
                origin.x:   vehicleIcon.width  / 2
                origin.y:   vehicleIcon.height / 2
                angle:      isNaN(heading) ? 0 : heading
            }
        }

        QGCMapLabel {
            anchors.top:                parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            map:                        _root.map
            font.pointSize:             ScreenTools.defaultFontPointSize
            visible:                    !isNaN(altitude)
            text:                       visible ? QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnitsString(altitude, 0) + "\n" + callsign : ""
        }
    }
}
