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
import QGroundControl.ScreenTools

Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          escIndicatorRow.width

    property bool showIndicator: _escs.count > 0

    property var  _activeVehicle:   QGroundControl.multiVehicleManager.activeVehicle
    property var  _escs:            _activeVehicle ? _activeVehicle.escs : null

    // ESC status properties derived from vehicle data
    property int    _motorCount:        _escs && _escs.count > 0 ? _escs.get(0).count.rawValue : 0
    property int    _infoBitmask:       _escs && _escs.count > 0? _escs.get(0).info.rawValue : 0
    property int    _onlineMotorCount:  _getOnlineMotorCount()
    property bool   _escHealthy:        _getEscHealthStatus()

    function _getOnlineMotorCount() {
        if (_motorCount === 0) return 0

        let count = 0
        for (let i = 0; i < _motorCount && i < 8; i++) {
            if ((_infoBitmask & (1 << i)) !== 0) {
                count++
            }
        }
        return count
    }

    function _getEscHealthStatus() {
        if (_motorCount === 0) return false

        // Health is good if all expected motors are online and have no failure flags
        if (_onlineMotorCount !== _motorCount || _motorCount === 0) return false

        // Check failure flags for each motor
        for (let motorIndex = 0; motorIndex < _motorCount; motorIndex++) {
            if ((_infoBitmask & (1 << motorIndex)) !== 0) { // Motor is online
                if (_escs.get(motorIndex).failureFlags > 0) { // Any failure flag set means unhealthy
                    return false
                }
            }
        }

        return true
    }

    function getEscStatusColor() {
        return _escHealthy ? qgcPal.colorGreen : qgcPal.colorRed
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
            source:             "/qmlimages/EscIndicator.svg"
            fillMode:           Image.PreserveAspectFit
            sourceSize.height:  height
            color:              qgcPal.buttonText
        }

        Column {
            id:                     escValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      qgcPal.buttonText
                text:                       _onlineMotorCount.toString()
                font.pointSize:             ScreenTools.smallFontPointSize
            }

            QGCLabel {
                color:          getEscStatusColor()
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
