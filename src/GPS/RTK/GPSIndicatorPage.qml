import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS
import QGroundControl.GPS.NTRIP

ToolIndicatorPage {
    showExpand: true

    property var    activeVehicle:  QGroundControl.multiVehicleManager.activeVehicle
    property var    _ntripMgr:      QGroundControl.ntripManager
    property var    _rtcmMavlink:   QGroundControl.gpsManager ? QGroundControl.gpsManager.rtcmMavlink : null
    property string na:             qsTr("N/A", "No data to display")
    property string valueNA:        qsTr("-.--", "No data to display")

    property bool   correctionsActive: {
        var ntripOn = _ntripMgr && _ntripMgr.connectionStatus === NTRIPManager.Connected
        var rtkOn   = QGroundControl.gpsManager && QGroundControl.gpsManager.deviceCount > 0
        return ntripOn || rtkOn
    }
    property int    _vehicleLock:       activeVehicle ? activeVehicle.gps.lock.rawValue : 0
    property bool   vehicleHasRtk:      _vehicleLock >= VehicleGPSFactGroup.FixRTKFloat
    property var    _posMgr:            QGroundControl.qgcPositionManager
    property var    _gcsGpsSettings:    QGroundControl.settingsManager.gcsGpsSettings
    property bool   _gcsNmeaActive:     !!_gcsGpsSettings && _gcsGpsSettings.nmeaActive

    // RTK convergence tracking
    property real   convergenceTimeSec: -1
    property real   convergenceStart:   0

    onCorrectionsActiveChanged: {
        if (correctionsActive && !vehicleHasRtk) {
            convergenceStart = Date.now() / 1000.0
            convergenceTimeSec = -1
        }
    }
    onVehicleHasRtkChanged: {
        if (vehicleHasRtk && convergenceStart > 0) {
            convergenceTimeSec = (Date.now() / 1000.0) - convergenceStart
            convergenceStart = 0
        }
    }

    // ====================================================================
    // Content: essential status at a glance
    // ====================================================================
    contentComponent: Component {
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // -- Vehicle GPS Status ------------------------------------------
            GPSStatusBlock {
                heading: qsTr("Vehicle GPS")
                gps:     activeVehicle ? activeVehicle.gps : null
                visible: !!activeVehicle
            }

            // -- RTK Corrections Status --------------------------------------
            RTKCorrectionsStatusBlock {
                vehicle:            activeVehicle
                correctionsActive:  correctionsActive
                valueNA:            valueNA
            }

            // -- Hints -------------------------------------------------------
            QGCLabel {
                Layout.fillWidth: true
                visible:          activeVehicle && !correctionsActive && !vehicleHasRtk
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
            GPSStatusBlock {
                heading:     qsTr("Vehicle GPS Details")
                gps:         activeVehicle ? activeVehicle.gps : null
                showDetails: true
                visible:     !!activeVehicle
            }

            // -- Vehicle RTK Status -------------------------------------------
            SettingsGroupLayout {
                heading:      qsTr("Vehicle RTK Status")
                showDividers: true
                visible:      activeVehicle && activeVehicle.gps.rtkRate.rawValue > 0

                LabelledLabel {
                    label:     qsTr("Baseline")
                    labelText: activeVehicle ? GPSFormatter.formatAccuracy(activeVehicle.gps.rtkBaseline.rawValue) : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.rtkBaseline.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("Accuracy")
                    labelText: activeVehicle ? GPSFormatter.formatAccuracy(activeVehicle.gps.rtkAccuracy.rawValue) : valueNA
                    visible:   activeVehicle && !isNaN(activeVehicle.gps.rtkAccuracy.rawValue)
                }

                LabelledLabel {
                    label:     qsTr("RTK Satellites")
                    labelText: activeVehicle ? activeVehicle.gps.rtkNumSats.valueString : na
                }

                LabelledLabel {
                    label:     qsTr("Baseline Rate")
                    labelText: activeVehicle ? (activeVehicle.gps.rtkRate.valueString + " Hz") : valueNA
                }

                LabelledLabel {
                    label:     qsTr("Ambiguity (IAR)")
                    labelText: activeVehicle ? activeVehicle.gps.rtkIAR.valueString : na
                    visible:   activeVehicle && activeVehicle.gps.rtkIAR.rawValue > 0
                }

                LabelledLabel {
                    label:     qsTr("Convergence Time")
                    labelText: convergenceTimeSec >= 0 ? GPSFormatter.formatDuration(Math.round(convergenceTimeSec)) : qsTr("Converging...")
                    visible:   vehicleHasRtk || (correctionsActive && convergenceStart > 0)
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
                    labelText: _rtcmMavlink ? GPSFormatter.formatDataSize(_rtcmMavlink.totalBytesSent) : valueNA
                }
            }

            // -- Vehicle GPS 2 -----------------------------------------------
            GPSStatusBlock {
                heading:     qsTr("Vehicle GPS 2")
                gps:         activeVehicle ? activeVehicle.gps2 : null
                showDetails: true
                visible:     !!activeVehicle && !!activeVehicle.gps2 && activeVehicle.gps2.telemetryAvailable
            }

            // -- RTK Base Station Status (per-device) ------------------------
            Repeater {
                model: QGroundControl.gpsManager ? QGroundControl.gpsManager.devices : null

                RTKBaseStationStatus {
                    required property var object
                    device: object
                }
            }

            // -- NTRIP Status ------------------------------------------------
            NTRIPConnectionStatus {
                rtcmMavlink: _rtcmMavlink
            }

            // -- Ground Station Position -------------------------------------
            SettingsGroupLayout {
                id: gcsSection
                heading:      qsTr("Ground Station")
                showDividers: true
                property bool gcsRtkActive:    Boolean(QGroundControl.gpsManager) && QGroundControl.gpsManager.deviceCount > 0
                // True once the NMEA device opens; distinguishes "port set, unresponsive" vs "streaming, no fix".
                property bool nmeaSourceActive: !!_posMgr && _posMgr.usingNmeaSource
                property bool posValid:        !!_posMgr && _posMgr.gcsPosition.isValid
                visible:      Boolean(_posMgr) && (posValid || !!_gcsNmeaActive || gcsRtkActive)

                RowLayout {
                    spacing: ScreenTools.defaultFontPixelWidth
                    visible: Boolean(!gcsSection.posValid && (!!_gcsNmeaActive || gcsSection.gcsRtkActive))

                    FixStatusDot { statusColor: qgcPal.colorOrange }

                    QGCLabel {
                        text: {
                            if (gcsSection.gcsRtkActive)    return qsTr("Waiting for RTK GPS position...")
                            if (gcsSection.nmeaSourceActive) return qsTr("NMEA connected — waiting for fix…")
                            if (_gcsNmeaActive)             return qsTr("NMEA configured — device not responding")
                            return qsTr("Waiting for position data...")
                        }
                    }
                }

                LabelledLabel {
                    label:     qsTr("Source")
                    labelText: _posMgr && _posMgr.sourceDescription ? _posMgr.sourceDescription : na
                    visible:   Boolean(_posMgr) && _posMgr.sourceDescription !== undefined && _posMgr.sourceDescription !== ""
                }

                LabelledLabel {
                    label:     qsTr("Position")
                    labelText: {
                        var mgr = _posMgr
                        if (!mgr || !mgr.gcsPosition.isValid) return valueNA
                        return GPSFormatter.formatCoordinate(mgr.gcsPosition.latitude, mgr.gcsPosition.longitude)
                    }
                    visible: !!gcsSection.posValid
                }

                LabelledLabel {
                    label:     qsTr("Accuracy")
                    labelText: {
                        var mgr = _posMgr
                        if (!mgr || !isFinite(mgr.gcsPositionHorizontalAccuracy)) return valueNA
                        return GPSFormatter.formatAccuracy(mgr.gcsPositionHorizontalAccuracy)
                    }
                    visible: !!gcsSection.posValid && !!_posMgr && isFinite(_posMgr.gcsPositionHorizontalAccuracy)
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

                        FixStatusDot {
                            dotSize:  ScreenTools.defaultFontPixelHeight * 0.5
                            severity: model.severity
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
            SignalIntegrityBlock {
                heading: qsTr("GPS 1 Signal Integrity")
                gps:     activeVehicle ? activeVehicle.gps : null
            }

            SignalIntegrityBlock {
                heading: qsTr("GPS 2 Signal Integrity")
                gps:     activeVehicle ? activeVehicle.gps2 : null
            }
        }
    }
}
