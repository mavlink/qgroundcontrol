/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import MAVLink

//-------------------------------------------------------------------------
//-- ESC Indicator
Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          escIndicatorRow.width

    property bool showIndicator: true
    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var  _escStatus:           _activeVehicle ? _activeVehicle.escStatus : null

    // ESC status properties derived from vehicle data
    property bool   _escDataAvailable:  _escStatus ? _escStatus.telemetryAvailable : false
    property int    _escCount:          4      // Assuming 4 ESCs for now
    property bool   _escHealthy:        _escDataAvailable && _getEscHealthStatus()
    property real   _maxEscRpm:         _escDataAvailable ? Math.max(_escStatus.rpmFirst.rawValue, _escStatus.rpmSecond.rawValue, _escStatus.rpmThird.rawValue, _escStatus.rpmFourth.rawValue) : 0

    function _getEscHealthStatus() {
        if (!_escStatus || !_escDataAvailable) return false
        // Consider ESCs healthy if they have reasonable RPM values when armed
        var anyRpm = _escStatus.rpmFirst.rawValue > 0 || _escStatus.rpmSecond.rawValue > 0 || 
                     _escStatus.rpmThird.rawValue > 0 || _escStatus.rpmFourth.rawValue > 0
        return _activeVehicle.armed ? anyRpm : true
    }

    function getEscStatusColor() {
        if (!_activeVehicle || !_escDataAvailable) {
            return qgcPal.text
        }
        
        if (_escHealthy) {
            return qgcPal.colorGreen
        } else {
            return qgcPal.colorRed
        }
    }

    Row {
        id:             escIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        QGCColoredImage {
            id:                 escIcon
            width:              height
            anchors.top:        parent.top
            anchors.bottom:     parent.bottom
            source:             "/qmlimages/EscMotor.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            opacity:            (_activeVehicle && _escDataAvailable) ? 1 : 0.5
            color:              getEscStatusColor()
        }

        Column {
            id:                     escValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            visible:                _activeVehicle && _escDataAvailable
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      qgcPal.buttonText
                text:                       _escCount.toString()
                font.pointSize:            ScreenTools.smallFontPointSize
            }

            QGCLabel {
                color:          qgcPal.buttonText
                text:           _escHealthy ? qsTr("OK") : qsTr("ERR")
                font.pointSize: ScreenTools.smallFontPointSize
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(escIndicatorPage, control)
    }

    Component {
        id: escIndicatorPage

        EscIndicatorPage { }
    }
}