/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:             control
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    width:          gimbalIndicatorRow.width

    property bool   showIndicator:              gimbalController.gimbals.count

    property var    activeVehicle:              QGroundControl.multiVehicleManager.activeVehicle
    property var    gimbalController:           activeVehicle.gimbalController
    property var    gimbals:                    gimbalController.gimbals
    property var    activeGimbal:               gimbalController.activeGimbal
    property var    multiGimbalSetup:           gimbalController.gimbals.count > 1
    property bool   joystickButtonsAvailable:   activeVehicle.joystickEnabled
    property bool   showAzimuth:                QGroundControl.settingsManager.gimbalControllerSettings.toolbarIndicatorShowAzimuth.rawValue

    property var    margins:                    ScreenTools.defaultFontPixelWidth
    property var    panelRadius:                ScreenTools.defaultFontPixelWidth * 0.5
    property var    buttonHeight:               height * 1.6
    property var    squareButtonPadding:        ScreenTools.defaultFontPixelWidth
    property var    separatorHeight:            buttonHeight * 0.9
    property var    settingsPanelVisible:       false

    property var _gimbalControllerSettings: QGroundControl.settingsManager.gimbalControllerSettings

    QGCPalette { id: qgcPal }

    Row {
        id:             gimbalIndicatorRow
        anchors.top:    parent.top
        anchors.bottom: parent.bottom
        spacing:        ScreenTools.defaultFontPixelWidth / 2

        Column {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            spacing:        0

            QGCColoredImage {
                id:                      gimbalIndicatorIcon
                anchors.horizontalCenter: parent.horizontalCenter
                width:                   height
                height:                  multiGimbalSetup ? parent.height - gimbalIdLabel.contentHeight : parent.height
                source:                  "/gimbal/payload.png"
                fillMode:                Image.PreserveAspectFit
                sourceSize.height:       height
                color:                   qgcPal.windowTransparentText

            }

            QGCLabel {
                id:                     gimbalIdLabel
                anchors.horizontalCenter: parent.horizontalCenter
                font.pointSize:         ScreenTools.smallFontPointSize
                text:                   activeGimbal ? activeGimbal.deviceId.rawValue : ""
                color:                  qgcPal.windowTransparentText
                visible:                multiGimbalSetup
            }
        }

        // Telemetry and status indicator
        GridLayout {
            anchors.verticalCenter:  parent.verticalCenter
            columns:        2
            rows:           2
            rowSpacing:     0
            columnSpacing:  margins / 2

            QGCLabel {
                id:                     statusLabel
                font.pointSize:         ScreenTools.smallFontPointSize
                text:                   activeGimbal && activeGimbal.retracted ?
                                            qsTr("Retracted") :
                                            (activeGimbal && activeGimbal.yawLock ? qsTr("Yaw locked") : qsTr("Yaw follow"))
                color:                  qgcPal.windowTransparentText
                Layout.columnSpan:      2
                Layout.alignment:       Qt.AlignHCenter
            }
            QGCLabel {
                id:             pitchLabel
                font.pointSize: ScreenTools.smallFontPointSize
                text:           activeGimbal ? qsTr("P: ") + activeGimbal.absolutePitch.valueString : ""
                color:          qgcPal.windowTransparentText
            }
            QGCLabel {
                id:             panLabel
                font.pointSize: ScreenTools.smallFontPointSize
                text:           activeGimbal ?
                                    (showAzimuth ?
                                        (qsTr("Az: ") + activeGimbal.absoluteYaw.valueString) :
                                        (qsTr("Y: ") + activeGimbal.bodyYaw.valueString)) :
                                    ""
                color:          qgcPal.windowTransparentText
            }
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(gimbalPopup, control)
    }

    Component {
        id: gimbalPopup

        ToolIndicatorPage {
            showExpand:         true
            waitForParameters:  false
            contentComponent:   gimbalContentComponent
            expandedComponent:  gimbalExpandedComponent
        }
    }

    Component {
        id: gimbalContentComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth / 2

            SettingsGroupLayout {
                heading: qsTr("Active Gimbal")
                visible: multiGimbalSetup

                QGCComboBox {
                    id:                     gimbalSelectorCombo
                    Layout.fillWidth:       true
                    visible:                multiGimbalSetup
                    onCurrentIndexChanged:  gimbalController.activeGimbal = gimbalController.gimbals.get(currentIndex)

                    function _updateComboModel() {
                        let gimbalModel = []
                        let activeIndex = -1
                        for (var i = 0; i < gimbals.count; i++) {
                            var gimbal = gimbals.get(i)
                            gimbalModel.push(qsTr("Gimbal %1").arg(gimbal.deviceId.valueString))
                            if (gimbal === activeGimbal) {
                                activeIndex = i
                            }
                        }
                        gimbalSelectorCombo.model = gimbalModel
                        gimbalSelectorCombo.currentIndex = activeIndex
                    }

                    Component.onCompleted:  _updateComboModel()
                    Connections {
                        target: gimbals
                        function onCountChanged(count) { _updateComboModel() }
                    }
                }
            }

            SettingsGroupLayout {
                heading:        qsTr("Commands")
                showDividers:   false

                QGCButton {
                    Layout.fillWidth:   true
                    text:               activeGimbal.yawLock ? qsTr("Yaw Follow") : qsTr("Yaw Lock")
                    visible:            activeGimbal.supportsYawLock
                    onClicked: {
                        gimbalController.setGimbalYawLock(!activeGimbal.yawLock)
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Center")
                    onClicked: {
                        gimbalController.centerGimbal()
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Tilt 90")
                    onClicked: {
                        gimbalController.sendPitchBodyYaw(-90, 0)
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Point Home")
                    onClicked: {
                        activeVehicle.guidedModeROI(activeVehicle.homePosition)
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               qsTr("Retract")
                    visible:            activeGimbal.supportsRetract
                    onClicked: {
                        gimbalController.setGimbalRetract(true)
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    Layout.fillWidth:   true
                    text:               activeGimbal.gimbalHaveControl ? qsTr("Release Control") : qsTr("Acquire Control")
                    visible:            _gimbalControllerSettings.toolbarIndicatorShowAcquireReleaseControl.rawValue
                    onClicked: {
                        activeGimbal.gimbalHaveControl ? gimbalController.releaseGimbalControl() : gimbalController.acquireGimbalControl()
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }
        }
    }

    Component {
        id: gimbalExpandedComponent

        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelWidth / 2

            SettingsGroupLayout {
                heading:        qsTr("On-Screen Control")
                showDividers:   false

                FactCheckBoxSlider {
                    id:                 enableOnScreenControlCheckbox
                    Layout.fillWidth:   true
                    text:               qsTr("Enabled")
                    fact:               _gimbalControllerSettings.EnableOnScreenControl
                }

                LabelledFactComboBox {
                    label:      qsTr("Control type")
                    fact:       _gimbalControllerSettings.ControlType
                    visible:    enableOnScreenControlCheckbox.checked
                }

                LabelledFactTextField {
                    label:      qsTr("Horizontal FOV")
                    fact:       _gimbalControllerSettings.CameraHFov
                    visible:    enableOnScreenControlCheckbox.checked && _gimbalControllerSettings.ControlType.rawValue === 0
                }

                LabelledFactTextField {
                    label:      qsTr("Vertical FOV")
                    fact:       _gimbalControllerSettings.CameraVFov
                    visible:    enableOnScreenControlCheckbox.checked && _gimbalControllerSettings.ControlType.rawValue === 0
                }

                LabelledFactTextField {
                    label:      qsTr("Max speed")
                    fact:       _gimbalControllerSettings.CameraSlideSpeed
                    visible:    enableOnScreenControlCheckbox.checked && _gimbalControllerSettings.ControlType.rawValue === 1
                }
            }

            SettingsGroupLayout {
                heading:        qsTr("Zoom speed")
                showDividers:   false

                LabelledFactTextField {
                    label:      qsTr("Max speed (min zoom)")
                    fact:       _gimbalControllerSettings.zoomMaxSpeed
                }

                LabelledFactTextField {
                    label:      qsTr("Min speed (max zoom)")
                    fact:       _gimbalControllerSettings.zoomMinSpeed
                }

            }

            SettingsGroupLayout {
                LabelledFactTextField {
                    label:      qsTr("Joystick buttons speed:")
                    fact:       _gimbalControllerSettings.joystickButtonsSpeed
                    enabled:    joystickButtonsAvailable && _gimbalControllerSettings.visible
                }

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Show gimbal Azimuth indicator in map")
                    fact:               _gimbalControllerSettings.showAzimuthIndicatorOnMap
                }

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Use Azimuth instead of local yaw on top toolbar indicator")
                    fact:               _gimbalControllerSettings.toolbarIndicatorShowAzimuth
                }

                FactCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Show Acquire/Release control button")
                    fact:               _gimbalControllerSettings.toolbarIndicatorShowAcquireReleaseControl
                }
            }
        }
    }

    Connections {
        id:                         acquirePopupConnection
        property bool isPopupOpen:  false
        target:                     gimbalController
        function onShowAcquireGimbalControlPopup() {
            if(!acquirePopupConnection.isPopupOpen){
                acquirePopupConnection.isPopupOpen = true;
                mainWindow.showMessageDialog(
                    "Request Gimbal Control?",
                    "Command not sent. Another user has control of the gimbal.",
                    Dialog.Yes | Dialog.No,
                    gimbalController.acquireGimbalControl,
                    function() { acquirePopupConnection.isPopupOpen = false }
                )
            }
        }
    }
}
