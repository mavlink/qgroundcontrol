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
//import QGroundControl.Controllers   1.0



/// Marker for displaying a vehicle location on the map
Item {
    id:             _root
    anchors.fill: parent

    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    range

    property var    _distanceSensor:        vehicle?vehicle.distanceSensors:null
    property var    _range:                 range?range:6 // default 6m view

    property var    _rotationNone:          _distanceSensor?_distanceSensor.rotationNone.value:0
    property var    _rotationYaw45:         _distanceSensor?_distanceSensor.rotationYaw45.value:0
    property var    _rotationYaw90:         _distanceSensor?_distanceSensor.rotationYaw90.value:0
    property var    _rotationYaw135:        _distanceSensor?_distanceSensor.rotationYaw135.value:0
    property var    _rotationYaw180:        _distanceSensor?_distanceSensor.rotationYaw180.value:0
    property var    _rotationYaw225:        _distanceSensor?_distanceSensor.rotationYaw225.value:0
    property var    _rotationYaw270:        _distanceSensor?_distanceSensor.rotationYaw270.value:0
    property var    _rotationYaw315:        _distanceSensor?_distanceSensor.rotationYaw315.value:0
    property var    _rottab:                [_rotationNone,_rotationYaw45,_rotationYaw90,_rotationYaw135,_rotationYaw180,_rotationYaw225,_rotationYaw270,_rotationYaw315]

//    on_RottabChanged: _sectorViewEllipsoid.requestPaint()
    on_RotationNoneChanged:     _sectorViewEllipsoid.requestPaint()
    on_RotationYaw45Changed:    _sectorViewEllipsoid.requestPaint()
    on_RotationYaw90Changed:    _sectorViewEllipsoid.requestPaint()
    on_RotationYaw135Changed:   _sectorViewEllipsoid.requestPaint()
    on_RotationYaw180Changed:   _sectorViewEllipsoid.requestPaint()
    on_RotationYaw225Changed:   _sectorViewEllipsoid.requestPaint()
    on_RotationYaw270Changed:   _sectorViewEllipsoid.requestPaint()
    on_RotationYaw315Changed:   _sectorViewEllipsoid.requestPaint()

    property var _minlength: Math.min(_root.width,_root.height)
    property var _ratio: (_minlength/2)/_root._range

    Canvas{
        id:_sectorViewEllipsoid
        anchors.fill: _root
        opacity:    0.5
        visible: _distanceSensor?true:false
        onPaint: {
                if(_distanceSensor) {
                    var ctx = getContext("2d");
                    ctx.reset();
                    ctx.translate(width/2,height/2)
                    ctx.strokeStyle = Qt.rgba(1, 0, 0, 1);
                    ctx.lineWidth = width/100;
                    ctx.scale(_root.width  / _minlength,_root.height / _minlength);
                    ctx.rotate(-Math.PI/2-Math.PI/8);
                    for(var i=0; i< _rottab.length; i++)
                    {
                        var a=Math.PI/4*i;
                        ctx.beginPath();
                        ctx.arc(0,0,_rottab[i]*_ratio,0+a+Math.PI/50,Math.PI/4+a-Math.PI/50,false);
                        ctx.stroke();
                    }
                }
            }
    }
        Item{
            anchors.fill: parent
            visible: _distanceSensor?true:false
            Repeater{
                model: _rottab
                QGCLabel{
                            x:                      _sectorViewEllipsoid.width  / 2-width/2
                            y:                      _sectorViewEllipsoid.height / 2-height/2
                            text:                   modelData
                            font.family:            ScreenTools.demiboldFontFamily
                            transform: Translate {
                                        x: Math.cos(-Math.PI/2+Math.PI/4*index)*(modelData*_ratio)
                                        y: Math.sin(-Math.PI/2+Math.PI/4*index)*(modelData*_ratio)
                                        }
                        }
            }
            transform: Scale {
                            origin.x:       _sectorViewEllipsoid.width  / 2
                            origin.y:       _sectorViewEllipsoid.height / 2
                            xScale:         _root.width  / _minlength
                            yScale:         _root.height / _minlength
                        }


        }

}

