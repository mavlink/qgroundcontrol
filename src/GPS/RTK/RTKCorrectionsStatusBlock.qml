import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.NTRIP

/// Status block for the RTK corrections pipeline (NTRIP + GCS-connected RTK base).
/// Shows current corrections state (RTK Fixed / Float / Waiting / No Fix), source,
/// correction age, and a warning when corrections have been streaming with no RTK
/// achieved. Drop-in for GPSIndicatorPage and anywhere else that needs the same
/// RTK-side view of vehicle GPS.
SettingsGroupLayout {
    id: root

    required property var vehicle

    /// Whether RTCM corrections are flowing from any source.
    property bool correctionsActive: false

    property string valueNA: qsTr("-.--")

    readonly property int  _vehicleLock:   vehicle ? vehicle.gps.lock.rawValue : 0
    readonly property bool _vehicleHasRtk: _vehicleLock >= VehicleGPSFactGroup.FixRTKFloat
    readonly property var  _ntripMgr:      QGroundControl.ntripManager

    // Seconds corrections have been flowing without an RTK fix; gates the
    // "No RTK Fix" warning after kNoFixWarningSec.
    property int  _correctionsSentSec: 0
    readonly property int kNoFixWarningSec: 30

    heading:      qsTr("RTK Corrections")
    showDividers: true
    visible:      vehicle && correctionsActive

    QGCPalette { id: _pal }

    Timer {
        interval:          1000
        running:           root.correctionsActive && !root._vehicleHasRtk
        repeat:            true
        onTriggered:       root._correctionsSentSec++
        onRunningChanged:  root._correctionsSentSec = 0
    }

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        FixStatusDot {
            statusColor: {
                if (root._vehicleLock >= VehicleGPSFactGroup.FixRTKFixed)         return _pal.colorGreen
                if (root._vehicleLock >= VehicleGPSFactGroup.FixRTKFloat)         return _pal.colorOrange
                if (root.correctionsActive && root._correctionsSentSec > 30)      return _pal.colorRed
                if (root.correctionsActive)                                       return _pal.colorOrange
                return _pal.colorGrey
            }
        }

        QGCLabel {
            text: {
                if (root._vehicleLock >= VehicleGPSFactGroup.FixRTKFixed)         return qsTr("RTK Fixed")
                if (root._vehicleLock >= VehicleGPSFactGroup.FixRTKFloat)         return qsTr("RTK Float")
                if (root.correctionsActive && root._correctionsSentSec > 30)      return qsTr("No RTK Fix")
                if (root.correctionsActive)                                       return qsTr("Waiting for RTK...")
                return qsTr("No Corrections")
            }
        }
    }

    LabelledLabel {
        label:     qsTr("Source")
        labelText: {
            var sources = []
            if (root._ntripMgr && root._ntripMgr.connectionStatus === NTRIPManager.Connected)
                sources.push(qsTr("NTRIP"))
            if (QGroundControl.gpsManager && QGroundControl.gpsManager.deviceCount > 0)
                sources.push(qsTr("RTK Base"))
            return sources.join(" + ")
        }
    }

    LabelledLabel {
        label:     qsTr("Correction Age")
        labelText: {
            var stats = root._ntripMgr ? root._ntripMgr.connectionStats : null
            if (!stats || stats.correctionAgeSec < 0) return root.valueNA
            var age = stats.correctionAgeSec
            if (age < 1.0) return qsTr("< 1 s")
            return age.toFixed(0) + " s"
        }
        visible: root._ntripMgr && root._ntripMgr.connectionStatus === NTRIPManager.Connected
    }

    QGCLabel {
        Layout.fillWidth: true
        text:             qsTr("Corrections are being sent but vehicle has not achieved RTK fix. Check base station accuracy, satellite count, and signal quality.")
        wrapMode:         Text.WordWrap
        color:            _pal.colorOrange
        font.pointSize:   ScreenTools.smallFontPointSize
        visible:          root.correctionsActive && !root._vehicleHasRtk && root._correctionsSentSec > 30
    }
}
