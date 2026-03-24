import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

// Parameters that share the same name across vehicle types but have different metadata:
//   FS_THR_ENABLE   — Copter (8 values, 0-7) vs Rover (3 values, 0-2)                      [split: copterThrottleFailsafe / roverThrottleFailsafe]
//   FS_GCS_ENABLE   — Copter (8 values, 0-7) vs Rover (3 values, 0-2)                      [split: copterGcsFailsafe / roverGcsFailsafe]
//   FS_OPTIONS      — Copter (6 bitmask bits, 0-5) vs Rover (1 bitmask bit, 0 only)        [handled inline per component; Plane has no FS_OPTIONS]
//   FS_EKF_ACTION   — Copter (Report/Land/AltHold/LandAlways) vs Rover (Disabled/Hold/Report) [split: copterEkfFailsafe / roverEkfFailsafe]
//   FS_EKF_THRESH   — Copter adds "0: Disabled"; Plane/Rover do not                        [split: copterEkfFailsafe / roverEkfFailsafe]
//   FS_CRASH_CHECK  — Copter (0/1) vs Rover (0/1/2 adds HoldAndDisarm)                     [split: copterGeneralFS / roverGeneralFS]
//   FENCE_ACTION    — All 3 vehicle types have completely different action sets              [copter-only: copterGeoFence]
//   BATT_FS_LOW_ACT — All 3 vehicle types have different action sets                        [shared: batteryFailsafeComponent — uses combo from firmware metadata]
//   BATT_FS_CRT_ACT — All 3 vehicle types have different action sets                        [shared: batteryFailsafeComponent — uses combo from firmware metadata]

SetupPage {
    id:             safetyPage
    pageComponent:  safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    ScreenTools.defaultFontPixelHeight

            property var _controller: controller

            FactPanelController { id: controller; }

            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            APMBatteryParams {
                id:           battParams
                controller:   _controller
                batteryIndex: 0
            }

            property int  _batteryCount:        battParams.getBatteryCount()
            property int  _enabledBatteryCount:  battParams.getEnabledBatteryCount()

            // Older firmwares use ARMING_CHECK. Newer firmwares use ARMING_SKIPCHK.
            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK", false /* reportMissing */)
            property Fact _armingSkipCheck: controller.getParameterFact(-1, "ARMING_SKIPCHK", false /* reportMissing */)

            property real _margins:         ScreenTools.defaultFontPixelWidth / 2
            property real _comboWidth:      ScreenTools.defaultFontPixelWidth * 30
            property bool _showIcon:        !ScreenTools.isTinyScreen
            property bool _roverFirmware:   controller.parameterExists(-1, "MODE1") // This catches all usage of ArduRover firmware vehicle types: Rover, Boat...

            property string _restartRequired: qsTr("Requires vehicle reboot")

            // FS_OPTIONS bitmask constants (from ArduCopter FailsafeOption enum)
            readonly property int _fsOptionRCContinueAuto:       1   // Bit 0: Continue in Auto on RC/Throttle failsafe
            readonly property int _fsOptionGCSContinueAuto:      2   // Bit 1: Continue in Auto on GCS failsafe
            readonly property int _fsOptionRCContinueGuided:     4   // Bit 2: Continue in Guided on RC/Throttle failsafe
            readonly property int _fsOptionContinueLanding:      8   // Bit 3: Continue if landing on any failsafe
            readonly property int _fsOptionGCSContinuePilot:     16  // Bit 4: Continue in pilot control on GCS failsafe
            readonly property int _fsOptionReleaseGripper:       32  // Bit 5: Release gripper on any failsafe

            property bool _fsOptionsAvailable: controller.parameterExists(-1, "FS_OPTIONS")
            property Fact _fsOptions:          controller.getParameterFact(-1, "FS_OPTIONS", false /* reportMissing */)

            // FS_THR_ENABLE value constants (from ArduCopter)
            readonly property int _fsThrDisabled:                    0
            readonly property int _fsThrEnabledAlwaysRTL:            1
            readonly property int _fsThrEnabledAlwaysLand:           3
            readonly property int _fsThrEnabledAlwaysSmartRTLOrRTL:  4
            readonly property int _fsThrEnabledAlwaysSmartRTLOrLand: 5
            readonly property int _fsThrEnabledAutoDoLandOrRTL:      6
            readonly property int _fsThrEnabledAlwaysBrakeOrLand:    7

            // FS_DR_ENABLE value constants (from ArduCopter)
            readonly property int _fsDrDisabled:                    0
            readonly property int _fsDrLand:                        1
            readonly property int _fsDrRTL:                         2
            readonly property int _fsDrSmartRTLOrRTL:               3
            readonly property int _fsDrSmartRTLOrLand:              4
            readonly property int _fsDrAutoDoLandOrRTL:             6

            // FS_GCS_ENABLE value constants (from ArduCopter)
            readonly property int _fsGcsDisabled:                   0
            readonly property int _fsGcsRTL:                        1
            readonly property int _fsGcsSmartRTLOrRTL:              3
            readonly property int _fsGcsSmartRTLOrLand:             4
            readonly property int _fsGcsLand:                       5
            readonly property int _fsGcsAutoDoLandOrRTL:            6
            readonly property int _fsGcsBrakeOrLand:                7

            // ----- Loaders (display order) -----

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterRTL : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeRTL : undefined
            }

            Repeater {
                model: _batteryCount

                QGCGroupBox {
                    required property int index

                    APMBatteryParams {
                        id:           _batt
                        controller:   _controller
                        batteryIndex: index
                    }

                    title:   _enabledBatteryCount > 1
                                ? qsTr("Battery %1 Failsafe").arg(battParams.labelForIndex(index))
                                : qsTr("Battery Failsafe")
                    visible: _batt.monitorEnabled

                    Loader {
                        sourceComponent: _batt.paramsAvailable ? batteryFailsafeComponent : restartRequiredComponent

                        property Fact failsafeBattLowAct:      _batt.fsLowAct
                        property Fact failsafeBattCritAct:     _batt.fsCritAct
                        property Fact failsafeBattLowMah:      _batt.lowMah
                        property Fact failsafeBattCritMah:     _batt.critMah
                        property Fact failsafeBattLowVoltage:  _batt.lowVolt
                        property Fact failsafeBattCritVoltage: _batt.critVolt
                    }
                }
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGcsFailsafe : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeGcsFailsafe : undefined
            }

            Loader {
                sourceComponent: _roverFirmware ? roverGcsFailsafe : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeGeneralFS : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? rcFailsafeComponent : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterThrottleFailsafe : undefined
            }

            Loader {
                sourceComponent: _roverFirmware ? roverThrottleFailsafe : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterEkfFailsafe : undefined
            }

            Loader {
                sourceComponent: _roverFirmware ? roverEkfFailsafe : undefined
            }

            Loader {
                sourceComponent: (controller.vehicle.multiRotor && controller.parameterExists(-1, "FS_DR_ENABLE")) ? deadReckoningFailsafeComponent : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeneralFS : undefined
            }

            Loader {
                sourceComponent: _roverFirmware ? roverGeneralFS : undefined
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeoFence : undefined
            }

            QGCGroupBox {
                id:    armingChecksGroupBox
                title: _armingCheck ? qsTr("Arming Checks") : qsTr("Skip Arming Checks")

                property bool _hasSkippedChecks: _armingCheck ? _armingCheck.value != 1 : _armingSkipCheck.value != 0
                property bool _allowEditing: false

                ColumnLayout {
                    spacing: _margins

                    QGCLabel {
                        wrapMode:           Text.WordWrap
                        color:              qgcPal.warningText
                        text:               qsTr("Warning: Skipping arming checks can lead to loss of Vehicle control.")
                        Layout.fillWidth:   true
                    }

                    QGCCheckBoxSlider {
                        Layout.fillWidth:   true
                        text:               qsTr("Allow changes")
                        checked:            armingChecksGroupBox._hasSkippedChecks || armingChecksGroupBox._allowEditing
                        enabled:            !armingChecksGroupBox._hasSkippedChecks

                        onClicked: armingChecksGroupBox._allowEditing = checked
                    }

                    FactBitmask {
                        firstEntryIsAll:        _armingCheck ? true : false
                        fact:                   _armingCheck ? _armingCheck : _armingSkipCheck
                        Layout.preferredWidth:  safetyPage.availableWidth * 0.75
                        visible:                armingChecksGroupBox._hasSkippedChecks || armingChecksGroupBox._allowEditing
                    }
                }
            }

            // ----- Component definitions -----

            Component {
                id: batteryFailsafeComponent

                ColumnLayout {
                    spacing: _margins

                    LabelledFactComboBox {
                        label:              qsTr("Low action")
                        fact:               failsafeBattLowAct
                        indexModel:         false
                        comboBoxPreferredWidth: _comboWidth
                        Layout.fillWidth:   true
                    }

                    LabelledFactComboBox {
                        label:              qsTr("Critical action")
                        fact:               failsafeBattCritAct
                        indexModel:         false
                        comboBoxPreferredWidth: _comboWidth
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Low voltage threshold")
                        fact:               failsafeBattLowVoltage
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Critical voltage threshold")
                        fact:               failsafeBattCritVoltage
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Low mAh threshold")
                        fact:               failsafeBattLowMah
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }

                    LabelledFactTextField {
                        label:              qsTr("Critical mAh threshold")
                        fact:               failsafeBattCritMah
                        textFieldShowUnits: true
                        Layout.fillWidth:   true
                    }
                }
            }

            Component {
                id: restartRequiredComponent

                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        text: _restartRequired
                    }

                    QGCButton {
                        text:       qsTr("Reboot vehicle")
                        onClicked:  controller.vehicle.rebootVehicle()
                    }
                }
            }

            Component {
                id: copterRTL

                QGCGroupBox {
                    title: qsTr("Return to Launch")

                    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPD_MS")
                    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT_M")
                    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
                    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL_M")
                    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
                    property bool _rtlAltIsMeters:  controller.parameterExists(-1, "noremap.RTL_ALT_M")

                    RowLayout {
                        spacing: ScreenTools.defaultFontPixelHeight

                        QGCColoredImage {
                            id:                     icon
                            visible:                _showIcon
                            Layout.alignment:       Qt.AlignTop
                            Layout.preferredHeight: ScreenTools.defaultFontPixelWidth * 20
                            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 20
                            color:                  qgcPal.text
                            sourceSize.width:       width
                            mipmap:                 true
                            fillMode:               Image.PreserveAspectFit
                            source:                 "/qmlimages/ReturnToHomeAltitude.svg"
                        }

                        ColumnLayout {
                            spacing: _margins

                            QGCRadioButton {
                                text:    qsTr("Return at current altitude")
                                checked: _rtlAltFact.value == 0

                                onClicked: _rtlAltFact.value = 0
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: _margins

                                QGCRadioButton {
                                    Layout.fillWidth: true
                                    text:    qsTr("Return at specified altitude")
                                    checked: _rtlAltFact.value != 0

                                    // RTL_ALT_M (4.7+) is in meters, RTL_ALT (pre-4.7) is in centimeters
                                    onClicked: _rtlAltFact.value = _rtlAltIsMeters ? 15 : 1500
                                }

                                FactTextField {
                                    fact:      _rtlAltFact
                                    showUnits: true
                                    enabled:   _rtlAltFact.value != 0
                                }
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: _margins

                                QGCCheckBox {
                                    id:      homeLoiterCheckbox
                                    Layout.fillWidth: true
                                    checked: _rtlLoitTimeFact.value > 0
                                    text:    qsTr("Loiter above Home for")

                                    onClicked: _rtlLoitTimeFact.value = (checked ? 60 : 0)
                                }

                                FactTextField {
                                    fact:      _rtlLoitTimeFact
                                    showUnits: true
                                    enabled:   homeLoiterCheckbox.checked
                                }
                            }

                            LabelledFactTextField {
                                label:              qsTr("Final land stage altitude")
                                fact:               _rtlAltFinalFact
                                textFieldShowUnits: true
                                Layout.fillWidth:   true
                            }

                            LabelledFactTextField {
                                label:              qsTr("Final land stage descent speed")
                                fact:               _landSpeedFact
                                textFieldShowUnits: true
                                Layout.fillWidth:   true
                            }
                        }
                    }
                }
            }

            Component {
                id: planeRTL

                QGCGroupBox {
                    title: qsTr("Return to Launch")

                    property Fact _rtlAltFact: controller.getParameterFact(-1, "RTL_ALTITUDE")

                    ColumnLayout {
                        spacing: _margins

                        QGCRadioButton {
                            text:    qsTr("Return at current altitude")
                            checked: _rtlAltFact.value < 0

                            onClicked: _rtlAltFact.value = -1
                        }

                        RowLayout {
                            spacing: _margins

                            QGCRadioButton {
                                text:    qsTr("Return at specified altitude")
                                checked: _rtlAltFact.value >= 0

                                onClicked: _rtlAltFact.value = 10000
                            }

                            FactTextField {
                                fact:      _rtlAltFact
                                showUnits: true
                                enabled:   _rtlAltFact.value >= 0
                            }
                        }
                    }
                }
            }

            Component {
                id: copterGcsFailsafe

                QGCGroupBox {
                    title: qsTr("Ground Station Failsafe")

                    property Fact _gcsEnable:  controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _gcsTimeout: controller.getParameterFact(-1, "FS_GCS_TIMEOUT")
                    property bool _gcsEnabled: _gcsEnable.rawValue !== _fsGcsDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _gcsEnabled

                            onClicked: {
                                if (checked) {
                                    _gcsEnable.rawValue = _fsGcsRTL
                                } else {
                                    _gcsEnable.rawValue = _fsGcsDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:              qsTr("Timeout")
                            fact:               _gcsTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _gcsEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _gcsEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("RTL")
                                checked:   _gcsEnable.rawValue === _fsGcsRTL
                                onClicked: _gcsEnable.rawValue = _fsGcsRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Land")
                                checked:   _gcsEnable.rawValue === _fsGcsLand
                                onClicked: _gcsEnable.rawValue = _fsGcsLand
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or RTL")
                                checked:   _gcsEnable.rawValue === _fsGcsSmartRTLOrRTL
                                onClicked: _gcsEnable.rawValue = _fsGcsSmartRTLOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or Land")
                                checked:   _gcsEnable.rawValue === _fsGcsSmartRTLOrLand
                                onClicked: _gcsEnable.rawValue = _fsGcsSmartRTLOrLand
                            }

                            QGCRadioButton {
                                text:      qsTr("Auto DO_LAND_START or RTL")
                                checked:   _gcsEnable.rawValue === _fsGcsAutoDoLandOrRTL
                                onClicked: _gcsEnable.rawValue = _fsGcsAutoDoLandOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Brake or Land")
                                checked:   _gcsEnable.rawValue === _fsGcsBrakeOrLand
                                onClicked: _gcsEnable.rawValue = _fsGcsBrakeOrLand
                            }
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _fsOptionsAvailable && _gcsEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _fsOptionsAvailable && _gcsEnabled

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Auto mode")
                                fact:               _fsOptions
                                bitMask:            _fsOptionGCSContinueAuto
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In pilot control")
                                fact:               _fsOptions
                                bitMask:            _fsOptionGCSContinuePilot
                            }
                        }
                    }
                }
            }

            Component {
                id: planeGcsFailsafe

                QGCGroupBox {
                    title: qsTr("Ground Station Failsafe")

                    // Plane: 0=Disabled, 1=Heartbeat, 2=Heartbeat+REMRSSI, 3=Heartbeat+AUTO
                    readonly property int _planeGcsDisabled:            0
                    readonly property int _planeGcsHeartbeat:           1
                    readonly property int _planeGcsHeartbeatAndRemRSSI: 2
                    readonly property int _planeGcsHeartbeatAndAuto:    3

                    property Fact _gcsEnable:  controller.getParameterFact(-1, "FS_GCS_ENABL")
                    property bool _gcsEnabled: _gcsEnable.rawValue !== _planeGcsDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _gcsEnabled

                            onClicked: {
                                if (checked) {
                                    _gcsEnable.rawValue = _planeGcsHeartbeat
                                } else {
                                    _gcsEnable.rawValue = _planeGcsDisabled
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _gcsEnabled

                            QGCLabel { text: qsTr("Trigger:") }

                            QGCRadioButton {
                                text:      qsTr("Heartbeat")
                                checked:   _gcsEnable.rawValue === _planeGcsHeartbeat
                                onClicked: _gcsEnable.rawValue = _planeGcsHeartbeat
                            }

                            QGCRadioButton {
                                text:      qsTr("Heartbeat and Remote RSSI")
                                checked:   _gcsEnable.rawValue === _planeGcsHeartbeatAndRemRSSI
                                onClicked: _gcsEnable.rawValue = _planeGcsHeartbeatAndRemRSSI
                            }

                            QGCRadioButton {
                                text:      qsTr("Heartbeat and AUTO")
                                checked:   _gcsEnable.rawValue === _planeGcsHeartbeatAndAuto
                                onClicked: _gcsEnable.rawValue = _planeGcsHeartbeatAndAuto
                            }
                        }
                    }
                }
            }

            Component {
                id: roverGcsFailsafe

                QGCGroupBox {
                    title: qsTr("Ground Station Failsafe")

                    // Rover: 0=Disabled, 1=Enabled, 2=Enabled Continue with Mission in Auto
                    readonly property int _roverGcsDisabled:             0
                    readonly property int _roverGcsEnabled:              1
                    readonly property int _roverGcsContinueAutoMission:  2

                    property Fact _gcsEnable:  controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _gcsTimeout: controller.getParameterFact(-1, "FS_GCS_TIMEOUT")
                    property bool _gcsEnabled: _gcsEnable.rawValue !== _roverGcsDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _gcsEnabled

                            onClicked: {
                                if (checked) {
                                    _gcsEnable.rawValue = _roverGcsEnabled
                                } else {
                                    _gcsEnable.rawValue = _roverGcsDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:              qsTr("Timeout")
                            fact:               _gcsTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _gcsEnabled
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _gcsEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _gcsEnabled

                            QGCCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Auto mode")
                                checked:            _gcsEnable.rawValue === _roverGcsContinueAutoMission

                                onClicked: {
                                    _gcsEnable.rawValue = checked ? _roverGcsContinueAutoMission : _roverGcsEnabled
                                }
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Hold mode")
                                visible:            _fsOptionsAvailable
                                fact:               _fsOptions
                                bitMask:            1  // Rover FS_OPTIONS bit 0
                            }
                        }
                    }
                }
            }

            Component {
                id: rcFailsafeComponent

                QGCGroupBox {
                    title: qsTr("RC Failsafe")

                    ColumnLayout {
                        spacing: _margins

                        QGCLabel {
                            text: qsTr("Always enabled")
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _fsOptionsAvailable
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _fsOptionsAvailable

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Auto mode")
                                fact:               _fsOptions
                                bitMask:            _fsOptionRCContinueAuto
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Guided mode")
                                fact:               _fsOptions
                                bitMask:            _fsOptionRCContinueGuided
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("Landing")
                                fact:               _fsOptions
                                bitMask:            _fsOptionContinueLanding
                            }
                        }
                    }
                }
            }

            Component {
                id: planeGeneralFS

                QGCGroupBox {
                    title: qsTr("Failsafe Triggers")

                    property Fact _failsafeThrEnable:      controller.getParameterFact(-1, "THR_FAILSAFE")
                    property Fact _failsafeThrValue:       controller.getParameterFact(-1, "THR_FS_VALUE")
                    property Fact _failsafeShortAction:    controller.getParameterFact(-1, "FS_SHORT_ACTN")
                    property Fact _failsafeLongAction:     controller.getParameterFact(-1, "FS_LONG_ACTN")
                    property Fact _failsafeLongTimeout:    controller.getParameterFact(-1, "FS_LONG_TIMEOUT")
                    property bool _isQuadPlane:            controller.parameterExists(-1, "Q_TRANS_FAIL")
                    property Fact _transFailTimeout:       controller.getParameterFact(-1, "Q_TRANS_FAIL", false /* reportMissing */)
                    property Fact _transFailAction:        controller.getParameterFact(-1, "Q_TRANS_FAIL_ACT", false /* reportMissing */)

                    ColumnLayout {
                        spacing: _margins

                        RowLayout {
                            Layout.fillWidth: true

                            QGCCheckBox {
                                id:                 throttleEnableCheckBox
                                text:               qsTr("Throttle PWM threshold")
                                checked:            _failsafeThrEnable.value === 1
                                Layout.fillWidth:   true

                                onClicked: _failsafeThrEnable.value = (checked ? 1 : 0)
                            }

                            FactTextField {
                                fact:               _failsafeThrValue
                                showUnits:          true
                                enabled:            throttleEnableCheckBox.checked
                            }
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Short failsafe action")
                            fact:             _failsafeShortAction
                            indexModel:       false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("Long failsafe action")
                            fact:             _failsafeLongAction
                            indexModel:       false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                        }

                        LabelledFactTextField {
                            label:              qsTr("Long failsafe timeout")
                            fact:               _failsafeLongTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                        }

                        LabelledFactComboBox {
                            label:            qsTr("VTOL transition failure action")
                            fact:             _transFailAction
                            indexModel:       false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible:          _isQuadPlane
                        }

                        LabelledFactTextField {
                            label:              qsTr("VTOL transition failure timeout")
                            fact:               _transFailTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _isQuadPlane
                        }
                    }
                }
            }

            Component {
                id: copterThrottleFailsafe

                QGCGroupBox {
                    title: qsTr("Throttle Failsafe")

                    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "FS_THR_VALUE")
                    property bool _thrEnabled:          _failsafeThrEnable.rawValue !== _fsThrDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _thrEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysRTL
                                } else {
                                    _failsafeThrEnable.rawValue = _fsThrDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:              qsTr("PWM threshold")
                            fact:               _failsafeThrValue
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _thrEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _thrEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Always RTL")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAlwaysRTL
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Always Land")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAlwaysLand
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysLand
                            }

                            QGCRadioButton {
                                text:      qsTr("Always SmartRTL or RTL")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAlwaysSmartRTLOrRTL
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysSmartRTLOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Always SmartRTL or Land")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAlwaysSmartRTLOrLand
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysSmartRTLOrLand
                            }

                            QGCRadioButton {
                                text:      qsTr("Auto DO_LAND_START or RTL")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAutoDoLandOrRTL
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAutoDoLandOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Always Brake or Land")
                                checked:   _failsafeThrEnable.rawValue === _fsThrEnabledAlwaysBrakeOrLand
                                onClicked: _failsafeThrEnable.rawValue = _fsThrEnabledAlwaysBrakeOrLand
                            }
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _fsOptionsAvailable && _thrEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _fsOptionsAvailable && _thrEnabled

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Auto mode")
                                fact:               _fsOptions
                                bitMask:            _fsOptionRCContinueAuto
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Guided mode")
                                fact:               _fsOptions
                                bitMask:            _fsOptionRCContinueGuided
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("Landing")
                                fact:               _fsOptions
                                bitMask:            _fsOptionContinueLanding
                            }
                        }
                    }
                }
            }

            Component {
                id: roverThrottleFailsafe

                QGCGroupBox {
                    title: qsTr("Throttle Failsafe")

                    // Rover: 0=Disabled, 1=Enabled, 2=Continue with Mission in Auto
                    readonly property int _roverThrDisabled:             0
                    readonly property int _roverThrEnabled:              1
                    readonly property int _roverThrContinueAutoMission:  2

                    // Rover FS_ACTION value constants
                    readonly property int _roverActionNothing:           0
                    readonly property int _roverActionRTL:               1
                    readonly property int _roverActionHold:              2
                    readonly property int _roverActionSmartRTLOrRTL:     3
                    readonly property int _roverActionSmartRTLOrHold:    4
                    readonly property int _roverActionTerminate:         5
                    readonly property int _roverActionLoiterOrHold:      6

                    property Fact _failsafeThrEnable: controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:  controller.getParameterFact(-1, "FS_THR_VALUE")
                    property Fact _failsafeAction:    controller.getParameterFact(-1, "FS_ACTION")
                    property Fact _failsafeTimeout:   controller.getParameterFact(-1, "FS_TIMEOUT")
                    property bool _thrEnabled:        _failsafeThrEnable.rawValue !== _roverThrDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _thrEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeThrEnable.rawValue = _roverThrEnabled
                                } else {
                                    _failsafeThrEnable.rawValue = _roverThrDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:              qsTr("PWM threshold")
                            fact:               _failsafeThrValue
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _thrEnabled
                        }

                        LabelledFactTextField {
                            label:              qsTr("Timeout")
                            fact:               _failsafeTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _thrEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _thrEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Nothing")
                                checked:   _failsafeAction.rawValue === _roverActionNothing
                                onClicked: _failsafeAction.rawValue = _roverActionNothing
                            }

                            QGCRadioButton {
                                text:      qsTr("RTL")
                                checked:   _failsafeAction.rawValue === _roverActionRTL
                                onClicked: _failsafeAction.rawValue = _roverActionRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("Hold")
                                checked:   _failsafeAction.rawValue === _roverActionHold
                                onClicked: _failsafeAction.rawValue = _roverActionHold
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or RTL")
                                checked:   _failsafeAction.rawValue === _roverActionSmartRTLOrRTL
                                onClicked: _failsafeAction.rawValue = _roverActionSmartRTLOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or Hold")
                                checked:   _failsafeAction.rawValue === _roverActionSmartRTLOrHold
                                onClicked: _failsafeAction.rawValue = _roverActionSmartRTLOrHold
                            }

                            QGCRadioButton {
                                text:      qsTr("Terminate")
                                checked:   _failsafeAction.rawValue === _roverActionTerminate
                                onClicked: _failsafeAction.rawValue = _roverActionTerminate
                            }

                            QGCRadioButton {
                                text:      qsTr("Loiter or Hold")
                                checked:   _failsafeAction.rawValue === _roverActionLoiterOrHold
                                onClicked: _failsafeAction.rawValue = _roverActionLoiterOrHold
                            }
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _thrEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _thrEnabled

                            QGCCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Auto mode")
                                checked:            _failsafeThrEnable.rawValue === _roverThrContinueAutoMission

                                onClicked: {
                                    _failsafeThrEnable.rawValue = checked ? _roverThrContinueAutoMission : _roverThrEnabled
                                }
                            }

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("In Hold mode")
                                visible:            _fsOptionsAvailable
                                fact:               _fsOptions
                                bitMask:            1  // Rover FS_OPTIONS bit 0
                            }
                        }
                    }
                }
            }

            // FS_EKF_ACTION: Copter (0=Report,1=Land,2=AltHold,3=LandAlways) vs Rover (0=Disabled,1=Hold,2=ReportOnly)
            Component {
                id: copterEkfFailsafe

                QGCGroupBox {
                    title: qsTr("EKF Failsafe")

                    // Copter FS_EKF_ACTION value constants
                    readonly property int _fsEkfReportOnly:     0
                    readonly property int _fsEkfLandIfNoPos:    1
                    readonly property int _fsEkfAltHoldIfNoPos: 2
                    readonly property int _fsEkfLandAlways:     3

                    property Fact _failsafeEkfAction:   controller.getParameterFact(-1, "FS_EKF_ACTION")
                    property Fact _failsafeEkfThresh:   controller.getParameterFact(-1, "FS_EKF_THRESH")
                    property Fact _failsafeEkfFilt:     controller.getParameterFact(-1, "FS_EKF_FILT", false /* reportMissing */)
                    property bool _ekfEnabled:          _failsafeEkfAction.rawValue !== _fsEkfReportOnly

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _ekfEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeEkfAction.rawValue = _fsEkfLandIfNoPos
                                } else {
                                    _failsafeEkfAction.rawValue = _fsEkfReportOnly
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:            qsTr("Threshold")
                            fact:             _failsafeEkfThresh
                            Layout.fillWidth: true
                            visible:          _ekfEnabled
                        }

                        LabelledFactTextField {
                            label:              qsTr("Filter")
                            fact:               _failsafeEkfFilt
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _ekfEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _ekfEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Land if position required")
                                checked:   _failsafeEkfAction.rawValue === _fsEkfLandIfNoPos
                                onClicked: _failsafeEkfAction.rawValue = _fsEkfLandIfNoPos
                            }

                            QGCRadioButton {
                                text:      qsTr("AltHold if position required")
                                checked:   _failsafeEkfAction.rawValue === _fsEkfAltHoldIfNoPos
                                onClicked: _failsafeEkfAction.rawValue = _fsEkfAltHoldIfNoPos
                            }

                            QGCRadioButton {
                                text:      qsTr("Land from all modes")
                                checked:   _failsafeEkfAction.rawValue === _fsEkfLandAlways
                                onClicked: _failsafeEkfAction.rawValue = _fsEkfLandAlways
                            }
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _fsOptionsAvailable && _ekfEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _fsOptionsAvailable && _ekfEnabled

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("Landing")
                                fact:               _fsOptions
                                bitMask:            _fsOptionContinueLanding
                            }
                        }
                    }
                }
            }

            Component {
                id: roverEkfFailsafe

                QGCGroupBox {
                    title: qsTr("EKF Failsafe")

                    // Rover FS_EKF_ACTION value constants
                    readonly property int _roverEkfDisabled:   0
                    readonly property int _roverEkfHold:       1
                    readonly property int _roverEkfReportOnly: 2

                    property Fact _failsafeEkfAction:   controller.getParameterFact(-1, "FS_EKF_ACTION")
                    property Fact _failsafeEkfThresh:   controller.getParameterFact(-1, "FS_EKF_THRESH")
                    property bool _ekfEnabled:          _failsafeEkfAction.rawValue !== _roverEkfDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _ekfEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeEkfAction.rawValue = _roverEkfHold
                                } else {
                                    _failsafeEkfAction.rawValue = _roverEkfDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:            qsTr("Threshold")
                            fact:             _failsafeEkfThresh
                            Layout.fillWidth: true
                            visible:          _ekfEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _ekfEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Hold")
                                checked:   _failsafeEkfAction.rawValue === _roverEkfHold
                                onClicked: _failsafeEkfAction.rawValue = _roverEkfHold
                            }

                            QGCRadioButton {
                                text:      qsTr("Report only")
                                checked:   _failsafeEkfAction.rawValue === _roverEkfReportOnly
                                onClicked: _failsafeEkfAction.rawValue = _roverEkfReportOnly
                            }
                        }
                    }
                }
            }

            Component {
                id: deadReckoningFailsafeComponent

                QGCGroupBox {
                    title: qsTr("Dead Reckoning Failsafe")

                    property Fact _failsafeDREnable:    controller.getParameterFact(-1, "FS_DR_ENABLE")
                    property Fact _failsafeDRTimeout:   controller.getParameterFact(-1, "FS_DR_TIMEOUT")
                    property bool _drEnabled:           _failsafeDREnable.rawValue !== _fsDrDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Enabled")
                            checked:            _drEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeDREnable.rawValue = _fsDrLand
                                } else {
                                    _failsafeDREnable.rawValue = _fsDrDisabled
                                }
                            }
                        }

                        LabelledFactTextField {
                            label:              qsTr("Timeout")
                            fact:               _failsafeDRTimeout
                            textFieldShowUnits: true
                            Layout.fillWidth:   true
                            visible:            _drEnabled
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _drEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Land")
                                checked:   _failsafeDREnable.rawValue === _fsDrLand
                                onClicked: _failsafeDREnable.rawValue = _fsDrLand
                            }

                            QGCRadioButton {
                                text:      qsTr("RTL")
                                checked:   _failsafeDREnable.rawValue === _fsDrRTL
                                onClicked: _failsafeDREnable.rawValue = _fsDrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or RTL")
                                checked:   _failsafeDREnable.rawValue === _fsDrSmartRTLOrRTL
                                onClicked: _failsafeDREnable.rawValue = _fsDrSmartRTLOrRTL
                            }

                            QGCRadioButton {
                                text:      qsTr("SmartRTL or Land")
                                checked:   _failsafeDREnable.rawValue === _fsDrSmartRTLOrLand
                                onClicked: _failsafeDREnable.rawValue = _fsDrSmartRTLOrLand
                            }

                            QGCRadioButton {
                                text:      qsTr("Auto Land/Return or RTL")
                                checked:   _failsafeDREnable.rawValue === _fsDrAutoDoLandOrRTL
                                onClicked: _failsafeDREnable.rawValue = _fsDrAutoDoLandOrRTL
                            }
                        }

                        QGCLabel {
                            text:    qsTr("Ignore failsafe if:")
                            visible: _fsOptionsAvailable && _drEnabled
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.leftMargin:  ScreenTools.defaultFontPixelWidth * 2
                            spacing:            _margins
                            visible:            _fsOptionsAvailable && _drEnabled

                            FactBitMaskCheckBoxSlider {
                                Layout.fillWidth:   true
                                text:               qsTr("Landing")
                                fact:               _fsOptions
                                bitMask:            _fsOptionContinueLanding
                            }
                        }
                    }
                }
            }

            // FS_CRASH_CHECK: Copter (0=Disabled,1=Enabled) vs Rover (0=Disabled,1=Hold,2=HoldAndDisarm)
            Component {
                id: copterGeneralFS

                QGCGroupBox {
                    title: qsTr("Other Failsafe Options")

                    property Fact _failsafeCrashCheck:  controller.getParameterFact(-1, "FS_CRASH_CHECK")
                    property Fact _failsafeVibeEnable:  controller.getParameterFact(-1, "FS_VIBE_ENABLE", false /* reportMissing */)

                    ColumnLayout {
                        spacing: _margins

                        FactCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Crash check failsafe")
                            fact:               _failsafeCrashCheck
                        }

                        FactCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Vibration failsafe")
                            fact:               _failsafeVibeEnable
                        }

                        FactBitMaskCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Release gripper on any failsafe")
                            visible:            _fsOptionsAvailable
                            fact:               _fsOptions
                            bitMask:            _fsOptionReleaseGripper
                        }
                    }
                }
            }

            Component {
                id: roverGeneralFS

                QGCGroupBox {
                    title: qsTr("Other Failsafe Options")

                    // Rover FS_CRASH_CHECK value constants
                    readonly property int _roverCrashDisabled:       0
                    readonly property int _roverCrashHold:           1
                    readonly property int _roverCrashHoldAndDisarm:  2

                    property Fact _failsafeCrashCheck:  controller.getParameterFact(-1, "FS_CRASH_CHECK")
                    property bool _crashEnabled:        _failsafeCrashCheck.rawValue !== _roverCrashDisabled

                    ColumnLayout {
                        spacing: _margins

                        QGCCheckBoxSlider {
                            Layout.fillWidth:   true
                            text:               qsTr("Crash check failsafe")
                            checked:            _crashEnabled

                            onClicked: {
                                if (checked) {
                                    _failsafeCrashCheck.rawValue = _roverCrashHold
                                } else {
                                    _failsafeCrashCheck.rawValue = _roverCrashDisabled
                                }
                            }
                        }

                        ColumnLayout {
                            spacing: 0
                            visible: _crashEnabled

                            QGCLabel { text: qsTr("Action:") }

                            QGCRadioButton {
                                text:      qsTr("Hold")
                                checked:   _failsafeCrashCheck.rawValue === _roverCrashHold
                                onClicked: _failsafeCrashCheck.rawValue = _roverCrashHold
                            }

                            QGCRadioButton {
                                text:      qsTr("Hold and Disarm")
                                checked:   _failsafeCrashCheck.rawValue === _roverCrashHoldAndDisarm
                                onClicked: _failsafeCrashCheck.rawValue = _roverCrashHoldAndDisarm
                            }
                        }
                    }
                }
            }

            Component {
                id: copterGeoFence

                QGCGroupBox {
                    title: qsTr("GeoFence")

                    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
                    property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
                    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
                    property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
                    property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
                    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

                    readonly property int _maxAltitudeFenceBitMask: 1
                    readonly property int _circleFenceBitMask:      2
                    readonly property int _polygonFenceBitMask:     4

                    ColumnLayout {
                        spacing: _margins

                        FactCheckBoxSlider {
                            id:               enabledSlider
                            Layout.fillWidth: true
                            text:             qsTr("Enabled")
                            fact:             _fenceEnable
                        }

                        ColumnLayout {
                            visible: enabledSlider.checked

                            RowLayout {
                                QGCCheckBox {
                                    text:    qsTr("Maximum Altitude")
                                    checked: _fenceType.rawValue & _maxAltitudeFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _maxAltitudeFenceBitMask
                                        } else {
                                            _fenceType.rawValue &= ~_maxAltitudeFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact: _fenceAltMax
                                }
                            }

                            RowLayout {
                                QGCCheckBox {
                                    text:    qsTr("Circle centered on Home")
                                    checked: _fenceType.rawValue & _circleFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _circleFenceBitMask
                                        } else {
                                            _fenceType.rawValue &= ~_circleFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact:      _fenceRadius
                                    showUnits: true
                                }
                            }

                            QGCCheckBox {
                                text:    qsTr("Inclusion/Exclusion Circles+Polygons")
                                checked: _fenceType.rawValue & _polygonFenceBitMask

                                onClicked: {
                                    if (checked) {
                                        _fenceType.rawValue |= _polygonFenceBitMask
                                    } else {
                                        _fenceType.rawValue &= ~_polygonFenceBitMask
                                    }
                                }
                            }
                        }

                        ColumnLayout {
                            visible: enabledSlider.checked

                            LabelledFactComboBox {
                                label:            qsTr("Breach action")
                                fact:             _fenceAction
                                comboBoxPreferredWidth: _comboWidth
                                Layout.fillWidth: true
                            }

                            LabelledFactTextField {
                                label:            qsTr("Fence margin")
                                fact:             _fenceMargin
                                Layout.fillWidth: true
                            }
                        }
                    }
                }
            }
        }
    }
}
