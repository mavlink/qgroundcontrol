/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactSystem

Rectangle {
    anchors.margins:    -ScreenTools.defaultFontPixelHeight
    height:             warningsCol.height
    width:              warningsCol.width
    color:              Qt.rgba(1, 1, 1, 0.5)
    radius:             ScreenTools.defaultFontPixelWidth / 2
    visible:            _noGPSLockVisible || _prearmErrorVisible || (_showAltitudeWarning && (_vehicleAltitudeBelowMin || _vehicleAltitudeAboveMax || !_terrainDataAvailable.value))

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property bool _noGPSLockVisible:    _activeVehicle && _activeVehicle.requiresGpsFix && !_activeVehicle.coordinate.isValid
    property bool _prearmErrorVisible:  _activeVehicle && !_activeVehicle.armed && _activeVehicle.prearmError

    property Fact _altitudeWarnThresholdEnabled: QGroundControl.settingsManager.flyViewSettings.altitudeWarnThresholdEnabled
    property Fact _altitudeWarnMinAGL: QGroundControl.settingsManager.flyViewSettings.altitudeWarnMinAGL
    property Fact _altitudeWarnMaxAGL: QGroundControl.settingsManager.flyViewSettings.altitudeWarnMaxAGL
    property Fact _altitudeAboveTerrain: _activeVehicle ? _activeVehicle.altitudeAboveTerr : null
    property Fact _terrainDataAvailable: _activeVehicle ? _activeVehicle.terrainDataAvailable : null
    property bool _vehicleAltitudeBelowMin: _altitudeAboveTerrain ? (_altitudeAboveTerrain.value < _altitudeWarnMinAGL.value) : false
    property bool _vehicleAltitudeAboveMax: _altitudeAboveTerrain ? (_altitudeAboveTerrain.value > _altitudeWarnMaxAGL.value) : false
    property bool _showAltitudeWarning: _activeVehicle && _activeVehicle.flying && !_activeVehicle.landing && _altitudeWarnThresholdEnabled.value

    Column {
        id:         warningsCol
        spacing:    ScreenTools.defaultFontPixelHeight

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _noGPSLockVisible
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("No GPS Lock for Vehicle")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _prearmErrorVisible
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       _activeVehicle ? _activeVehicle.prearmError : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _showAltitudeWarning && _terrainDataAvailable.value && _vehicleAltitudeBelowMin
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       visible ? qsTr("Altitude below minimum threshold of %1 %2 above terrain: (%3 %4)").arg(_altitudeWarnMinAGL.value).arg(_altitudeWarnMinAGL.units).arg(_altitudeAboveTerrain.value.toFixed(2)).arg(_altitudeAboveTerrain.units) : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _showAltitudeWarning && _terrainDataAvailable.value && _vehicleAltitudeAboveMax
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       visible ? qsTr("Altitude above maximum threshold of %1 %2 above terrain: (%3 %4)").arg(_altitudeWarnMaxAGL.value).arg(_altitudeWarnMaxAGL.units).arg(_altitudeAboveTerrain.value.toFixed(2)).arg(_altitudeAboveTerrain.units) : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _showAltitudeWarning && !_terrainDataAvailable.value
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("Terrain data not available at vehicle pos, altitude warning thresholds are disabled")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _prearmErrorVisible
            width:                      ScreenTools.defaultFontPixelWidth * 50
            horizontalAlignment:        Text.AlignHCenter
            wrapMode:                   Text.WordWrap
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("The vehicle has failed a pre-arm check. In order to arm the vehicle, resolve the failure.")
        }
    }
}
