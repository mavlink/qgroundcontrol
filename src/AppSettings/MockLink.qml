import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    color:          qgcPal.window
    anchors.fill:   parent

    readonly property real _margins: ScreenTools.defaultFontPixelHeight

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCFlickable {
        anchors.fill:   parent
        contentWidth:   column.width  + (_margins * 2)
        contentHeight:  column.height + (_margins * 2)
        clip:           true

        ColumnLayout {
            id:                 column
            anchors.margins:    _margins
            anchors.left:       parent.left
            anchors.top:        parent.top
            spacing:            ScreenTools.defaultFontPixelHeight / 4

            QGCCheckBox {
                id:             sendStatusText
                text:           qsTr("Send status text + voice")
            }
            QGCCheckBox {
                id:             enableCamera
                text:           qsTr("Enable camera")
            }
            QGCCheckBox {
                id:             enableGimbal
                text:           qsTr("Enable gimbal")
            }
            QGCCheckBox {
                id:             enableProximity
                text:           qsTr("Enable proximity sensors")
            }
            QGCCheckBox {
                id:             apmStartFreshParams
                text:           qsTr("Start with fresh firmware parameters (setup required)")
                visible:        vehicleTypeCombo.apmSelected

                onVisibleChanged: {
                    if (!visible) {
                        checked = false
                    }
                }
            }
            LabelledComboBox {
                id:                 vehicleTypeCombo
                label:              qsTr("Vehicle Type")
                Layout.fillWidth:   true
                model:              _vehicleNames

                readonly property var _vehicleKeys: QGroundControl.hasAPMSupport ?
                                                        [ "px4", "apmCopter", "apmPlane", "apmSub", "apmRover", "generic" ] :
                                                        [ "px4", "generic" ]
                readonly property var _vehicleNames: QGroundControl.hasAPMSupport ?
                                                        [ qsTr("PX4 Vehicle"), qsTr("APM ArduCopter Vehicle"), qsTr("APM ArduPlane Vehicle"), qsTr("APM ArduSub Vehicle"), qsTr("APM ArduRover Vehicle"), qsTr("Generic Vehicle") ] :
                                                        [ qsTr("PX4 Vehicle"), qsTr("Generic Vehicle") ]
                readonly property string selectedKey: currentIndex >= 0 ? _vehicleKeys[currentIndex] : "px4"
                readonly property bool apmSelected: selectedKey.startsWith("apm")
            }
            QGCButton {
                text:               qsTr("Start MockLink")
                Layout.fillWidth:   true
                onClicked: {
                    switch (vehicleTypeCombo.selectedKey) {
                    case "px4":
                        QGroundControl.startPX4MockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked)
                        break
                    case "apmCopter":
                        QGroundControl.startAPMArduCopterMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked)
                        break
                    case "apmPlane":
                        QGroundControl.startAPMArduPlaneMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked)
                        break
                    case "apmSub":
                        QGroundControl.startAPMArduSubMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked)
                        break
                    case "apmRover":
                        QGroundControl.startAPMArduRoverMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked, apmStartFreshParams.checked)
                        break
                    default:
                        QGroundControl.startGenericMockLink(sendStatusText.checked, enableCamera.checked, enableGimbal.checked, enableProximity.checked)
                        break
                    }
                }
            }
            QGCButton {
                text:               qsTr("Stop One MockLink")
                Layout.fillWidth:   true
                onClicked:          QGroundControl.stopOneMockLink()
            }
        }
    }
}
