import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

/// Reusable GPS status display block.
/// Shows fix status dot, lock type, satellite count, HDOP/VDOP, accuracy, and errors.
/// Works with any VehicleGPSFactGroup (GPS 1, GPS 2, or future receivers).
SettingsGroupLayout {
    id: root

    required property string heading
    required property var    gps

    property string na:      qsTr("N/A")
    property string valueNA: qsTr("-.--")
    property bool   showDetails: false

    showDividers: true
    visible:      !!gps

    RowLayout {
        spacing: ScreenTools.defaultFontPixelWidth

        FixStatusDot {
            lockValue: root.gps ? root.gps.lock.rawValue : -1
        }

        QGCLabel {
            text: root.gps ? root.gps.lock.enumStringValue : root.na
        }
    }

    LabelledLabel {
        label:     qsTr("Satellites")
        labelText: root.gps ? root.gps.count.valueString : root.na
    }

    LabelledLabel {
        label:     qsTr("HDOP / VDOP")
        labelText: {
            if (!root.gps) return root.valueNA
            var h = isNaN(root.gps.hdop.value) ? root.valueNA : root.gps.hdop.value.toFixed(1)
            var v = isNaN(root.gps.vdop.value) ? root.valueNA : root.gps.vdop.value.toFixed(1)
            return h + " / " + v
        }
    }

    LabelledLabel {
        label:     qsTr("Error")
        labelText: root.gps ? _gpsErrorText(root.gps.systemErrors.value) : ""
        visible:   root.gps && root.gps.systemErrors.value > 0
    }

    // -- Detail rows (shown in expanded view) --------------------------------

    LabelledLabel {
        label:     qsTr("Altitude MSL")
        labelText: root.gps ? root.gps.altitudeMSL.valueString : root.valueNA
        visible:   root.showDetails && root.gps && !isNaN(root.gps.altitudeMSL.rawValue)
    }

    LabelledLabel {
        label:     qsTr("Ground Speed")
        labelText: root.gps ? root.gps.groundSpeed.valueString : root.valueNA
        visible:   root.showDetails && root.gps && !isNaN(root.gps.groundSpeed.rawValue)
    }

    LabelledLabel {
        label:     qsTr("H Acc / V Acc")
        labelText: {
            if (!root.gps) return root.valueNA
            return GPSFormatter.formatAccuracy(root.gps.hAcc.rawValue)
                   + " / " + GPSFormatter.formatAccuracy(root.gps.vAcc.rawValue)
        }
        visible:   root.showDetails && root.gps
                   && (!isNaN(root.gps.hAcc.rawValue) || !isNaN(root.gps.vAcc.rawValue))
    }

    LabelledLabel {
        label:     qsTr("Speed Accuracy")
        labelText: root.gps ? root.gps.velAcc.valueString : root.valueNA
        visible:   root.showDetails && root.gps && !isNaN(root.gps.velAcc.rawValue)
    }

    LabelledLabel {
        label:     qsTr("Heading Accuracy")
        labelText: root.gps ? root.gps.hdgAcc.valueString : root.valueNA
        visible:   root.showDetails && root.gps && !isNaN(root.gps.hdgAcc.rawValue)
    }

    LabelledLabel {
        label:     qsTr("Course Over Ground")
        labelText: root.gps ? root.gps.courseOverGround.valueString : root.valueNA
        visible:   root.showDetails && root.gps && !isNaN(root.gps.courseOverGround.rawValue)
    }

    function _gpsErrorText(errorVal) {
        var errors = []
        if (errorVal & VehicleGPSFactGroup.ErrorIncomingCorrection) errors.push(qsTr("Incoming correction"))
        if (errorVal & VehicleGPSFactGroup.ErrorConfiguration)      errors.push(qsTr("Configuration"))
        if (errorVal & VehicleGPSFactGroup.ErrorSoftware)           errors.push(qsTr("Software"))
        if (errorVal & VehicleGPSFactGroup.ErrorAntenna)            errors.push(qsTr("Antenna"))
        if (errorVal & VehicleGPSFactGroup.ErrorEventCongestion)    errors.push(qsTr("Event congestion"))
        if (errorVal & VehicleGPSFactGroup.ErrorCpuOverload)        errors.push(qsTr("CPU overload"))
        if (errorVal & VehicleGPSFactGroup.ErrorOutputCongestion)   errors.push(qsTr("Output congestion"))
        return errors.length > 0 ? errors.join(", ") : ""
    }
}
