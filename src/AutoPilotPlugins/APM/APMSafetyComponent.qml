/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.Controls
import QGroundControl.ScreenTools

SetupPage {
    id:             safetyPage
    pageComponent:  safetyPageComponent

    Component {
        id: safetyPageComponent

        Flow {
            id:         flowLayout
            width:      availableWidth
            spacing:    _margins

            FactPanelController { id: controller; }

            QGCPalette { id: ggcPal; colorGroupEnabled: true }

            property Fact _batt1Monitor:                    controller.getParameterFact(-1, "BATT_MONITOR")
            property Fact _batt2Monitor:                    controller.getParameterFact(-1, "BATT2_MONITOR", false /* reportMissing */)
            property bool _batt2MonitorAvailable:           controller.parameterExists(-1, "BATT2_MONITOR")
            property bool _batt1MonitorEnabled:             _batt1Monitor.rawValue !== 0
            property bool _batt2MonitorEnabled:             _batt2MonitorAvailable ? _batt2Monitor.rawValue !== 0 : false
            property bool _batt1ParamsAvailable:            controller.parameterExists(-1, "BATT_CAPACITY")
            property bool _batt2ParamsAvailable:            controller.parameterExists(-1, "BATT2_CAPACITY")

            property Fact _failsafeBatt1LowAct:             controller.getParameterFact(-1, "BATT_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2LowAct:             controller.getParameterFact(-1, "BATT2_FS_LOW_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1CritAct:            controller.getParameterFact(-1, "BATT_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt2CritAct:            controller.getParameterFact(-1, "BATT2_FS_CRT_ACT", false /* reportMissing */)
            property Fact _failsafeBatt1LowMah:             controller.getParameterFact(-1, "BATT_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2LowMah:             controller.getParameterFact(-1, "BATT2_LOW_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1CritMah:            controller.getParameterFact(-1, "BATT_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt2CritMah:            controller.getParameterFact(-1, "BATT2_CRT_MAH", false /* reportMissing */)
            property Fact _failsafeBatt1LowVoltage:         controller.getParameterFact(-1, "BATT_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2LowVoltage:         controller.getParameterFact(-1, "BATT2_LOW_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt1CritVoltage:        controller.getParameterFact(-1, "BATT_CRT_VOLT", false /* reportMissing */)
            property Fact _failsafeBatt2CritVoltage:        controller.getParameterFact(-1, "BATT2_CRT_VOLT", false /* reportMissing */)

            property Fact _armingCheck: controller.getParameterFact(-1, "ARMING_CHECK")

            property real _margins:         ScreenTools.defaultFontPixelHeight
            property real _innerMargin:     _margins / 2
            property bool _showIcon:        !ScreenTools.isTinyScreen
            property bool _roverFirmware:   controller.parameterExists(-1, "MODE1") // This catches all usage of ArduRover firmware vehicle types: Rover, Boat...


            property string _restartRequired: qsTr("Requires vehicle reboot")

            Component {
                id: batteryFailsafeComponent

                Column {
                    spacing: _margins

                    GridLayout {
                        id:             gridLayout
                        columnSpacing:  _margins
                        rowSpacing:     _margins
                        columns:        2
                        QGCLabel { text: qsTr("Low action:") }
                        FactComboBox {
                            fact:               failsafeBattLowAct
                            indexModel:         false
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Critical action:") }
                        FactComboBox {
                            fact:               failsafeBattCritAct
                            indexModel:         false
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Low voltage threshold:") }
                        FactTextField {
                            fact:               failsafeBattLowVoltage
                            showUnits:          true
                            Layout.fillWidth:   true
                        }


                        QGCLabel { text: qsTr("Critical voltage threshold:") }
                        FactTextField {
                            fact:               failsafeBattCritVoltage
                            showUnits:          true
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Low mAh threshold:") }
                        FactTextField {
                            fact:               failsafeBattLowMah
                            showUnits:          true
                            Layout.fillWidth:   true
                        }

                        QGCLabel { text: qsTr("Critical mAh threshold:") }
                        FactTextField {
                            fact:               failsafeBattCritMah
                            showUnits:          true
                            Layout.fillWidth:   true
                        }
                    } // GridLayout
                } // Column
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

            Column {
                spacing: _margins / 2
                visible: _batt1MonitorEnabled

                QGCLabel {
                    text:       qsTr("Battery1 Failsafe Triggers")
                    font.bold:   true
                }

                Rectangle {
                    width:  battery1FailsafeLoader.x + battery1FailsafeLoader.width + _margins
                    height: battery1FailsafeLoader.y + battery1FailsafeLoader.height + _margins
                    color:  ggcPal.windowShade

                    Loader {
                        id:                 battery1FailsafeLoader
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        sourceComponent:    _batt1ParamsAvailable ? batteryFailsafeComponent : restartRequiredComponent

                        property Fact battMonitor:              _batt1Monitor
                        property bool battParamsAvailable:      _batt1ParamsAvailable
                        property Fact failsafeBattLowAct:       _failsafeBatt1LowAct
                        property Fact failsafeBattCritAct:      _failsafeBatt1CritAct
                        property Fact failsafeBattLowMah:       _failsafeBatt1LowMah
                        property Fact failsafeBattCritMah:      _failsafeBatt1CritMah
                        property Fact failsafeBattLowVoltage:   _failsafeBatt1LowVoltage
                        property Fact failsafeBattCritVoltage:  _failsafeBatt1CritVoltage
                    }
                } // Rectangle
            } // Column - Battery Failsafe Settings


            Column {
                spacing: _margins / 2
                visible: _batt2MonitorEnabled

                QGCLabel {
                    text:       qsTr("Battery2 Failsafe Triggers")
                    font.bold:   true
                }

                Rectangle {
                    width:  battery2FailsafeLoader.x + battery2FailsafeLoader.width + _margins
                    height: battery2FailsafeLoader.y + battery2FailsafeLoader.height + _margins
                    color:  ggcPal.windowShade

                    Loader {
                        id:                 battery2FailsafeLoader
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        sourceComponent:    _batt2ParamsAvailable ? batteryFailsafeComponent : restartRequiredComponent

                        property Fact battMonitor:              _batt2Monitor
                        property bool battParamsAvailable:      _batt2ParamsAvailable
                        property Fact failsafeBattLowAct:       _failsafeBatt2LowAct
                        property Fact failsafeBattCritAct:      _failsafeBatt2CritAct
                        property Fact failsafeBattLowMah:       _failsafeBatt2LowMah
                        property Fact failsafeBattCritMah:      _failsafeBatt2CritMah
                        property Fact failsafeBattLowVoltage:   _failsafeBatt2LowVoltage
                        property Fact failsafeBattCritVoltage:  _failsafeBatt2CritVoltage
                    }
                } // Rectangle
            } // Column - Battery Failsafe Settings

            Component {
                id: planeGeneralFS

                Column {
                    spacing: _margins / 2

                    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "THR_FAILSAFE")
                    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "THR_FS_VALUE")
                    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABL")

                    QGCLabel {
                        text:       qsTr("Failsafe Triggers")
                        font.bold:   true
                    }

                    Rectangle {
                        width:  fsColumn.x + fsColumn.width + _margins
                        height: fsColumn.y + fsColumn.height + _margins
                        color:  qgcPal.windowShade

                        ColumnLayout {
                            id:                 fsColumn
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top

                            RowLayout {
                                QGCCheckBox {
                                    id:                 throttleEnableCheckBox
                                    text:               qsTr("Throttle PWM threshold:")
                                    checked:            _failsafeThrEnable.value === 1

                                    onClicked: _failsafeThrEnable.value = (checked ? 1 : 0)
                                }

                                FactTextField {
                                    fact:               _failsafeThrValue
                                    showUnits:          true
                                    enabled:            throttleEnableCheckBox.checked
                                }
                            }

                            QGCCheckBox {
                                text:       qsTr("GCS failsafe")
                                checked:    _failsafeGCSEnable.value != 0
                                onClicked:  _failsafeGCSEnable.value = checked ? 1 : 0
                            }
                        }
                    } // Rectangle - Failsafe trigger settings
                } // Column - Failsafe trigger settings
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeGeneralFS : undefined
            }

            Component {
                id: roverGeneralFS

                Column {
                    spacing: _margins / 2

                    property Fact _failsafeGCSEnable:   controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _failsafeThrEnable:   controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:    controller.getParameterFact(-1, "FS_THR_VALUE")
                    property Fact _failsafeAction:      controller.getParameterFact(-1, "FS_ACTION")
                    property Fact _failsafeCrashCheck:  controller.getParameterFact(-1, "FS_CRASH_CHECK")

                    QGCLabel {
                        id:         failsafeLabel
                        text:       qsTr("Failsafe Triggers")
                        font.bold:   true
                    }

                    Rectangle {
                        id:     failsafeSettings
                        width:  fsGrid.x + fsGrid.width + _margins
                        height: fsGrid.y + fsGrid.height + _margins
                        color:  ggcPal.windowShade

                        GridLayout {
                            id:                 fsGrid
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            columns:            2

                            QGCLabel { text: qsTr("Ground Station failsafe:") }
                            FactComboBox {
                                Layout.fillWidth:   true
                                fact:               _failsafeGCSEnable
                                indexModel:         false
                            }

                            QGCLabel { text: qsTr("Throttle failsafe:") }
                            FactComboBox {
                                Layout.fillWidth:   true
                                fact:               _failsafeThrEnable
                                indexModel:         false
                            }

                            QGCLabel { text: qsTr("PWM threshold:") }
                            FactTextField {
                                Layout.fillWidth:   true
                                fact:               _failsafeThrValue
                            }

                            QGCLabel { text: qsTr("Failsafe Crash Check:") }
                            FactComboBox {
                                Layout.fillWidth:   true
                                fact:               _failsafeCrashCheck
                                indexModel:         false
                            }
                        }
                    } // Rectangle - Failsafe Settings
                } // Column - Failsafe Settings
            }

            Loader {
                sourceComponent: _roverFirmware ? roverGeneralFS : undefined
            }

            Component {
                id: copterGeneralFS

                Column {
                    spacing: _margins / 2

                    property Fact _failsafeGCSEnable:               controller.getParameterFact(-1, "FS_GCS_ENABLE")
                    property Fact _failsafeBattLowAct:              controller.getParameterFact(-1, "r.BATT_FS_LOW_ACT", false /* reportMissing */)
                    property Fact _failsafeBattMah:                 controller.getParameterFact(-1, "r.BATT_LOW_MAH", false /* reportMissing */)
                    property Fact _failsafeBattVoltage:             controller.getParameterFact(-1, "r.BATT_LOW_VOLT", false /* reportMissing */)
                    property Fact _failsafeThrEnable:               controller.getParameterFact(-1, "FS_THR_ENABLE")
                    property Fact _failsafeThrValue:                controller.getParameterFact(-1, "FS_THR_VALUE")

                    QGCLabel {
                        text:       qsTr("General Failsafe Triggers")
                        font.bold:   true
                    }

                    Rectangle {
                        width:  generalFailsafeColumn.x + generalFailsafeColumn.width + _margins
                        height: generalFailsafeColumn.y + generalFailsafeColumn.height + _margins
                        color:  ggcPal.windowShade

                        Column {
                            id:                 generalFailsafeColumn
                            anchors.margins:    _margins
                            anchors.top:        parent.top
                            anchors.left:       parent.left
                            spacing:            _margins

                            GridLayout {
                                columnSpacing:  _margins
                                rowSpacing:     _margins
                                columns:        2

                                QGCLabel { text: qsTr("Ground Station failsafe:") }
                                FactComboBox {
                                    fact:               _failsafeGCSEnable
                                    indexModel:         false
                                    Layout.fillWidth:   true
                                }

                                QGCLabel { text: qsTr("Throttle failsafe:") }
                                QGCComboBox {
                                    model:              [qsTr("Disabled"), qsTr("Always RTL"),
                                        qsTr("Continue with Mission in Auto Mode"), qsTr("Always Land")]
                                    currentIndex:       _failsafeThrEnable.value
                                    Layout.fillWidth:   true

                                    onActivated: (index) => { _failsafeThrEnable.value = index }
                                }

                                QGCLabel { text: qsTr("PWM threshold:") }
                                FactTextField {
                                    fact:               _failsafeThrValue
                                    showUnits:          true
                                    Layout.fillWidth:   true
                                }
                            } // GridLayout
                        } // Column
                    } // Rectangle - Failsafe Settings
                } // Column - General Failsafe Settings
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeneralFS : undefined
            }

            Component {
                id: copterGeoFence

                Column {
                    spacing: _margins / 2

                    property Fact _fenceAction: controller.getParameterFact(-1, "FENCE_ACTION")
                    property Fact _fenceAltMax: controller.getParameterFact(-1, "FENCE_ALT_MAX")
                    property Fact _fenceEnable: controller.getParameterFact(-1, "FENCE_ENABLE")
                    property Fact _fenceMargin: controller.getParameterFact(-1, "FENCE_MARGIN")
                    property Fact _fenceRadius: controller.getParameterFact(-1, "FENCE_RADIUS")
                    property Fact _fenceType:   controller.getParameterFact(-1, "FENCE_TYPE")

                    readonly property int _maxAltitudeFenceBitMask: 1
                    readonly property int _circleFenceBitMask:      2
                    readonly property int _polygonFenceBitMask:     4

                    QGCLabel {
                        text:           qsTr("GeoFence")
                        font.bold:      true
                    }

                    Rectangle {
                        width:  mainLayout.width + (_margins * 2)
                        height: mainLayout.height + (_margins * 2)
                        color:  ggcPal.windowShade

                        ColumnLayout {
                            id:         mainLayout
                            x:          _margins
                            y:          _margins
                            spacing:    ScreenTools.defaultFontPixellHeight / 2

                            FactCheckBox {
                                id:     enabledCheckBox
                                text:   qsTr("Enabled")
                                fact:   _fenceEnable
                            }

                            GridLayout {
                                columns:    2
                                enabled:    enabledCheckBox.checked

                                QGCCheckBox {
                                    text:       qsTr("Maximum Altitude")
                                    checked:    _fenceType.rawValue & _maxAltitudeFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _maxAltitudeFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_maxAltitudeFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact: _fenceAltMax
                                }

                                QGCCheckBox {
                                    text:       qsTr("Circle centered on Home")
                                    checked:    _fenceType.rawValue & _circleFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _circleFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_circleFenceBitMask
                                        }
                                    }
                                }

                                FactTextField {
                                    fact:       _fenceRadius
                                    showUnits:  true
                                }

                                QGCCheckBox {
                                    text:       qsTr("Inclusion/Exclusion Circles+Polygons")
                                    checked:    _fenceType.rawValue & _polygonFenceBitMask

                                    onClicked: {
                                        if (checked) {
                                            _fenceType.rawValue |= _polygonFenceBitMask
                                        } else {
                                            _fenceType.value &= ~_polygonFenceBitMask
                                        }
                                    }
                                }

                                Item {
                                    height: 1
                                    width:  1
                                }
                            } // GridLayout

                            Item {
                                height: 1
                                width:  1
                            }

                            GridLayout {
                                columns: 2
                                enabled: enabledCheckBox.checked

                                QGCLabel {
                                    text: qsTr("Breach action")
                                }

                                FactComboBox {
                                    sizeToContents: true
                                    fact:           _fenceAction
                                }

                                QGCLabel {
                                    text: qsTr("Fence margin")
                                }

                                FactTextField {
                                    fact: _fenceMargin
                                }
                            }
                        }
                    } // Rectangle - GeoFence Settings
                } // Column - GeoFence Settings
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterGeoFence : undefined
            }

            Component {
                id: copterRTL

                Column {
                    spacing: _margins / 2

                    property Fact _landSpeedFact:   controller.getParameterFact(-1, "LAND_SPEED")
                    property Fact _rtlAltFact:      controller.getParameterFact(-1, "RTL_ALT")
                    property Fact _rtlLoitTimeFact: controller.getParameterFact(-1, "RTL_LOIT_TIME")
                    property Fact _rtlAltFinalFact: controller.getParameterFact(-1, "RTL_ALT_FINAL")

                    QGCLabel {
                        id:             rtlLabel
                        text:           qsTr("Return to Launch")
                        font.bold:      true
                    }

                    Rectangle {
                        id:     rtlSettings
                        width:  landSpeedField.x + landSpeedField.width + _margins
                        height: landSpeedField.y + landSpeedField.height + _margins
                        color:  ggcPal.windowShade

                        QGCColoredImage {
                            id:                 icon
                            visible:            _showIcon
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            height:             ScreenTools.defaultFontPixelWidth * 20
                            width:              ScreenTools.defaultFontPixelWidth * 20
                            color:              ggcPal.text
                            sourceSize.width:   width
                            mipmap:             true
                            fillMode:           Image.PreserveAspectFit
                            source:             "/qmlimages/ReturnToHomeAltitude.svg"
                        }

                        QGCRadioButton {
                            id:                 returnAtCurrentRadio
                            anchors.margins:    _innerMargin
                            anchors.left:       _showIcon ? icon.right : parent.left
                            anchors.top:        parent.top
                            text:               qsTr("Return at current altitude")
                            checked:            _rtlAltFact.value == 0

                            onClicked: _rtlAltFact.value = 0
                        }

                        QGCRadioButton {
                            id:                 returnAltRadio
                            anchors.topMargin:  _innerMargin
                            anchors.top:        returnAtCurrentRadio.bottom
                            anchors.left:       returnAtCurrentRadio.left
                            text:               qsTr("Return at specified altitude:")
                            checked:            _rtlAltFact.value != 0

                            onClicked: _rtlAltFact.value = 1500
                        }

                        FactTextField {
                            id:                 rltAltField
                            anchors.leftMargin: _margins
                            anchors.left:       returnAltRadio.right
                            anchors.baseline:   returnAltRadio.baseline
                            fact:               _rtlAltFact
                            showUnits:          true
                            enabled:            returnAltRadio.checked
                        }

                        QGCCheckBox {
                            id:                 homeLoiterCheckbox
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   landDelayField.baseline
                            checked:            _rtlLoitTimeFact.value > 0
                            text:               qsTr("Loiter above Home for:")

                            onClicked: _rtlLoitTimeFact.value = (checked ? 60 : 0)
                        }

                        FactTextField {
                            id:                 landDelayField
                            anchors.topMargin:  _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        rltAltField.bottom
                            fact:               _rtlLoitTimeFact
                            showUnits:          true
                            enabled:            homeLoiterCheckbox.checked === true
                        }

                        QGCLabel {
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   rltAltFinalField.baseline
                            text:               qsTr("Final land stage altitude:")
                        }

                        FactTextField {
                            id:                 rltAltFinalField
                            anchors.topMargin:  _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        landDelayField.bottom
                            fact:               _rtlAltFinalFact
                            showUnits:          true
                        }

                        QGCLabel {
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.baseline:   landSpeedField.baseline
                            text:               qsTr("Final land stage descent speed:")
                        }

                        FactTextField {
                            id:                 landSpeedField
                            anchors.topMargin: _innerMargin
                            anchors.left:       rltAltField.left
                            anchors.top:        rltAltFinalField.bottom
                            fact:               _landSpeedFact
                            showUnits:          true
                        }
                    } // Rectangle - RTL Settings
                } // Column - RTL Settings
            }

            Loader {
                sourceComponent: controller.vehicle.multiRotor ? copterRTL : undefined
            }

            Component {
                id: planeRTL

                Column {
                    spacing: _margins / 2

                    property Fact _rtlAltFact: {
                        if (controller.firmwareMajorVersion < 4 || (controller.firmwareMajorVersion === 4 && controller.firmwareMinorVersion < 5)) {
                            return controller.getParameterFact(-1, "ALT_HOLD_RTL")
                        } else {
                            return controller.getParameterFact(-1, "RTL_ALTITUDE")
                        }
                    }

                    QGCLabel {
                        text:           qsTr("Return to Launch")
                        font.bold:      true
                    }

                    Rectangle {
                        width:  rltAltField.x + rltAltField.width + _margins
                        height: rltAltField.y + rltAltField.height + _margins
                        color:  qgcPal.windowShade

                        QGCRadioButton {
                            id:                 returnAtCurrentRadio
                            anchors.margins:    _margins
                            anchors.left:       parent.left
                            anchors.top:        parent.top
                            text:               qsTr("Return at current altitude")
                            checked:            _rtlAltFact.value < 0

                            onClicked: _rtlAltFact.value = -1
                        }

                        QGCRadioButton {
                            id:                 returnAltRadio
                            anchors.topMargin:  _margins / 2
                            anchors.left:       returnAtCurrentRadio.left
                            anchors.top:        returnAtCurrentRadio.bottom
                            text:               qsTr("Return at specified altitude:")
                            checked:            _rtlAltFact.value >= 0

                            onClicked: _rtlAltFact.value = 10000
                        }

                        FactTextField {
                            id:                 rltAltField
                            anchors.leftMargin: _margins
                            anchors.left:       returnAltRadio.right
                            anchors.baseline:   returnAltRadio.baseline
                            fact:               _rtlAltFact
                            showUnits:          true
                            enabled:            returnAltRadio.checked
                        }
                    } // Rectangle - RTL Settings
                } // Column - RTL Settings
            }

            Loader {
                sourceComponent: controller.vehicle.fixedWing ? planeRTL : undefined
            }

            Column {
                spacing: _margins / 2

                QGCLabel {
                    text:           qsTr("Arming Checks")
                    font.bold:      true
                }

                Rectangle {
                    width:  flowLayout.width
                    height: armingCheckInnerColumn.height + (_margins * 2)
                    color:  ggcPal.windowShade

                    Column {
                        id:                 armingCheckInnerColumn
                        anchors.margins:    _margins
                        anchors.top:        parent.top
                        anchors.left:       parent.left
                        anchors.right:      parent.right
                        spacing: _margins

                        FactBitmask {
                            id:                 armingCheckBitmask
                            anchors.left:       parent.left
                            anchors.right:      parent.right
                            firstEntryIsAll:    true
                            fact:               _armingCheck
                        }

                        QGCLabel {
                            id:             armingCheckWarning
                            anchors.left:   parent.left
                            anchors.right:  parent.right
                            wrapMode:       Text.WordWrap
                            color:          qgcPal.warningText
                            text:            qsTr("Warning: Turning off arming checks can lead to loss of Vehicle control.")
                            visible:        _armingCheck.value != 1
                        }
                    }
                } // Rectangle - Arming checks
            } // Column - Arming Checks
        } // Flow
    } // Component - safetyPageComponent
} // SetupView
