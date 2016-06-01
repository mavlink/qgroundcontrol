/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


/**
 * @file
 *   @brief QGC Map Background
 *   @author Gus Grubba <mavlink@grubba.com>
 */

import QtQuick          2.4
import QtQuick.Controls 1.3
import QtLocation       5.3
import QtPositioning    5.3

import QGroundControl                       1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.Controls              1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Vehicle               1.0
import QGroundControl.Mavlink               1.0

Map {
    id: _map

    property string mapName:            'defaultMap'
    property string mapType:            QGroundControl.flightMapSettings.mapTypeForMapName(mapName)
//  property alias  mapWidgets:         controlWidgets
    property bool   isSatelliteMap:     mapType == "Satellite Map" || mapType == "Hybrid Map"
    property bool   showScale:          false

    readonly property real  maxZoomLevel: 20
    property variant        scaleLengths: [5, 10, 25, 50, 100, 150, 250, 500, 1000, 2000, 5000, 10000, 20000, 50000, 100000, 200000, 500000, 1000000, 2000000]

    function formatDistance(meters)
    {
        var dist = Math.round(meters)
        if (dist > 1000 ){
            if (dist > 100000){
                dist = Math.round(dist / 1000)
            }
            else{
                dist = Math.round(dist / 100)
                dist = dist / 10
            }
            dist = dist + " km"
        }
        else{
            dist = dist + " m"
        }
        return dist
    }

    function calculateScale() {
        var coord1, coord2, dist, text, f
        f = 0
        coord1 = _map.toCoordinate(Qt.point(0, scale.y))
        coord2 = _map.toCoordinate(Qt.point(0 + scaleImage.sourceSize.width, scale.y))
        dist = Math.round(coord1.distanceTo(coord2))
        if (dist === 0) {
            // not visible
        } else {
            for (var i = 0; i < scaleLengths.length - 1; i++) {
                if (dist < (scaleLengths[i] + scaleLengths[i+1]) / 2 ) {
                    f = scaleLengths[i] / dist
                    dist = scaleLengths[i]
                    break;
                }
            }
            if (f === 0) {
                f = dist / scaleLengths[i]
                dist = scaleLengths[i]
            }
        }
        text = formatDistance(dist)
        scaleImage.width = (scaleImage.sourceSize.width * f) - 2 * scaleImageLeft.sourceSize.width
        scaleText.text = text
    }

    zoomLevel:                  18
    center:                     QGroundControl.lastKnownHomePosition
    gesture.flickDeceleration:  3000

    plugin: Plugin { name: "QGroundControl" }

    ExclusiveGroup { id: mapTypeGroup }

    Component.onCompleted: onMapTypeChanged

    property bool _initialMapPositionSet: false

    Connections {
        target: mainWindow
        onGcsPositionChanged: {
            if (!_initialMapPositionSet) {
                _initialMapPositionSet = true
                center = mainWindow.gcsPosition
            }
        }
    }

    onMapTypeChanged: {
        QGroundControl.flightMapSettings.setMapTypeForMapName(mapName, mapType)
        var fullMapName = QGroundControl.flightMapSettings.mapProvider + " " + mapType
        for (var i = 0; i < _map.supportedMapTypes.length; i++) {
            if (fullMapName === _map.supportedMapTypes[i].name) {
                _map.activeMapType = _map.supportedMapTypes[i]
                return
            }
        }
    }

    MapQuickItem {
        anchorPoint.x:  sourceItem.width  / 2
        anchorPoint.y:  sourceItem.height / 2
        visible:        mainWindow.gcsPosition.isValid
        coordinate:     mainWindow.gcsPosition
        sourceItem:     MissionItemIndexLabel {
            label: "Q"
        }
    }

    onWidthChanged: {
        if(_map.showScale)
            scaleTimer.restart()
    }

    onHeightChanged: {
        if(_map.showScale)
            scaleTimer.restart()
    }

    onZoomLevelChanged:{
        if(_map.showScale)
            scaleTimer.restart()
    }

    Timer {
        id:         scaleTimer
        interval:   100
        running:    false
        repeat:     false
        onTriggered: {
            _map.calculateScale()
        }
    }
    /*
        Scale
    */
    Item {
        id:                         scale
        visible:                    _map.showScale && scaleText.text !== "0 m"
        z:                          _map.z + 20
        width:                      scaleImageLeft.width + scaleImage.width + scaleImageRight.width
        anchors {
            bottom:                 parent.bottom
            bottomMargin:           ScreenTools.defaultFontPixelHeight * (0.66)
            right:                  parent.right
            rightMargin:            ScreenTools.defaultFontPixelHeight * (0.33)
        }
        Image {
            id:                     scaleImageLeft
            source:                 isSatelliteMap ? "/qmlimages/scale_end.png" : "/qmlimages/scale_endLight.png"
            anchors.bottom:         parent.bottom
            anchors.left:           parent.left
        }
        Image {
            id:                     scaleImage
            source:                 isSatelliteMap ? "/qmlimages/scale.png" : "/qmlimages/scaleLight.png"
            anchors.bottom:         parent.bottom
            anchors.left:           scaleImageLeft.right
        }
        Image {
            id:                     scaleImageRight
            source:                 isSatelliteMap ? "/qmlimages/scale_end.png" : "/qmlimages/scale_endLight.png"
            anchors.bottom:         parent.bottom
            anchors.left:           scaleImage.right
        }
        QGCLabel {
            id: scaleText
            color:                  isSatelliteMap ? "white" : "black"
            font.family:            ScreenTools.demiboldFontFamily
            horizontalAlignment:    Text.AlignHCenter
            anchors.bottom:         parent.bottom
            anchors.right:          parent.right
            anchors.bottomMargin:   ScreenTools.defaultFontPixelHeight * (0.83)
            text: "0 m"
        }
        Component.onCompleted: {
            if(_map.showScale)
                _map.calculateScale();
        }
    }

    /*********************************************
    /// Map control widgets
    Column {
        id:                 controlWidgets
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        spacing:            ScreenTools.defaultFontPixelWidth / 2
        z:                  1000    // Must be on top for clicking
        // Pinch zoom doesn't seem to be working, so zoom buttons in mobile on for now
        //visible:            !ScreenTools.isMobile

        Row {
            layoutDirection:    Qt.RightToLeft
            spacing:            ScreenTools.defaultFontPixelWidth / 2

            readonly property real _zoomIncrement: 1.0
            property real _buttonWidth: ScreenTools.defaultFontPixelWidth * 5

            NumberAnimation {
                id: animateZoom

                property real startZoom
                property real endZoom

                target:     _map
                properties: "zoomLevel"
                from:       startZoom
                to:         endZoom
                duration:   500

                easing {
                    type: Easing.OutExpo
                }
            }


            QGCButton {
                width:  parent._buttonWidth
                z:      QGroundControl.zOrderWidgets
                //iconSource: "/qmlimages/ZoomPlus.svg"
                text:   "+"

                onClicked: {
                    var endZoomLevel = _map.zoomLevel + parent._zoomIncrement
                    if (endZoomLevel > _map.maximumZoomLevel) {
                        endZoomLevel = _map.maximumZoomLevel
                    }
                    animateZoom.startZoom = _map.zoomLevel
                    animateZoom.endZoom = endZoomLevel
                    animateZoom.start()
                }
            }

            QGCButton {
                width:  parent._buttonWidth
                z:      QGroundControl.zOrderWidgets
                //iconSource: "/qmlimages/ZoomMinus.svg"
                text:   "-"

                onClicked: {
                    var endZoomLevel = _map.zoomLevel - parent._zoomIncrement
                    if (endZoomLevel < _map.minimumZoomLevel) {
                        endZoomLevel = _map.minimumZoomLevel
                    }
                    animateZoom.startZoom = _map.zoomLevel
                    animateZoom.endZoom = endZoomLevel
                    animateZoom.start()
                }
            }
        } // Row - +/- buttons
    } // Column - Map control widgets
*********************************************/

/*
 The slider and scale display are commented out for now to try to save real estate - DonLakeFlyer
 Not sure if I'll bring them back or not. Need room for waypoint list at bottom

    QGCSlider {
        id: zoomSlider;
        minimum: map.minimumZoomLevel;
        maximum: map.maximumZoomLevel;
        opacity: 1
        visible: parent.visible
        z: 1000
        anchors {
            bottom: parent.bottom;
            bottomMargin:   ScreenTools.defaultFontPixelHeight * (1.25)
            rightMargin:    ScreenTools.defaultFontPixelHeight * (1.66)
            leftMargin:     ScreenTools.defaultFontPixelHeight * (1.66)
            left: parent.left
        }
        width: parent.width - anchors.rightMargin - anchors.leftMargin
        value: map.zoomLevel
        Binding {
            target: zoomSlider; property: "value"; value: map.zoomLevel
        }
        onValueChanged: {
            map.zoomLevel = value
        }
    }
*/

} // Map
