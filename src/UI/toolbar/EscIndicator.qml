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

    property int    _onlineMotorCount:  0
    property bool   _escHealthy:        false

    function _isMotorOnline(motorIndex) {
        // Each ESC's info fact now contains its individual online status (1=online, 0=offline)
        return _escs && motorIndex < _escs.count && _escs.get(motorIndex).info.rawValue !== 0
    }

    function _recalcEscStatus() {
        // Recalculate online motor count
        let onlineCount = 0;
        if (_escs && _escs.count > 0) {
            for (let i = 0; i < _escs.count; i++) {
                if (_isMotorOnline(i)) {
                    onlineCount++;
                }
            }
        }
        _onlineMotorCount = onlineCount;

        // Recalculate health status
        let healthy = true;
        if (_onlineMotorCount !== _motorCount) {
            healthy = false;
        } else if (_escs) {
            for (let index = 0; index < _escs.count; index++) {
                if (_isMotorOnline(index)) {
                    if (_escs.get(index).failureFlags.rawValue > 0) {
                        healthy = false;
                        break;
                    }
                }
            }
        }
        _escHealthy = healthy;
    }

    function getEscStatusColor() {
        return _escHealthy ? qgcPal.colorGreen : qgcPal.colorRed
    }

    Component.onCompleted: _recalcEscStatus()

    Timer {
        id:         escDebounceTimer
        interval:   50
        running:    false
        repeat:     false
        onTriggered: control._recalcEscStatus()
    }

    Connections {
        target: _escs
        function onCountChanged() { escDebounceTimer.restart() }
    }

    // Connect to each ESC's info and failureFlags changes via an Instantiator
    Instantiator {
        model:  _escs
        active: _escs && _escs.count > 0

        delegate: QtObject {
            property var esc: model ? _escs.get(index) : null

            property var _infoConn: Connections {
                target: esc ? esc.info : null
                function onRawValueChanged() { escDebounceTimer.restart() }
            }

            property var _failureFlagsConn: Connections {
                target: esc ? esc.failureFlags : null
                function onRawValueChanged() { escDebounceTimer.restart() }
            }
        }
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
            color:              qgcPal.text
        }

        Column {
            id:                     escValuesColumn
            anchors.verticalCenter: parent.verticalCenter
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      qgcPal.text
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
