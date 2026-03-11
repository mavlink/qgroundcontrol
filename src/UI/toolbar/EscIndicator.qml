import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

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

    property int    _onlineMotorCount:  _getOnlineMotorCount()
    property bool   _escHealthy:        _getEscHealthStatus()

    function _isMotorOnline(motorIndex) {
        // Each ESC's info fact now contains its individual online status (1=online, 0=offline)
        return _escs && motorIndex < _escs.count && _escs.get(motorIndex).info.rawValue !== 0
    }

    function _getOnlineMotorCount() {
        if (!_escs || _escs.count === 0) return 0;

        let count = 0;
        for (let i = 0; i < _escs.count; i++) {
            if (_isMotorOnline(i)) {
                count++;
            }
        }
        return count;
    }

    function _getEscHealthStatus() {
        // Health is good if all expected motors are online and have no failure flags
        if (_onlineMotorCount !== _motorCount) return false

        // Check failure flags for each motor
        for (let index = 0; index < _escs.count; index++) {
            if (_isMotorOnline(index)) { // Motor is online
                if (_escs.get(index).failureFlags.rawValue > 0) { // Any failure flag set means unhealthy
                    return false
                }
            }
        }

        return true
    }

    function getEscStatusColor() {
        return _escHealthy ? qgcPal.colorGreen : qgcPal.colorRed
    }

    QGCPalette { id: qgcPal }

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
            color:              qgcPal.windowTransparentText
        }

        Column {
            id:                     escValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      qgcPal.windowTransparentText
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
