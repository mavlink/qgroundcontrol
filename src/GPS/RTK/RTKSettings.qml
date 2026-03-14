import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    id: root
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var  rtkSettings:     QGroundControl.settingsManager.rtkSettings
    property var  _autoConnect:    QGroundControl.settingsManager.autoConnectSettings
    property var  _gpsRtk:         QGroundControl.gpsRtk
    property var  _gpsMgr:         QGroundControl.gpsManager
    property int  _rtkDevCount:    _gpsMgr ? _gpsMgr.deviceCount : 0
    property var  useFixedPosition: rtkSettings.useFixedBasePosition.rawValue
    property var  manufacturer:    rtkSettings.baseReceiverManufacturers.rawValue

    readonly property int _trimble:     0b00001
    readonly property int _septentrio:  0b00010
    readonly property int _femtomes:    0b00100
    readonly property int _ublox:       0b01000
    readonly property int _nmea:        0b10000
    readonly property int _allRtk:      _trimble | _septentrio | _femtomes | _ublox
    readonly property int _all:         _allRtk | _nmea
    property int          settingsDisplayId: _all

    readonly property var _gpsTypeStrings: ["blox", "trimble", "septentrio", "femtomes", "blox", "nmea"]

    function updateSettingsDisplayId() {
        switch (manufacturer) {
            case 0: settingsDisplayId = _all;        break
            case 1: settingsDisplayId = _trimble;    break
            case 2: settingsDisplayId = _septentrio; break
            case 3: settingsDisplayId = _femtomes;   break
            case 4: settingsDisplayId = _ublox;      break
            case 5: settingsDisplayId = _nmea;       break
            default: settingsDisplayId = _all
        }
    }

    function _gpsTypeForManufacturer(mfr) {
        return (mfr >= 0 && mfr < _gpsTypeStrings.length) ? _gpsTypeStrings[mfr] : "blox"
    }

    onManufacturerChanged: updateSettingsDisplayId()
    Component.onCompleted: updateSettingsDisplayId()

    QGCPalette { id: qgcPal }

    // -- RTK Connection -------------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:          qsTr("RTK Connection")
        showDividers:     true

        ConnectionStatusRow {
            statusColor: {
                if (!_gpsMgr) return qgcPal.colorGrey
                if (_gpsRtk && _gpsRtk.valid && _gpsRtk.valid.value) return qgcPal.colorGreen
                if (_rtkDevCount > 0) return qgcPal.colorOrange
                return qgcPal.colorGrey
            }
            statusText: {
                if (!_gpsMgr || _rtkDevCount === 0) return qsTr("No RTK base station connected")
                if (_gpsRtk && _gpsRtk.valid && _gpsRtk.valid.value) return qsTr("RTK streaming")
                return qsTr("Connected — survey-in active")
            }
            buttonText:    qsTr("Disconnect")
            buttonVisible: _rtkDevCount > 0
            onButtonClicked: _gpsMgr.disconnectAll()
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _gpsMgr && _gpsMgr.lastError !== ""
            text:    _gpsMgr ? _gpsMgr.lastError : ""
            color:   qgcPal.colorRed
            wrapMode: Text.WordWrap
            font.pointSize: ScreenTools.smallFontPointSize
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:    qsTr("AutoConnect RTK GPS")
            fact:    _autoConnect.autoConnectRTKGPS
            visible: fact.visible
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _rtkDevCount === 0 && _autoConnect.autoConnectRTKGPS.rawValue
            text:    qsTr("Devices matching known RTK receivers will connect automatically when plugged in.")
            font.pointSize: ScreenTools.smallFontPointSize
            color:   qgcPal.colorGrey
            wrapMode: Text.WordWrap
        }

        GridLayout {
            columns:         2
            Layout.fillWidth: true
            columnSpacing:   ScreenTools.defaultFontPixelWidth
            visible:         _rtkDevCount === 0

            QGCLabel { text: qsTr("Receiver") }
            FactComboBox {
                Layout.fillWidth: true
                fact:    rtkSettings.baseReceiverManufacturers
                visible: fact.visible
            }

            QGCLabel { text: qsTr("Serial Port") }
            QGCComboBox {
                id:               rtkPortCombo
                Layout.fillWidth: true
                model:            QGroundControl.linkManager.serialPorts
                currentIndex:     -1
            }
        }

        QGCButton {
            Layout.fillWidth: true
            text:    qsTr("Connect")
            visible: _rtkDevCount === 0 && !_autoConnect.autoConnectRTKGPS.rawValue
            enabled: rtkPortCombo.currentIndex >= 0
            onClicked: {
                var port = rtkPortCombo.currentText
                var typeStr = root._gpsTypeForManufacturer(root.manufacturer)
                _gpsMgr.connectGPS(port, typeStr)
            }
        }
    }

    // -- Base Station Configuration -------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("RTK Base Station")
        headingDescription: qsTr("Configure the local RTK GPS receiver used as a base station")
        showDividers:       true

        GridLayout {
            columns:         2
            Layout.fillWidth: true
            columnSpacing:   ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text:    qsTr("Output Mode")
                visible: settingsDisplayId & _allRtk
            }
            FactComboBox {
                Layout.fillWidth: true
                fact:    rtkSettings.gpsOutputMode
                visible: settingsDisplayId & _allRtk
            }
        }
    }

    // -- Survey-In / Fixed Position ------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Base Position Mode")
        headingDescription: qsTr("How the base station determines its precise location")
        showDividers:       true
        visible:            settingsDisplayId & _allRtk

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 2

            QGCRadioButton {
                text:    qsTr("Survey-In")
                checked: useFixedPosition == BaseModeDefinition.BaseSurveyIn
                onClicked: rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseSurveyIn
            }

            QGCRadioButton {
                text:    qsTr("Fixed Position")
                checked: useFixedPosition == BaseModeDefinition.BaseFixed
                onClicked: rtkSettings.useFixedBasePosition.rawValue = BaseModeDefinition.BaseFixed
            }
        }

        FactSlider {
            Layout.fillWidth:      true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
            label: qsTr("Accuracy")
            fact:  rtkSettings.surveyInAccuracyLimit
            majorTickStepSize: 0.1
            visible: useFixedPosition == BaseModeDefinition.BaseSurveyIn
                     && rtkSettings.surveyInAccuracyLimit.visible
                     && (settingsDisplayId & _ublox)
        }

        FactSlider {
            Layout.fillWidth:      true
            Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 40
            label: qsTr("Min Duration")
            fact:  rtkSettings.surveyInMinObservationDuration
            majorTickStepSize: 10
            visible: useFixedPosition == BaseModeDefinition.BaseSurveyIn
                     && rtkSettings.surveyInMinObservationDuration.visible
                     && (settingsDisplayId & (_ublox | _femtomes | _trimble))
        }

        LabelledFactTextField {
            label:   rtkSettings.fixedBasePositionLatitude.shortDescription
            fact:    rtkSettings.fixedBasePositionLatitude
            visible: useFixedPosition == BaseModeDefinition.BaseFixed
        }

        LabelledFactTextField {
            label:   rtkSettings.fixedBasePositionLongitude.shortDescription
            fact:    rtkSettings.fixedBasePositionLongitude
            visible: useFixedPosition == BaseModeDefinition.BaseFixed
        }

        LabelledFactTextField {
            label:   rtkSettings.fixedBasePositionAltitude.shortDescription
            fact:    rtkSettings.fixedBasePositionAltitude
            visible: useFixedPosition == BaseModeDefinition.BaseFixed
        }

        LabelledFactTextField {
            label:   rtkSettings.fixedBasePositionAccuracy.shortDescription
            fact:    rtkSettings.fixedBasePositionAccuracy
            visible: useFixedPosition == BaseModeDefinition.BaseFixed && (settingsDisplayId & _ublox)
        }

        LabelledButton {
            label:      qsTr("Current Base Position")
            buttonText: enabled ? qsTr("Save") : qsTr("Not Yet Valid")
            visible:    useFixedPosition == BaseModeDefinition.BaseFixed
            enabled:    _gpsRtk && _gpsRtk.valid.value

            onClicked: {
                if (!_gpsRtk) return
                rtkSettings.fixedBasePositionLatitude.rawValue  = _gpsRtk.currentLatitude.rawValue
                rtkSettings.fixedBasePositionLongitude.rawValue = _gpsRtk.currentLongitude.rawValue
                rtkSettings.fixedBasePositionAltitude.rawValue  = _gpsRtk.currentAltitude.rawValue
                rtkSettings.fixedBasePositionAccuracy.rawValue  = _gpsRtk.currentAccuracy.rawValue
            }
        }
    }

    // -- U-blox Configuration ------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("U-blox Configuration")
        headingDescription: qsTr("Advanced settings for u-blox RTK receivers")
        showDividers:       true
        visible:            settingsDisplayId & _ublox

        GridLayout {
            columns:         2
            Layout.fillWidth: true
            columnSpacing:   ScreenTools.defaultFontPixelWidth

            QGCLabel { text: rtkSettings.ubxMode.shortDescription }
            FactComboBox {
                Layout.fillWidth: true
                fact: rtkSettings.ubxMode
            }

            QGCLabel { text: rtkSettings.ubxDynamicModel.shortDescription }
            FactComboBox {
                Layout.fillWidth: true
                fact: rtkSettings.ubxDynamicModel
            }

            QGCLabel { text: rtkSettings.ubxUart2Baudrate.shortDescription }
            FactComboBox {
                Layout.fillWidth: true
                fact: rtkSettings.ubxUart2Baudrate
            }
        }

        LabelledFactTextField {
            label: rtkSettings.ubxDgnssTimeout.shortDescription
            fact:  rtkSettings.ubxDgnssTimeout
        }

        LabelledFactTextField {
            label: rtkSettings.ubxMinCno.shortDescription
            fact:  rtkSettings.ubxMinCno
        }

        LabelledFactTextField {
            label: rtkSettings.ubxMinElevation.shortDescription
            fact:  rtkSettings.ubxMinElevation
        }

        LabelledFactTextField {
            label: rtkSettings.ubxOutputRate.shortDescription
            fact:  rtkSettings.ubxOutputRate
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: rtkSettings.ubxPpkOutput.shortDescription
            fact: rtkSettings.ubxPpkOutput
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text: rtkSettings.ubxJamDetSensitivityHi.shortDescription
            fact: rtkSettings.ubxJamDetSensitivityHi
        }
    }
}
