import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.NTRIP
import QGroundControl.GPS.RTK

ToolIndicatorPage {
    showExpand: true

    property var    activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle
    property var    _gpsRtk:        QGroundControl.gpsRtk
    property var    _ntripMgr:      QGroundControl.ntripManager
    property var    _rtcmMavlink:   QGroundControl.gpsManager ? QGroundControl.gpsManager.rtcmMavlink : null
    property var    _gpsAggregate:  activeVehicle ? activeVehicle.gpsAggregate : null
    property string na:             qsTr("N/A", "No data to display")
    property string valueNA:        qsTr("-.--", "No data to display")

    property bool   _correctionsActive: {
        var ntripOn = _ntripMgr && _ntripMgr.connectionStatus === NTRIPManager.Connected
        var rtkOn   = QGroundControl.gpsManager && QGroundControl.gpsManager.deviceCount > 0
        return ntripOn || rtkOn
    }
    property int    _vehicleLock:       activeVehicle ? activeVehicle.gps.lock.rawValue : 0
    property bool   _vehicleHasRtk:     _vehicleLock >= VehicleGPSFactGroup.FixRTKFloat
    property int    _correctionsSentSec: 0
    property var    _posMgr:            QGroundControl.qgcPositionManger
    property var    _autoConnectSettings: QGroundControl.settingsManager.autoConnectSettings
    property bool   _gcsNmeaActive:     _autoConnectSettings.autoConnectNmeaPort.valueString !== ""
                                        && _autoConnectSettings.autoConnectNmeaPort.valueString !== "Disabled"

    function gpsErrorText(errorVal) {
        var errors = []
        if (errorVal & 1)  errors.push(qsTr("Incoming correction"))
        if (errorVal & 2)  errors.push(qsTr("Configuration"))
        if (errorVal & 4)  errors.push(qsTr("Software"))
        if (errorVal & 8)  errors.push(qsTr("Antenna"))
        if (errorVal & 16) errors.push(qsTr("Event congestion"))
        if (errorVal & 32) errors.push(qsTr("CPU overload"))
        if (errorVal & 64) errors.push(qsTr("Output congestion"))
        return errors.length > 0 ? errors.join(", ") : ""
    }

    function formatDuration(secs) {
        if (secs < 60) return secs + "s"
        if (secs < 3600) return Math.floor(secs / 60) + "m " + (secs % 60) + "s"
        return Math.floor(secs / 3600) + "h " + Math.floor((secs % 3600) / 60) + "m"
    }

    function formatDataSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / 1048576).toFixed(1) + " MB"
    }

    function _fixTypeColor(lockVal) {
        if (lockVal >= VehicleGPSFactGroup.FixRTKFixed)  return qgcPal.colorGreen
        if (lockVal >= VehicleGPSFactGroup.FixRTKFloat)  return qgcPal.colorOrange
        if (lockVal >= VehicleGPSFactGroup.Fix3D)        return qgcPal.colorGreen
        if (lockVal >= VehicleGPSFactGroup.Fix2D)        return qgcPal.colorOrange
        if (lockVal >= VehicleGPSFactGroup.FixNoFix)     return qgcPal.colorRed
        return qgcPal.colorGrey
    }

    function _hasResilienceData(gps) {
        if (!gps) return false
        var jam  = gps.jammingState.value
        var spf  = gps.spoofingState.value
        var auth = gps.authenticationState.value
        return (jam  > VehicleGPSFactGroup.JammingUnknown  && jam  !== VehicleGPSFactGroup.JammingInvalid)
            || (spf  > VehicleGPSFactGroup.SpoofingUnknown && spf  !== VehicleGPSFactGroup.SpoofingInvalid)
            || (auth > VehicleGPSFactGroup.AuthUnknown     && auth !== VehicleGPSFactGroup.AuthInvalid)
    }

    function _isJammingVisible(gps) {
        if (!gps) return false
        var v = gps.jammingState.value
        return v > VehicleGPSFactGroup.JammingUnknown && v !== VehicleGPSFactGroup.JammingInvalid
    }

    function _isSpoofingVisible(gps) {
        if (!gps) return false
        var v = gps.spoofingState.value
        return v > VehicleGPSFactGroup.SpoofingUnknown && v !== VehicleGPSFactGroup.SpoofingInvalid
    }

    function _isAuthVisible(gps) {
        if (!gps) return false
        var v = gps.authenticationState.value
        return v > VehicleGPSFactGroup.AuthUnknown && v !== VehicleGPSFactGroup.AuthInvalid
    }

    function _formatAccuracy(rawVal) {
        if (isNaN(rawVal)) return valueNA
        if (rawVal < 1.0) return (rawVal * 100).toFixed(1) + " cm"
        return rawVal.toFixed(3) + " m"
    }

    // ====================================================================
    // Content: essential status at a glance
    // ====================================================================
    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // -- Vehicle GPS Status ------------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("Vehicle GPS")
                showDividers: true
                visible:      activeVehicle

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight * 0.6
                        height: width
                        radius: width / 2
                        color:  activeVehicle ? _fixTypeColor(activeVehicle.gps.lock.rawValue) : qgcPal.colorGrey
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCLabel {
                        text: activeVehicle ? activeVehicle.gps.lock.enumStringValue : na
                    }
                }

                LabelledLabel {
                    label:     qsTr("Satellites")
                    labelText: activeVehicle ? activeVehicle.gps.count.valueString : na
                }

                LabelledLabel {
                    label:     qsTr("HDOP / VDOP")
                    labelText: {
                        if (!activeVehicle) return valueNA
                        var h = isNaN(activeVehicle.gps.hdop.value) ? valueNA : activeVehicle.gps.hdop.value.toFixed(1)
                        var v = isNaN(activeVehicle.gps.vdop.value) ? valueNA : activeVehicle.gps.vdop.value.toFixed(1)
                        return h + " / " + v
                    }
                }

                LabelledLabel {
                    label:     qsTr("Error")
                    labelText: activeVehicle ? gpsErrorText(activeVehicle.gps.systemErrors.value) : ""
                    visible:   activeVehicle && activeVehicle.gps.systemErrors.value > 0
                }
            }

            // -- RTK Corrections Status --------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("RTK Corrections")
                showDividers: true
                visible:      activeVehicle && _correctionsActive

                Timer {
                    id:       correctionTimer
                    interval: 1000
                    running:  _correctionsActive && !_vehicleHasRtk
                    repeat:   true
                    onTriggered: _correctionsSentSec++
                    onRunningChanged: if (!running) _correctionsSentSec = 0
                }

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight * 0.6
                        height: width
                        radius: width / 2
                        color:  {
                            if (_vehicleLock >= VehicleGPSFactGroup.FixRTKFixed)  return qgcPal.colorGreen
                            if (_vehicleLock >= VehicleGPSFactGroup.FixRTKFloat)  return qgcPal.colorOrange
                            if (_correctionsActive && _correctionsSentSec > 30)   return qgcPal.colorRed
                            if (_correctionsActive)                               return qgcPal.colorOrange
                            return qgcPal.colorGrey
                        }
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCLabel {
                        text: {
                            if (_vehicleLock >= VehicleGPSFactGroup.FixRTKFixed)  return qsTr("RTK Fixed")
                            if (_vehicleLock >= VehicleGPSFactGroup.FixRTKFloat)  return qsTr("RTK Float")
                            if (_correctionsActive && _correctionsSentSec > 30)   return qsTr("No RTK Fix")
                            if (_correctionsActive)                               return qsTr("Waiting for RTK...")
                            return qsTr("No Corrections")
                        }
                    }
                }

                LabelledLabel {
                    label:     qsTr("Source")
                    labelText: {
                        var sources = []
                        if (_ntripMgr && _ntripMgr.connectionStatus === NTRIPManager.Connected)
                            sources.push(qsTr("NTRIP"))
                        if (QGroundControl.gpsManager && QGroundControl.gpsManager.deviceCount > 0)
                            sources.push(qsTr("RTK Base"))
                        return sources.join(" + ")
                    }
                }

                QGCLabel {
                    text:           qsTr("Corrections are being sent but vehicle has not achieved RTK fix. Check base station accuracy, satellite count, and signal quality.")
                    wrapMode:       Text.WordWrap
                    color:          qgcPal.colorOrange
                    font.pointSize: ScreenTools.smallFontPointSize
                    visible:        _correctionsActive && !_vehicleHasRtk && _correctionsSentSec > 30
                    Layout.fillWidth: true
                }
            }

            // -- Hints -------------------------------------------------------
            QGCLabel {
                Layout.fillWidth: true
                visible:          activeVehicle && !_correctionsActive && !_vehicleHasRtk
                text:             qsTr("Connect an RTK base station or NTRIP caster for centimeter-level accuracy.")
                wrapMode:         Text.WordWrap
                font.pointSize:   ScreenTools.smallFontPointSize
                color:            qgcPal.colorGrey
            }

            // -- No vehicle placeholder --------------------------------------
            SettingsGroupLayout {
                heading: qsTr("Vehicle GPS")
                visible: !activeVehicle

                QGCLabel {
                    text:  qsTr("No vehicle connected")
                    color: qgcPal.colorGrey
                }
            }
        }
    }

    // ====================================================================
    // Expanded: detailed diagnostics
    // ====================================================================
    expandedComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // -- Vehicle GPS Details -----------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("Vehicle GPS Details")
                showDividers: true
                visible:      activeVehicle

                LabelledLabel {
                    label:     qsTr("Altitude MSL")
                    labelText: activeVehicle ? activeVehicle.gps.altitudeMSL.valueString : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.altitudeMSL.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Ground Speed")
                    labelText: activeVehicle ? activeVehicle.gps.groundSpeed.valueString : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.groundSpeed.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("H Acc / V Acc")
                    labelText: {
                        if (!activeVehicle) return valueNA
                        return _formatAccuracy(activeVehicle.gps.hAcc.rawValue)
                               + " / " + _formatAccuracy(activeVehicle.gps.vAcc.rawValue)
                    }
                    visible:   activeVehicle && (!isNaN(activeVehicle.gps.hAcc.rawValue) || !isNaN(activeVehicle.gps.vAcc.rawValue))
                }

                LabelledLabel {
                    label:     qsTr("Speed Accuracy")
                    labelText: activeVehicle ? activeVehicle.gps.velAcc.valueString : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.velAcc.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Heading Accuracy")
                    labelText: activeVehicle ? activeVehicle.gps.hdgAcc.valueString : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.hdgAcc.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Course Over Ground")
                    labelText: activeVehicle ? activeVehicle.gps.courseOverGround.valueString : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.courseOverGround.rawValue)
                }
            }

            // -- RTCM Bandwidth ----------------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("RTCM Data")
                showDividers: true
                visible:      _rtcmMavlink && _rtcmMavlink.totalBytesSent > 0

                LabelledLabel {
                    label:     qsTr("Bandwidth")
                    labelText: _rtcmMavlink ? (_rtcmMavlink.bandwidthKBps.toFixed(1) + " KB/s") : valueNA
                }

                LabelledLabel {
                    label:     qsTr("Total Sent")
                    labelText: _rtcmMavlink ? formatDataSize(_rtcmMavlink.totalBytesSent) : valueNA
                }
            }

            // -- Vehicle GPS 2 -----------------------------------------------
            SettingsGroupLayout {
                id:           gps2Section
                heading:      qsTr("Vehicle GPS 2")
                showDividers: true
                visible:      activeVehicle && activeVehicle.gps2

                property var _gps2: activeVehicle ? activeVehicle.gps2 : null

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight * 0.6
                        height: width
                        radius: width / 2
                        color:  gps2Section._gps2 ? _fixTypeColor(gps2Section._gps2.lock.rawValue) : qgcPal.colorGrey
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCLabel {
                        text: gps2Section._gps2 ? gps2Section._gps2.lock.enumStringValue : na
                    }
                }

                LabelledLabel {
                    label:     qsTr("Satellites")
                    labelText: gps2Section._gps2 ? gps2Section._gps2.count.valueString : na
                }

                LabelledLabel {
                    label:     qsTr("HDOP / VDOP")
                    labelText: {
                        if (!gps2Section._gps2) return valueNA
                        var h = isNaN(gps2Section._gps2.hdop.value) ? valueNA : gps2Section._gps2.hdop.value.toFixed(1)
                        var v = isNaN(gps2Section._gps2.vdop.value) ? valueNA : gps2Section._gps2.vdop.value.toFixed(1)
                        return h + " / " + v
                    }
                }

                LabelledLabel {
                    label:     qsTr("Altitude MSL")
                    labelText: gps2Section._gps2 ? gps2Section._gps2.altitudeMSL.valueString : valueNA
                    visible:   gps2Section._gps2 && !isNaN(gps2Section._gps2.altitudeMSL.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Ground Speed")
                    labelText: gps2Section._gps2 ? gps2Section._gps2.groundSpeed.valueString : valueNA
                    visible:   gps2Section._gps2 && !isNaN(gps2Section._gps2.groundSpeed.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("H Acc / V Acc")
                    labelText: {
                        if (!gps2Section._gps2) return valueNA
                        return _formatAccuracy(gps2Section._gps2.hAcc.rawValue)
                               + " / " + _formatAccuracy(gps2Section._gps2.vAcc.rawValue)
                    }
                    visible:   gps2Section._gps2 && (!isNaN(gps2Section._gps2.hAcc.rawValue) || !isNaN(gps2Section._gps2.vAcc.rawValue))
                }

                LabelledLabel {
                    label:     qsTr("Course Over Ground")
                    labelText: gps2Section._gps2 ? gps2Section._gps2.courseOverGround.valueString : valueNA
                    visible:   gps2Section._gps2 && !isNaN(gps2Section._gps2.courseOverGround.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Error")
                    labelText: gps2Section._gps2 ? gpsErrorText(gps2Section._gps2.systemErrors.value) : ""
                    visible:   gps2Section._gps2 && gps2Section._gps2.systemErrors.value > 0
                }
            }

            // -- RTK Base Station Status (per-device) ------------------------
            Repeater {
                model: QGroundControl.gpsManager ? QGroundControl.gpsManager.devices : null

                RTKBaseStationStatus {
                    required property var modelData
                    device: modelData
                }
            }

            // -- NTRIP Status ------------------------------------------------
            NTRIPConnectionStatus {
                rtcmMavlink: _rtcmMavlink
            }

            // -- Ground Station Position -------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("Ground Station")
                showDividers: true
                visible:      _posMgr && (_posMgr.gcsPosition.isValid || !!_gcsNmeaActive)

                property bool _posValid: !!_posMgr && _posMgr.gcsPosition.isValid

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth
                    visible: !parent._posValid && !!_gcsNmeaActive

                    Rectangle {
                        width:  ScreenTools.defaultFontPixelHeight * 0.6
                        height: width
                        radius: width / 2
                        color:  qgcPal.colorOrange
                        Layout.alignment: Qt.AlignVCenter
                    }

                    QGCLabel {
                        text: qsTr("Waiting for position data...")
                    }
                }

                LabelledLabel {
                    label:     qsTr("Position")
                    labelText: {
                        var mgr = _posMgr
                        if (!mgr || !mgr.gcsPosition.isValid) return valueNA
                        return mgr.gcsPosition.latitude.toFixed(7) + ", " + mgr.gcsPosition.longitude.toFixed(7)
                    }
                    visible: !!parent._posValid
                }

                LabelledLabel {
                    label:     qsTr("Accuracy")
                    labelText: {
                        var mgr = _posMgr
                        if (!mgr || !isFinite(mgr.gcsPositionHorizontalAccuracy)) return valueNA
                        return _formatAccuracy(mgr.gcsPositionHorizontalAccuracy)
                    }
                    visible: !!parent._posValid && !!_posMgr && isFinite(_posMgr.gcsPositionHorizontalAccuracy)
                }
            }

            // -- GPS Event Log -----------------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("Event Log")
                showDividers: true
                visible:      QGroundControl.gpsEventModel && QGroundControl.gpsEventModel.count > 0

                QGCListView {
                    Layout.fillWidth:       true
                    Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 12)
                    clip:                   true
                    model:                  QGroundControl.gpsEventModel
                    spacing:                ScreenTools.defaultFontPixelHeight * 0.15

                    delegate: RowLayout {
                        width:   ListView.view.width
                        spacing: ScreenTools.defaultFontPixelWidth

                        Rectangle {
                            width:  ScreenTools.defaultFontPixelHeight * 0.5
                            height: width
                            radius: width / 2
                            color:  model.severity === "Error" ? qgcPal.colorRed
                                    : model.severity === "Warning" ? qgcPal.colorOrange
                                    : qgcPal.colorGreen
                            Layout.alignment: Qt.AlignVCenter
                        }

                        QGCLabel {
                            text:           "[%1] %2".arg(model.source).arg(model.message)
                            font.pointSize: ScreenTools.smallFontPointSize
                            elide:          Text.ElideRight
                            Layout.fillWidth: true
                        }
                    }
                }
            }

            // -- Signal Integrity --------------------------------------------
            Repeater {
                model: {
                    var list = []
                    if (activeVehicle && _hasResilienceData(activeVehicle.gps))
                        list.push({ label: qsTr("GPS 1"), gps: activeVehicle.gps })
                    if (activeVehicle && activeVehicle.gps2 && _hasResilienceData(activeVehicle.gps2))
                        list.push({ label: qsTr("GPS 2"), gps: activeVehicle.gps2 })
                    return list
                }

                SettingsGroupLayout {
                    required property var modelData
                    property var _gps: modelData.gps

                    heading:      qsTr("%1 Signal Integrity").arg(modelData.label)
                    showDividers: true

                    LabelledLabel {
                        label:     qsTr("Jamming")
                        labelText: _gps ? (_gps.jammingState.enumStringValue || na) : na
                        visible:   _isJammingVisible(_gps)
                    }
                    LabelledLabel {
                        label:     qsTr("Spoofing")
                        labelText: _gps ? (_gps.spoofingState.enumStringValue || na) : na
                        visible:   _isSpoofingVisible(_gps)
                    }
                    LabelledLabel {
                        label:     qsTr("Authentication")
                        labelText: _gps ? (_gps.authenticationState.enumStringValue || na) : na
                        visible:   _isAuthVisible(_gps)
                    }
                }
            }
        }
    }
}
