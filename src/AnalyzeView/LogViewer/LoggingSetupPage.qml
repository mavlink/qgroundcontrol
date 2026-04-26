import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

AnalyzePage {
    id: loggingSetupPage
    pageComponent: pageComponent
    pageDescription: qsTr("Configure ArduPilot logging parameters, then follow download and troubleshooting guidance.")

    FactPanelController {
        id: controller
    }

    Component {
        id: pageComponent

        QGCFlickable {
            width: availableWidth
            height: availableHeight
            contentHeight: contentColumn.height
            contentWidth: contentColumn.width

            ColumnLayout {
                id: contentColumn
                width: availableWidth
                spacing: ScreenTools.defaultFontPixelHeight

                Repeater {
                    model: [
                        { param: "LOG_BACKEND_TYPE", desc: qsTr("Where logs are stored (SD, MAVLink stream, onboard flash).") },
                        { param: "LOG_BITMASK", desc: qsTr("What data groups are included in the log.") },
                        { param: "LOG_DISARMED", desc: qsTr("Enable disarmed logging for pre-arm and replay diagnostics.") },
                        { param: "LOG_FILE_DSRMROT", desc: qsTr("Rotate log file after disarm/rearm sequence.") },
                        { param: "LOG_FILE_MB_FREE", desc: qsTr("Minimum free space to keep available before logging.") },
                        { param: "LOG_FILE_RATEMAX", desc: qsTr("Limit file logging rate to control size.") },
                        { param: "LOG_BLK_RATEMAX", desc: qsTr("Limit block logging rate for constrained flash media.") },
                        { param: "LOG_MAV_RATEMAX", desc: qsTr("Limit MAVLink log stream rate.") },
                        { param: "LOG_MAX_FILES", desc: qsTr("Maximum retained log files before rotation.") },
                        { param: "LOG_REPLAY", desc: qsTr("Capture extra data needed for EKF replay analysis.") },
                        { param: "EK3_LOG_LEVEL", desc: qsTr("Controls EKF3 logging verbosity and log size.") }
                    ]

                    delegate: Rectangle {
                        id: paramCard
                        Layout.fillWidth: true
                        color: qgcPal.windowShade
                        radius: ScreenTools.defaultFontPixelWidth * 0.5
                        implicitHeight: paramColumn.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.8)

                        property string _paramName: modelData.param
                        property bool _available: controller.parameterExists(-1, _paramName)
                        property var _fact: _available ? controller.getParameterFact(-1, _paramName, false) : null

                        ColumnLayout {
                            id: paramColumn
                            anchors.fill: parent
                            anchors.margins: ScreenTools.defaultFontPixelHeight * 0.4
                            spacing: ScreenTools.defaultFontPixelHeight * 0.3

                            QGCLabel {
                                text: modelData.param
                                font.bold: true
                            }

                            QGCLabel {
                                wrapMode: Text.WordWrap
                                text: modelData.desc
                            }

                            RowLayout {
                                Layout.fillWidth: true

                                FactTextField {
                                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 18
                                    visible: paramCard._available
                                    fact: paramCard._fact
                                }

                                QGCLabel {
                                    visible: !paramCard._available
                                    text: qsTr("Parameter not available on this firmware/vehicle")
                                }
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    color: qgcPal.windowShade
                    radius: ScreenTools.defaultFontPixelWidth * 0.5
                    implicitHeight: guidanceText.implicitHeight + (ScreenTools.defaultFontPixelHeight * 1.2)

                    QGCLabel {
                        id: guidanceText
                        anchors.fill: parent
                        anchors.margins: ScreenTools.defaultFontPixelHeight * 0.6
                        wrapMode: Text.WordWrap
                        text: qsTr("Workflow: connect vehicle, review logging parameters, download logs from Onboard Logs page, open .bin/.tlog in Log Viewer, and use lower rate limits if SD/dataflash cannot keep up.")
                    }
                }
            }
        }
    }
}
