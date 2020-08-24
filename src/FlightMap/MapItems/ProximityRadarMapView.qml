/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.12
import QtLocation               5.3
import QtPositioning            5.3
import QtGraphicalEffects       1.0

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.Controls      1.0



/// Marker for displaying a vehicle location on the map
MapQuickItem {
    id:             _root
    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    map
    property double heading:        vehicle ? vehicle.heading.value : Number.NaN    ///< Vehicle heading, NAN for none

    anchorPoint.x:  vehicleItem.width  / 2
    anchorPoint.y:  vehicleItem.height / 2
    visible:        coordinate.isValid

    property bool   _adsbVehicle:   vehicle ? false : true
    property real   _uavSize:       ScreenTools.defaultFontPixelHeight * 5
    property real   _adsbSize:      ScreenTools.defaultFontPixelHeight * 2.5
    property var    _map:           map
    property bool   _multiVehicle:  QGroundControl.multiVehicleManager.vehicles.count > 1
    property real   _ratio:         1

    property var    _distanceSensor:        vehicle?vehicle.distanceSensors:null
    property var    _maxSensor:             _distanceSensor?_distanceSensor.maxDistance.value:1 //need to change in cc
    property var    _rotationNone:          _distanceSensor?_distanceSensor.rotationNone.value:0
    property var    _rotationYaw45:         _distanceSensor?_distanceSensor.rotationYaw45.value:0
    property var    _rotationYaw90:         _distanceSensor?_distanceSensor.rotationYaw90.value:0
    property var    _rotationYaw135:        _distanceSensor?_distanceSensor.rotationYaw135.value:0
    property var    _rotationYaw180:        _distanceSensor?_distanceSensor.rotationYaw180.value:0
    property var    _rotationYaw225:        _distanceSensor?_distanceSensor.rotationYaw225.value:0
    property var    _rotationYaw270:        _distanceSensor?_distanceSensor.rotationYaw270.value:0
    property var    _rotationYaw315:        _distanceSensor?_distanceSensor.rotationYaw315.value:0
    property var    _rottab:                [_rotationNone,_rotationYaw45,_rotationYaw90,_rotationYaw135,_rotationYaw180,_rotationYaw225,_rotationYaw270,_rotationYaw315]

    function calcSize(){
        if(_map) {
            var scaleLinePixelLength = 100
            var leftCoord  = _map.toCoordinate(Qt.point(0, 0), false /* clipToViewPort */)
            var rightCoord = _map.toCoordinate(Qt.point(scaleLinePixelLength, 0), false /* clipToViewPort */)
            var scaleLineMeters = Math.round(leftCoord.distanceTo(rightCoord))
        }
        _ratio=scaleLinePixelLength/scaleLineMeters;
    }

    on_RotationNoneChanged:     vehicleSensors.requestPaint()
    on_RotationYaw45Changed:    vehicleSensors.requestPaint()
    on_RotationYaw90Changed:    vehicleSensors.requestPaint()
    on_RotationYaw135Changed:   vehicleSensors.requestPaint()
    on_RotationYaw180Changed:   vehicleSensors.requestPaint()
    on_RotationYaw225Changed:   vehicleSensors.requestPaint()
    on_RotationYaw270Changed:   vehicleSensors.requestPaint()
    on_RotationYaw315Changed:   vehicleSensors.requestPaint()



    Connections {
        target:             map
        onWidthChanged:     scaleTimer.restart()
        onHeightChanged:    scaleTimer.restart()
        onZoomLevelChanged: scaleTimer.restart()
    }

    Timer {
        id:                 scaleTimer
        interval:           100
        running:            false
        repeat:             false
        onTriggered:        calcSize()
    }

    sourceItem: Item {
        id:         vehicleItem
        width:      detectionLimitCircle.width
        height:     detectionLimitCircle.height
        opacity:    0.5

        Canvas{
            id:                 vehicleSensors
            anchors.fill:       detectionLimitCircle
            transform: Rotation {
                origin.x:       detectionLimitCircle.width  / 2
                origin.y:       detectionLimitCircle.height / 2
                angle:          isNaN(heading) ? 0 : heading
            }
            function deg2rad(degrees)
            {
              var pi = Math.PI;
              return degrees * (pi/180);
            }
            onPaint: {
                    if(_distanceSensor)
                        {
                            var ctx = getContext("2d");
                            ctx.reset();
                            ctx.translate(width/2,height/2)
                            ctx.rotate(-Math.PI/2);
                            ctx.lineWidth = 5;
                            ctx.strokeStyle = Qt.rgba(1, 0, 0, 1);
                            for(var i=0; i<_rottab.length;i++){
                                var a=deg2rad(360-22.5)+Math.PI/4*i;
                                ctx.beginPath();
                                ctx.arc(0,0,_rottab[i]*_ratio,   a,a+Math.PI/4,false);
                                ctx.stroke();
                            }

                        }
                }
        }

        Rectangle {
            id:                 detectionLimitCircle
            width:              _maxSensor*2*_ratio
            height:             _maxSensor*2*_ratio
            anchors.fill:       detectionLimitCircle
            color:              Qt.rgba(1,1,1,0)
            border.color:       Qt.rgba(1,1,1,1)
            border.width:       5
            radius:             width*0.5
            transform: Rotation {
                origin.x:       detectionLimitCircle.width  / 2
                origin.y:       detectionLimitCircle.height / 2
                angle:          isNaN(heading) ? 0 : heading
            }
        }

        Component.onCompleted: {
                calcSize();
            }
    }
}

