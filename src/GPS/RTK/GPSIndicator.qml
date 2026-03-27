import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

Item {
    id:             control
    width:          gpsIndicatorRow.width
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _rtkConnected:      QGroundControl.gpsManager ? QGroundControl.gpsManager.deviceCount > 0 : false
    property int    _lockType:          _activeVehicle ? _activeVehicle.gps.lock.rawValue : 0
    property bool   _ntripActive:       QGroundControl.ntripManager.connectionStatus === NTRIPManager.Connected
    property bool   _correctionsActive: _ntripActive || _rtkConnected
    property bool   _vehicleHasRtk:     _lockType >= VehicleGPSFactGroup.FixRTKFloat

    // Hidden dot used solely to resolve lockValue → palette color
    FixStatusDot {
        id: _fixDot
        visible: false
        lockValue: _activeVehicle ? _lockType : -1
    }

    property color _fixColor: {
        if (!_activeVehicle)
            return _correctionsActive ? qgcPal.colorGrey : qgcPal.text
        return _fixDot.statusColor
    }

    property var    _gpsAggregate:  _activeVehicle ? _activeVehicle.gpsAggregate : null
    property bool   _hasInterference: {
        if (!_gpsAggregate) return false
        var jam  = _gpsAggregate.jammingState.value
        var spf  = _gpsAggregate.spoofingState.value
        return (jam >= VehicleGPSFactGroup.JammingMitigated && jam !== VehicleGPSFactGroup.JammingInvalid)
            || (spf >= VehicleGPSFactGroup.SpoofingMitigated && spf !== VehicleGPSFactGroup.SpoofingInvalid)
    }

    QGCPalette { id: qgcPal }

    property string _fixLabel: {
        if (!_activeVehicle) return ""
        if (_lockType >= VehicleGPSFactGroup.FixRTKFixed) return qsTr("RTK Fixed")
        if (_lockType >= VehicleGPSFactGroup.FixRTKFloat) return qsTr("RTK Float")
        if (_lockType >= VehicleGPSFactGroup.Fix3D)       return qsTr("3D")
        if (_lockType >= VehicleGPSFactGroup.Fix2D)       return qsTr("2D")
        return ""
    }

    Row {
        id:             gpsIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Row {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        -ScreenTools.defaultFontPixelWidth / 2

            QGCLabel {
                rotation:               90
                text:                   qsTr("RTK")
                color:                  qgcPal.text
                anchors.verticalCenter: parent.verticalCenter
                visible:                _rtkConnected
            }

            Item {
                width:              gpsIcon.width
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom

                QGCColoredImage {
                    id:                 gpsIcon
                    width:              height
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    source:             "qrc:/qml/QGroundControl/GPS/RTK/Images/Gps.svg"
                    fillMode:           Image.PreserveAspectFit
                    sourceSize.height:  height
                    opacity:            (_activeVehicle && _activeVehicle.gps.count.value > 0) ? 1 : 0.5
                    color: control._fixColor
                }

                // Corrections active dot (bottom-right)
                Rectangle {
                    width:          ScreenTools.defaultFontPixelHeight * 0.45
                    height:         width
                    radius:         width / 2
                    color:          _vehicleHasRtk ? qgcPal.colorGreen : qgcPal.colorOrange
                    border.color:   qgcPal.window
                    border.width:   1
                    anchors.right:  parent.right
                    anchors.bottom: parent.bottom
                    anchors.rightMargin:  -width * 0.15
                    anchors.bottomMargin: parent.height * 0.1
                    visible:        _correctionsActive
                }

                // Interference warning badge (top-right)
                Rectangle {
                    width:          ScreenTools.defaultFontPixelHeight * 0.5
                    height:         width
                    radius:         width / 2
                    color:          qgcPal.colorRed
                    border.color:   qgcPal.window
                    border.width:   1
                    anchors.right:  parent.right
                    anchors.top:    parent.top
                    anchors.rightMargin: -width * 0.15
                    anchors.topMargin:   parent.height * 0.1
                    visible:        _hasInterference

                    QGCLabel {
                        anchors.centerIn: parent
                        text:             "!"
                        color:            qgcPal.window
                        font.bold:        true
                        font.pixelSize:   parent.height * 0.7
                    }
                }
            }
        }

        Column {
            anchors.verticalCenter: parent.verticalCenter
            visible:                _activeVehicle && _activeVehicle.gps.count.value > 0
            spacing:                0

            QGCLabel {
                anchors.horizontalCenter: hdopValue.horizontalCenter
                color: qgcPal.text
                text:  _activeVehicle ? _activeVehicle.gps.count.valueString : ""
            }

            QGCLabel {
                id:    hdopValue
                color: qgcPal.text
                text:  _activeVehicle && !isNaN(_activeVehicle.gps.hdop.value) ? _activeVehicle.gps.hdop.value.toFixed(1) : ""
            }
        }

        QGCLabel {
            anchors.verticalCenter: parent.verticalCenter
            visible:                _fixLabel !== ""
            text:                   _fixLabel
            font.pointSize:         ScreenTools.smallFontPointSize
            font.bold:              true
            color: control._fixColor
        }
    }

    QGCMouseArea {
        fillItem:   parent
        onClicked:  mainWindow.showIndicatorDrawer(gpsIndicatorPage, control)
    }

    Component {
        id: gpsIndicatorPage
        GPSIndicatorPage { }
    }
}
