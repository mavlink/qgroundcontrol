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

    property bool showIndicator:       true

    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var  _escStatus:           _activeVehicle ? _activeVehicle.escStatus : null

    // ESC status properties derived from vehicle data
    property bool   _escDataAvailable:  _escStatus ? _escStatus.telemetryAvailable : false
    property int    _motorCount:        _escStatus ? _escStatus.count.rawValue : 0
    property int    _infoBitmask:       _escStatus ? _escStatus.info.rawValue : 0
    property int    _onlineMotorCount:  _getOnlineMotorCount()
    property bool   _escHealthy:        _escDataAvailable && _getEscHealthStatus()

    function _getOnlineMotorCount() {
        if (!_escDataAvailable) return 0
        var count = 0
        for (var i = 0; i < _motorCount && i < 8; i++) {
            if ((_infoBitmask & (1 << i)) !== 0) {
                count++
            }
        }
        return count
    }

    function _getEscHealthStatus() {
        if (!_escStatus || !_escDataAvailable) return false

        // Health is good if all expected motors are online and have no failure flags
        if (_onlineMotorCount !== _motorCount || _motorCount === 0) return false

        // Check failure flags for each motor
        for (var i = 0; i < _motorCount && i < 8; i++) {
            if ((_infoBitmask & (1 << i)) !== 0) { // Motor is online
                var failureFlags = _getMotorFailureFlags(i)
                if (failureFlags > 0) { // Any failure flag set means unhealthy
                    return false
                }
            }
        }

        return true
    }

    function _getMotorFailureFlags(motorIndex) {
        if (!_escStatus) return 0

        switch (motorIndex) {
        case 0: return _escStatus.failureFlagsFirst.rawValue
        case 1: return _escStatus.failureFlagsSecond.rawValue
        case 2: return _escStatus.failureFlagsThird.rawValue
        case 3: return _escStatus.failureFlagsFourth.rawValue
        case 4: return _escStatus.failureFlagsFifth.rawValue
        case 5: return _escStatus.failureFlagsSixth.rawValue
        case 6: return _escStatus.failureFlagsSeventh.rawValue
        case 7: return _escStatus.failureFlagsEighth.rawValue
        default: return 0
        }
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
            source:             "/qmlimages/MotorComponentIcon.svg"
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
                text:                       _onlineMotorCount.toString()
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
