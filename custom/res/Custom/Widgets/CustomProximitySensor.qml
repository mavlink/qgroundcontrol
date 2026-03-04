import QtQuick
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.FlightDisplay

Item{
    id:             _root
    visible:        proximityValues.telemetryAvailable || true

    property var    vehicle                                                         
    property double heading:    vehicle ? vehicle.heading.value : Number.NaN    


    property real _ratio:     1
    property real size

    width:  size
    height: size


    ProximityRadarValues{
        id: proximityValues
        vehicle: _root.vehicle
        onRotationValueChanged: vehicleSensors.requestPaint()
    }

    Canvas{
        id:                 vehicleSensors
        anchors.fill:       detectionLimitCircle

        transform: Rotation {
            origin.x:       detectionLimitCircle.width  / 2
            origin.y:       detectionLimitCircle.height / 2
            angle:          isNaN(heading) ? 0 : heading
        }

        function deg2rad(degrees) {
            var pi = Math.PI;
            return degrees * (pi/180);
        }

        onPaint: {
            var ctx = getContext("2d");
            ctx.reset();
            ctx.translate(width/2, height/2)
            ctx.rotate(-Math.PI/2);
            ctx.lineWidth = 5;
            ctx.strokeStyle = Qt.rgba(1, 0, 0, 1);
            for(var i=0; i<proximityValues.rgRotationValues.length; i++){
                var rotationValue = proximityValues.rgRotationValues[i]
                if (!isNaN(rotationValue)) {
                    var a=deg2rad(360-22.5)+Math.PI/4*i;
                    ctx.beginPath();
                    ctx.arc(0, 0, rotationValue * _ratio, a, a + Math.PI/4,false);
                    ctx.stroke();
                }
            }
        }
    }

    Rectangle {
        id:                 detectionLimitCircle
        anchors.fill:       parent
        color:              Qt.rgba(0, 0, 0, 0.7)
        border.color:       Qt.rgba(1,1,1,1)
        border.width:       5
        radius:             width * 0.5

        transform: Rotation {
            origin.x:       detectionLimitCircle.width  / 2
            origin.y:       detectionLimitCircle.height / 2
            angle:          isNaN(heading) ? 0 : heading
        }
    }


    // Column {
    //     id: columnLayout
    //     anchors.centerIn: parent
    //     spacing: 2

    //     QGCLabel { text: "telemetryAvailable: " + proximityValues.telemetryAvailable }
    //     QGCLabel { text: "rotationNoneValue: " + proximityValues.rotationNoneValue }
    //     QGCLabel { text: "rotationYaw45Value: " + proximityValues.rotationYaw45Value }
    //     QGCLabel { text: "rotationYaw90Value: " + proximityValues.rotationYaw90Value }
    //     QGCLabel { text: "rotationYaw135Value: " + proximityValues.rotationYaw135Value }
    //     QGCLabel { text: "rotationYaw180Value: " + proximityValues.rotationYaw180Value }
    //     QGCLabel { text: "rotationYaw225Value: " + proximityValues.rotationYaw225Value }
    //     QGCLabel { text: "rotationYaw270Value: " + proximityValues.rotationYaw270Value }
    //     QGCLabel { text: "rotationYaw315Value: " + proximityValues.rotationYaw315Value }
    //     QGCLabel { text: "maxDistance: " + proximityValues.maxDistance }
    //     QGCLabel { text: "rotationNoneValueString: " + proximityValues.rotationNoneValueString }
    //     QGCLabel { text: "rotationYaw45ValueString: " + proximityValues.rotationYaw45ValueString }
    //     QGCLabel { text: "rotationYaw90ValueString: " + proximityValues.rotationYaw90ValueString }
    //     QGCLabel { text: "rotationYaw135ValueString: " + proximityValues.rotationYaw135ValueString }
    //     QGCLabel { text: "rotationYaw180ValueString: " + proximityValues.rotationYaw180ValueString }
    //     QGCLabel { text: "rotationYaw225ValueString: " + proximityValues.rotationYaw225ValueString }
    //     QGCLabel { text: "rotationYaw270ValueString: " + proximityValues.rotationYaw270ValueString }
    //     QGCLabel { text: "rotationYaw315ValueString: " + proximityValues.rotationYaw315ValueString }
    //     QGCLabel { text: "rgRotationValues: [" + proximityValues.rgRotationValues.join(", ") + "]" }
    //     QGCLabel { text: "rgRotationValueStrings: [" + proximityValues.rgRotationValueStrings.join(", ") + "]" }
    //     QGCLabel { text: "_noValueStr: " + proximityValues._noValueStr }
    // }
}