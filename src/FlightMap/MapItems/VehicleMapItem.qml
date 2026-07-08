/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Effects
import QtQuick.Layouts
import QtLocation
import QtPositioning

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Vehicle
import QGroundControl.Controls
import QGroundControl.Palette

/// Marker for displaying a vehicle location on the map
MapQuickItem {
    id: _root

    property var    vehicle                                                         /// Vehicle object, undefined for ADSB vehicle
    property var    map
    property double altitude:       Number.NaN                                      ///< NAN to not show
    property string callsign:       ""                                              ///< Vehicle callsign
    property double heading:        vehicle ? vehicle.heading.value : Number.NaN    ///< Vehicle heading, NAN for none
    property real   size:           ScreenTools.defaultFontPixelHeight * 3          /// Default size for icon, most usage overrides this
    property bool   alert:          false                                           /// Collision alert

    anchorPoint.x:  vehicleItem.width  / 2
    anchorPoint.y:  vehicleItem.height / 2
    visible:        coordinate.isValid

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool   _adsbVehicle:   vehicle ? false : true
    property var    _map:           map
    property bool   _multiVehicle:  QGroundControl.multiVehicleManager.vehicles.count > 1
    property bool   _infoPinned:    false

    QGCPalette { id: qgcPal }

    function factText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined) {
            return fallbackText
        }
        return fact.valueString + (fact.units && fact.units !== "" ? " " + fact.units : "")
    }

    function firstBattery(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            return vehicle.batteries.get(0)
        }
        return null
    }

    function batteryInfoText(vehicle) {
        var battery = firstBattery(vehicle)
        if (!battery) {
            return qsTr("N/A")
        }

        var parts = []
        if (battery.voltage && !isNaN(battery.voltage.rawValue)) {
            parts.push(battery.voltage.valueString + (battery.voltage.units && battery.voltage.units !== "" ? " " + battery.voltage.units : ""))
        }
        if (battery.percentRemaining && !isNaN(battery.percentRemaining.rawValue)) {
            parts.push(battery.percentRemaining.valueString + battery.percentRemaining.units)
        }
        return parts.length > 0 ? parts.join("  ") : qsTr("N/A")
    }

    function gpsLockText(vehicle) {
        if (!vehicle || !vehicle.gps || !vehicle.gps.lock || isNaN(vehicle.gps.lock.rawValue)) {
            return qsTr("N/A")
        }

        switch (vehicle.gps.lock.rawValue) {
        case 2:
            return qsTr("2D")
        case 3:
            return qsTr("3D")
        case 4:
            return qsTr("DGPS")
        case 5:
            return qsTr("RTK Float")
        case 6:
            return qsTr("RTK Fixed")
        default:
            return qsTr("No Fix")
        }
    }

    function gpsInfoText(vehicle) {
        if (!vehicle || !vehicle.gps) {
            return qsTr("N/A")
        }

        var lockText = gpsLockText(vehicle)
        var satelliteText = vehicle.gps.count && !isNaN(vehicle.gps.count.rawValue) ? vehicle.gps.count.valueString : ""
        return satelliteText === "" ? lockText : lockText + " - " + satelliteText
    }

    function deviceTitleText(vehicle) {
        return vehicle ? qsTr("Vehicle %1").arg(vehicle.id) : qsTr("Unknown")
    }

    function vehicleStatusText(vehicle) {
        if (!vehicle) {
            return qsTr("Unknown")
        }
        if (vehicle.vehicleLinkManager && vehicle.vehicleLinkManager.communicationLost) {
            return qsTr("Communication Lost")
        }
        if (vehicle.landing) {
            return qsTr("Landing")
        }
        if (vehicle.flying) {
            return qsTr("Flying")
        }
        if (vehicle.armed) {
            return qsTr("Armed")
        }
        if (vehicle.readyToFlyAvailable && !vehicle.readyToFly) {
            return qsTr("Not Ready")
        }
        if (vehicle.readyToFlyAvailable && vehicle.readyToFly) {
            return qsTr("Ready")
        }
        return qsTr("Connected")
    }

    component VehicleInfoMetricRow: RowLayout {
        property string label
        property string value

        spacing: ScreenTools.defaultFontPixelWidth * 0.72

        QGCLabel {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 5.8
            text:                   label + ":"
            color:                  qgcPal.buttonText
            font.pointSize:         ScreenTools.captionFontPointSize
            elide:                  Text.ElideRight
        }

        QGCLabel {
            Layout.fillWidth:       true
            text:                   value
            color:                  qgcPal.text
            font.pointSize:         ScreenTools.captionFontPointSize
            font.bold:              true
            horizontalAlignment:    Text.AlignRight
            elide:                  Text.ElideRight
        }
    }

    sourceItem: Item {
        id:         vehicleItem
        width:      vehicleIcon.width
        height:     vehicleIcon.height
        opacity:    _adsbVehicle || vehicle === _activeVehicle ? 1.0 : 0.5

        property real _cardGap:       Math.max(6, ScreenTools.defaultFontPixelWidth * 0.72)
        property real _cardPaddingX:  ScreenTools.defaultFontPixelWidth * 0.82
        property real _cardPaddingY:  ScreenTools.defaultFontPixelHeight * 0.48
        property var  _mapCenter:     _root._map ? _root._map.center : QtPositioning.coordinate()
        property real _mapZoomLevel:  _root._map ? _root._map.zoomLevel : 0
        property real _screenX: {
            _mapCenter
            _mapZoomLevel
            if (!_root._map || _root._map.width <= 0 || !_root.coordinate.isValid) {
                return 0
            }
            return _root._map.fromCoordinate(_root.coordinate, false).x
        }
        property real _screenY: {
            _mapCenter
            _mapZoomLevel
            if (!_root._map || _root._map.height <= 0 || !_root.coordinate.isValid) {
                return 0
            }
            return _root._map.fromCoordinate(_root.coordinate, false).y
        }
        property bool _cardOnLeft:    _root._map && _root._map.width > 0 && _screenX > _root._map.width * 0.62

        MultiEffect {
            source: vehicleIcon
            shadowEnabled: vehicleIcon.visible && _adsbVehicle
            shadowColor: Qt.rgba(0.94,0.91,0,1.0)
            shadowVerticalOffset: 4
            shadowHorizontalOffset: 4
            shadowBlur: 1.0
            shadowOpacity: 0.5
            shadowScale: 1.3
            blurMax: 32
            blurMultiplier: .1
        }
            
        Repeater {
            model: vehicle ? vehicle.gimbalController.gimbals : [] 
            
            Item {
                id:                           canvasItem
                anchors.centerIn:             vehicleItem
                width:                        vehicleItem.width * 2
                height:                       vehicleItem.height * 2
                property var gimbalYaw:       object.absoluteYaw.rawValue
                rotation:                     gimbalYaw + 180
                onGimbalYawChanged:           canvas.requestPaint()
                visible:                      vehicle && !isNaN(gimbalYaw) && QGroundControl.settingsManager.gimbalControllerSettings.showAzimuthIndicatorOnMap.rawValue
                opacity:                      object === vehicle.gimbalController.activeGimbal ? 1.0 : 0.4

                Canvas {
                    id:                           canvas
                    anchors.centerIn:             canvasItem
                    anchors.verticalCenterOffset: vehicleItem.width
                    width:                        vehicleItem.width
                    height:                       vehicleItem.height

                    onPaint:                      paintHeading()

                    function paintHeading() {
                        var context = getContext("2d")
                        // console.log("painting heading " + object.param1Raw + " " + opacity + " " + visible + " " + _index)
                        context.clearRect(0, 0, vehicleIcon.width, vehicleIcon.height);

                        var centerX = canvas.width / 2;
                        var centerY = canvas.height / 2;
                        var length = canvas.height * 1.3
                        var width = canvas.width * 0.6

                        var point1 = [centerX - width , centerY + canvas.height * 0.6]
                        var point2 = [centerX, centerY - canvas.height * 0.5]
                        var point3 = [centerX + width , centerY + canvas.height * 0.6]
                        var point4 = [centerX, centerY + canvas.height * 0.2]

                        // Draw the arrow
                        context.save();
                        context.globalAlpha = 0.9;
                        context.beginPath();
                        context.moveTo(centerX, centerY + canvas.height * 0.2);
                        context.lineTo(point1[0], point1[1]);
                        context.lineTo(point2[0], point2[1]);
                        context.lineTo(point3[0], point3[1]);
                        context.lineTo(point4[0], point4[1]);
                        context.closePath();

                        const gradient = context.createLinearGradient(canvas.width / 2, canvas.height , canvas.width / 2, 0);
                        gradient.addColorStop(0.3, Qt.rgba(255,255,255,0));
                        gradient.addColorStop(0.5, Qt.rgba(255,255,255,0.5));
                        gradient.addColorStop(1, qgcPal.mapIndicator);

                        context.fillStyle = gradient;
                        context.fill();
                        context.restore();
                    }
                }
            }
        }

        Image {
            id:                 vehicleIcon
            source:             _adsbVehicle ? (alert ? "/qmlimages/AlertAircraft.svg" : "/qmlimages/AwarenessAircraft.svg") : vehicle.vehicleImageOpaque
            mipmap:             true
            width:              _root.size
            sourceSize.width:   _root.size
            fillMode:           Image.PreserveAspectFit
            transform: Rotation {
                origin.x:       vehicleIcon.width  / 2
                origin.y:       vehicleIcon.height / 2
                angle:          isNaN(heading) ? 0 : heading
            }
        }

        Rectangle {
            id:                 infoHoverBadge
            z:                  18
            anchors.right:      vehicleIcon.right
            anchors.top:        vehicleIcon.top
            anchors.rightMargin: -width * 0.18
            anchors.topMargin:  -height * 0.18
            width:              Math.max(14, ScreenTools.defaultFontPixelHeight * 0.86)
            height:             width
            radius:             width / 2
            visible:            opacity > 0
            opacity:            !_adsbVehicle && vehicle && infoButton.containsMouse && !ScreenTools.isMobile ? 1.0 : 0.0
            color:              Qt.rgba(0.045, 0.048, 0.052, 0.82)
            border.color:       Qt.rgba(0.82, 0.90, 0.95, 0.42)
            border.width:       1

            Behavior on opacity {
                NumberAnimation { duration: 90 }
            }

            QGCLabel {
                anchors.centerIn:   parent
                text:               "i"
                color:              "white"
                font.bold:          true
                font.pointSize:     ScreenTools.captionFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:   Text.AlignVCenter
            }
        }

        Rectangle {
            id:         vehicleInfoCard
            parent:     _root._map && _root._map.parent ? _root._map.parent : vehicleItem
            z:          10
            visible:    !_adsbVehicle && vehicle && _root._infoPinned
            width:      Math.max(ScreenTools.defaultFontPixelWidth * 15.4, infoLayout.implicitWidth + vehicleItem._cardPaddingX * 2)
            height:     infoLayout.implicitHeight + vehicleItem._cardPaddingY * 2
            x:          (_root._map ? _root._map.x : 0) + vehicleItem._screenX - _root.anchorPoint.x +
                            (vehicleItem._cardOnLeft ? -width - vehicleItem._cardGap : vehicleIcon.width + vehicleItem._cardGap)
            y:          (_root._map ? _root._map.y : 0) + vehicleItem._screenY - _root.anchorPoint.y +
                            Math.round((vehicleIcon.height - height) / 2)
            radius:     Math.round(ScreenTools.defaultFontPixelWidth * 0.58)
            color:      "transparent"
            border.color: Qt.rgba(0.82, 0.90, 0.95, _root._infoPinned ? 0.22 : 0.14)
            border.width: 1
            clip:       true

            GlassBackdrop {
                anchors.fill:       parent
                sourceItem:         _root._map
                backdropBlurEnabled:true
                targetItem:         vehicleInfoCard
                sampleAtItemPosition: false
                sampleX:            vehicleInfoCard.x - (_root._map ? _root._map.x : 0)
                sampleY:            vehicleInfoCard.y - (_root._map ? _root._map.y : 0)
                cornerRadius:       vehicleInfoCard.radius
                sourceScale:        0.46
                blurAmount:         0.94
                blurMax:            42
                sourceBrightness:   -0.01
                sourceSaturation:   0.62
                tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
                sheenColor:         "transparent"
            }

            ColumnLayout {
                id:                 infoLayout
                anchors.fill:       parent
                anchors.margins:    vehicleItem._cardPaddingX
                spacing:            Math.max(2, ScreenTools.defaultFontPixelHeight * 0.14)

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth * 0.55

                    QGCLabel {
                        Layout.fillWidth:       true
                        text:                   _root.deviceTitleText(vehicle)
                        color:                  qgcPal.text
                        font.bold:              true
                        font.pointSize:         ScreenTools.labelFontPointSize
                        elide:                  Text.ElideRight
                    }

                    QGCLabel {
                        id:                     statusLabel
                        Layout.alignment:       Qt.AlignVCenter
                        Layout.preferredWidth:  Math.min(implicitWidth, ScreenTools.defaultFontPixelWidth * 8)
                        text:                   _root.vehicleStatusText(vehicle)
                        color:                  qgcPal.text
                        opacity:                0.82
                        font.bold:              true
                        font.pointSize:         ScreenTools.captionFontPointSize
                        horizontalAlignment:    Text.AlignRight
                        elide:                  Text.ElideRight
                    }
                }

                Rectangle {
                    Layout.fillWidth:       true
                    Layout.preferredHeight: 1
                    color:                  Qt.rgba(0.82, 0.90, 0.95, 0.12)
                }

                VehicleInfoMetricRow {
                    Layout.fillWidth:   true
                    label:              qsTr("Ground Speed")
                    value:              _root.factText(vehicle ? vehicle.groundSpeed : null, qsTr("N/A"))
                }

                VehicleInfoMetricRow {
                    Layout.fillWidth:   true
                    label:              qsTr("Air Speed")
                    value:              _root.factText(vehicle ? vehicle.airSpeed : null, qsTr("N/A"))
                }

                VehicleInfoMetricRow {
                    Layout.fillWidth:   true
                    label:              qsTr("Altitude")
                    value:              _root.factText(vehicle ? vehicle.altitudeRelative : null, qsTr("N/A"))
                }

                VehicleInfoMetricRow {
                    Layout.fillWidth:   true
                    label:              qsTr("Battery")
                    value:              _root.batteryInfoText(vehicle)
                }

                VehicleInfoMetricRow {
                    Layout.fillWidth:   true
                    label:              qsTr("GPS")
                    value:              _root.gpsInfoText(vehicle)
                }
            }

        }

        QGCMouseArea {
            id:             infoButton
            z:              20
            fillItem:       vehicleIcon
            enabled:        !_adsbVehicle && !!vehicle
            hoverEnabled:   enabled && !ScreenTools.isMobile
            cursorShape:    Qt.PointingHandCursor
            onClicked: {
                if (vehicle && QGroundControl.multiVehicleManager.activeVehicle !== vehicle) {
                    QGroundControl.multiVehicleManager.activeVehicle = vehicle
                }
                _root._infoPinned = !_root._infoPinned
            }
        }

        QGCMapLabel {
            id:                         vehicleLabel
            anchors.top:                parent.bottom
            anchors.horizontalCenter:   parent.horizontalCenter
            map:                        _map
            text:                       vehicleLabelText
            font.pointSize:             _adsbVehicle ? ScreenTools.defaultFontPointSize : ScreenTools.smallFontPointSize
            visible:                    _adsbVehicle ? !isNaN(altitude) : _multiVehicle
            property string vehicleLabelText: visible ?
                                                  (_adsbVehicle ?
                                                       QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(altitude).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString + "\n" + callsign :
                                                       (_multiVehicle ? qsTr("Vehicle %1").arg(vehicle.id) : "")) :
                                                  ""

        }
    }
}
