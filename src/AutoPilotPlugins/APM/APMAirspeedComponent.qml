import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SetupPage {
    id: airspeedPage

    pageComponent: airspeedPageComponent

    Component {
        id: airspeedPageComponent

        ColumnLayout {
            id: mainLayout
            spacing: ScreenTools.defaultFontPixelHeight

            // Sensor type constants
            readonly property int _typeNone: 0
            readonly property int _typeAnalog: 2
            readonly property int _typeSDP3X: 7
            readonly property int _typeDroneCAN: 8
            readonly property int _typeDLVR: 9
            readonly property int _typeASP5033: 10
            readonly property int _typeMS5525_Addr1: 11
            readonly property int _typeMS5525_Addr2: 12
            readonly property int _typeNMEA: 13
            readonly property int _typeMSP: 14
            readonly property int _typeExtAHRS: 16
            readonly property int _typeSDPx1: 17
            readonly property int _typeSDPx2: 18
            readonly property int _typeSDPx3: 19

            readonly property real _margins: ScreenTools.defaultFontPixelWidth / 2
            readonly property real _comboWidth: ScreenTools.defaultFontPixelWidth * 30
            readonly property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 15

            // Primary sensor
            readonly property bool _arspdTypeAvailable: controller.parameterExists(-1, "ARSPD_TYPE")
            readonly property Fact _arspdType: _arspdTypeAvailable ? controller.getParameterFact(-1, "ARSPD_TYPE") : null
            readonly property int _sensorType: _arspdType ? _arspdType.rawValue : 0
            readonly property bool _sensorEnabled: _arspdTypeAvailable && _sensorType !== _typeNone

            // Second sensor
            readonly property bool _arspd2TypeAvailable: controller.parameterExists(-1, "ARSPD2_TYPE")
            readonly property Fact _arspd2Type: _arspd2TypeAvailable ? controller.getParameterFact(-1, "ARSPD2_TYPE") : null
            readonly property int _sensor2Type: _arspd2Type ? _arspd2Type.rawValue : 0
            readonly property bool _sensor2Enabled: _arspd2TypeAvailable && _sensor2Type !== _typeNone

            readonly property var _nonI2CTypes:  [_typeNone, _typeAnalog, _typeDroneCAN, _typeNMEA, _typeMSP, _typeExtAHRS]
            readonly property var _nonPitotTypes: [_typeNone, _typeNMEA, _typeMSP, _typeExtAHRS]

            readonly property var _psiRangeTypes: [_typeSDP3X, _typeDLVR, _typeASP5033, _typeMS5525_Addr1, _typeMS5525_Addr2, _typeSDPx1, _typeSDPx2, _typeSDPx3]

            function _isAnalog(sType) { return sType === _typeAnalog }
            function _isI2C(sType) { return _nonI2CTypes.indexOf(sType) === -1 }
            function _isPitot(sType) { return _nonPitotTypes.indexOf(sType) === -1 }
            function _hasPsiRange(sType) { return _psiRangeTypes.indexOf(sType) !== -1 }

            FactPanelController { id: controller }
            QGCPalette { id: qgcPal; colorGroupEnabled: true }

            QGCTabBar {
                id: tabBar
                Layout.fillWidth: true

                QGCTabButton {
                    text: qsTr("Basic")
                    checked: true
                }
                QGCTabButton {
                    text: qsTr("Advanced")
                }
            }

            // ─── Basic Tab ───

            Flow {
                Layout.maximumWidth: availableWidth
                spacing: ScreenTools.defaultFontPixelHeight
                visible: tabBar.currentIndex === 0

                // ─── Shared Basic Sensor Configuration Component ───

                Component {
                    id: basicSensorSettingsComponent

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactComboBox {
                            label: qsTr("Sensor type")
                            fact: sensorTypeFact
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                        }

                        ColumnLayout {
                            spacing: _margins
                            visible: sensorTypeFact.rawValue !== _typeNone

                            LabelledFactComboBox {
                                label: qsTr("Use airspeed")
                                fact: controller.getParameterFact(-1, paramPrefix + "USE", false)
                                indexModel: false
                                comboBoxPreferredWidth: _comboWidth
                                Layout.fillWidth: true
                                visible: controller.parameterExists(-1, paramPrefix + "USE")
                            }

                            LabelledFactTextField {
                                label: qsTr("Airspeed ratio")
                                fact: controller.getParameterFact(-1, paramPrefix + "RATIO", false)
                                textFieldPreferredWidth: _textFieldWidth
                                textFieldShowUnits: true
                                Layout.fillWidth: true
                                visible: _isPitot(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "RATIO")
                            }

                            FactCheckBox {
                                text: qsTr("Auto calibrate ratio in flight")
                                fact: controller.getParameterFact(-1, paramPrefix + "AUTOCAL", false)
                                visible: _isPitot(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "AUTOCAL")
                            }
                        }
                    }
                }

                // ─── Primary Sensor Configuration ───

                QGCGroupBox {
                    title: qsTr("Primary Airspeed Sensor")
                    visible: _arspdTypeAvailable

                    Loader {
                        sourceComponent: basicSensorSettingsComponent
                        property string paramPrefix: "ARSPD_"
                        property Fact sensorTypeFact: _arspdType
                    }
                }

                // ─── Second Sensor Configuration ───

                QGCGroupBox {
                    title: qsTr("Second Airspeed Sensor")
                    visible: _arspd2TypeAvailable

                    Loader {
                        sourceComponent: basicSensorSettingsComponent
                        property string paramPrefix: "ARSPD2_"
                        property Fact sensorTypeFact: _arspd2Type
                    }
                }

                // ─── Multi-Sensor Options ───

                QGCGroupBox {
                    title: qsTr("Multi-Sensor Options")
                    visible: _sensorEnabled && _sensor2Enabled

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactComboBox {
                            label: qsTr("Primary sensor")
                            fact: controller.getParameterFact(-1, "ARSPD_PRIMARY", false)
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "ARSPD_PRIMARY")
                        }
                    }
                }

                // ─── Airspeed Limits ───

                QGCGroupBox {
                    title: qsTr("Airspeed Limits")
                    visible: _sensorEnabled && controller.parameterExists(-1, "AIRSPEED_CRUISE")

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactTextField {
                            label: qsTr("Cruise airspeed")
                            fact: controller.getParameterFact(-1, "AIRSPEED_CRUISE", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "AIRSPEED_CRUISE")
                        }

                        LabelledFactTextField {
                            label: qsTr("Minimum airspeed")
                            fact: controller.getParameterFact(-1, "AIRSPEED_MIN", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "AIRSPEED_MIN")
                        }

                        LabelledFactTextField {
                            label: qsTr("Maximum airspeed")
                            fact: controller.getParameterFact(-1, "AIRSPEED_MAX", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "AIRSPEED_MAX")
                        }

                        LabelledFactTextField {
                            label: qsTr("Stall airspeed")
                            fact: controller.getParameterFact(-1, "AIRSPEED_STALL", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "AIRSPEED_STALL")
                        }
                    }
                }
            }

            // ─── Advanced Tab ───

            Flow {
                Layout.maximumWidth: availableWidth
                spacing: ScreenTools.defaultFontPixelHeight
                visible: tabBar.currentIndex === 1

                // ─── Shared Advanced Sensor Configuration Component ───

                Component {
                    id: advancedSensorSettingsComponent

                    ColumnLayout {
                        spacing: _margins

                        QGCLabel {
                            text: sensorLabel
                            font.bold: true
                        }

                        LabelledFactTextField {
                            label: qsTr("Airspeed offset")
                            fact: controller.getParameterFact(-1, paramPrefix + "OFFSET", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: _isPitot(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "OFFSET")
                        }

                        LabelledFactComboBox {
                            label: qsTr("Skip calibration")
                            fact: controller.getParameterFact(-1, paramPrefix + "SKIP_CAL", false)
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible: _isPitot(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "SKIP_CAL")
                        }

                        LabelledFactComboBox {
                            label: qsTr("Pitot tube order")
                            fact: controller.getParameterFact(-1, paramPrefix + "TUBE_ORDR", false)
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible: _isPitot(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "TUBE_ORDR")
                        }

                        LabelledFactComboBox {
                            label: qsTr("Analog pin")
                            fact: controller.getParameterFact(-1, paramPrefix + "PIN", false)
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible: _isAnalog(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "PIN")
                        }

                        LabelledFactComboBox {
                            label: qsTr("I2C bus")
                            fact: controller.getParameterFact(-1, paramPrefix + "BUS", false)
                            indexModel: false
                            comboBoxPreferredWidth: _comboWidth
                            Layout.fillWidth: true
                            visible: _isI2C(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "BUS")
                        }

                        LabelledFactTextField {
                            label: qsTr("PSI range")
                            fact: controller.getParameterFact(-1, paramPrefix + "PSI_RANGE", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: _hasPsiRange(sensorTypeFact.rawValue) && controller.parameterExists(-1, paramPrefix + "PSI_RANGE")
                        }
                    }
                }

                // ─── Sensor Advanced Settings ───

                QGCGroupBox {
                    title: qsTr("Sensor Settings")
                    visible: _sensorEnabled

                    ColumnLayout {
                        spacing: _margins

                        Loader {
                            sourceComponent: advancedSensorSettingsComponent
                            active: _sensorEnabled
                            property string paramPrefix: "ARSPD_"
                            property Fact sensorTypeFact: _arspdType
                            property string sensorLabel: qsTr("Primary Sensor")
                        }

                        Loader {
                            sourceComponent: advancedSensorSettingsComponent
                            active: _sensor2Enabled
                            property string paramPrefix: "ARSPD2_"
                            property Fact sensorTypeFact: _arspd2Type
                            property string sensorLabel: qsTr("Second Sensor")
                        }
                    }
                }

                // ─── Health Monitoring ───

                QGCGroupBox {
                    title: qsTr("Health Monitoring")
                    visible: _sensorEnabled

                    ColumnLayout {
                        spacing: _margins

                        LabelledFactTextField {
                            label: qsTr("Max airspeed/groundspeed difference")
                            fact: controller.getParameterFact(-1, "ARSPD_WIND_MAX", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "ARSPD_WIND_MAX")
                        }

                        LabelledFactTextField {
                            label: qsTr("Warning threshold")
                            fact: controller.getParameterFact(-1, "ARSPD_WIND_WARN", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "ARSPD_WIND_WARN")
                        }

                        LabelledFactTextField {
                            label: qsTr("Re-enable gate size")
                            fact: controller.getParameterFact(-1, "ARSPD_WIND_GATE", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "ARSPD_WIND_GATE")
                        }

                        LabelledFactTextField {
                            label: qsTr("Offset calibration error warning")
                            fact: controller.getParameterFact(-1, "ARSPD_OFF_PCNT", false)
                            textFieldPreferredWidth: _textFieldWidth
                            textFieldShowUnits: true
                            Layout.fillWidth: true
                            visible: controller.parameterExists(-1, "ARSPD_OFF_PCNT")
                        }
                    }
                }

                // ─── Airspeed Options ───

                QGCGroupBox {
                    title: qsTr("Airspeed Options")
                    visible: _sensorEnabled && controller.parameterExists(-1, "ARSPD_OPTIONS")

                    ColumnLayout {
                        spacing: _margins

                        FactBitmask {
                            fact: controller.getParameterFact(-1, "ARSPD_OPTIONS", false)
                            Layout.preferredWidth: airspeedPage.availableWidth * 0.5
                        }
                    }
                }
            }
        }
    }
}
