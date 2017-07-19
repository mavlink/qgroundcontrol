/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0

/// Marker for displaying a vehicle location on the map
MapQuickItem {
    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    map
    property double altitude:       Number.NaN                                      ///< NAN to not show
    property double heading:        vehicle ? vehicle.heading.value : Number.NaN    ///< Vehicle heading, NAN for none
    property real   size:           _adsbVehicle ? _adsbSize : _uavSize             /// Size for icon

    anchorPoint.x:  vehicleItem.width  / 2
    anchorPoint.y:  vehicleItem.height / 2
    visible:        coordinate.isValid

    property bool   _adsbVehicle:   vehicle ? false : true
    property real   _uavSize:       ScreenTools.defaultFontPixelHeight * 5
    property real   _adsbSize:      ScreenTools.defaultFontPixelHeight * 1.5
    property var    _map:           map
    property bool   _multiVehicle:  QGroundControl.multiVehicleManager.vehicles.count > 1

    sourceItem: Item {
        id:         vehicleItem
        width:      vehicleIcon.width
        height:     vehicleIcon.height
        opacity:    vehicle ? (vehicle.active ? 1.0 : 0.5) : 1.0

        Image {
            id:                 vehicleIcon
            source:             _adsbVehicle ? "/qmlimages/adsbVehicle.svg" : vehicle.vehicleImageOpaque
            mipmap:             true
            width:              size
            sourceSize.width:   size
            fillMode:           Image.PreserveAspectFit

            transform: Rotation {
                origin.x:       vehicleIcon.width  / 2
                origin.y:       vehicleIcon.height / 2
                angle:          isNaN(heading) ? 0 : heading
            }
        }

        QGCMapLabel {
            anchors.top:                parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            map:                        _map
            text:                       vehicleLabelText
            font.pointSize:             ScreenTools.smallFontPointSize
            visible:                    _adsbVehicle ? !isNaN(altitude) : _multiVehicle

            property string vehicleLabelText: visible ?
                                                  (_adsbVehicle ?
                                                       QGroundControl.metersToAppSettingsDistanceUnits(altitude).toFixed(0) + " " + QGroundControl.appSettingsDistanceUnitsString :
                                                       (_multiVehicle ? qsTr("Vehicle %1").arg(vehicle.id) : "")) :
                                                  ""

        }
    }
}
