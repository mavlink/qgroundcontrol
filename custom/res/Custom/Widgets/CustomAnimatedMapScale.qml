import QtQuick  
import QtQuick.Controls  
import QtQuick.Layouts  

import QGroundControl  
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.FlightMap


Item{
    // Animation for Map scale
    implicitWidth:  mapScale.width
    implicitHeight: mapScale.height
    
    property var _targetMap
    Timer {
        id: mapScaleHideTimer
        interval: 2000 
        onTriggered: {
            mapScaleFadeOutAnimation.start();
        }
    }
    NumberAnimation {
        id: mapScaleFadeOutAnimation
        target: mapScale
        property: "opacity"
        to: 0
        duration: 1000 
        easing.type: Easing.InOutQuad
    }

    NumberAnimation {
        id: mapScaleFadeInAnimation
        target: mapScale
        property: "opacity"
        to: 1
        duration: 200 
        easing.type: Easing.InOutQuad
    }

    MapScale {
        id:                         mapScale
        mapControl:                 _targetMap
        zoomButtonsVisible:         false
    }  

    Connections {
        target: _targetMap
        function onZoomLevelChanged() {
            mapScaleFadeInAnimation.stop(); 
            mapScaleFadeOutAnimation.stop(); 
            mapScaleHideTimer.stop(); 
            mapScaleFadeInAnimation.start(); 
            mapScaleHideTimer.start(); 
        }
        function onCenterChanged() { 
            mapScaleFadeInAnimation.stop();
            mapScaleFadeOutAnimation.stop();
            mapScaleHideTimer.stop();
            mapScaleFadeInAnimation.start();
            mapScaleHideTimer.start();
        }               
    } 

}