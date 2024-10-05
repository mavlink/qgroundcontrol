/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

Item {
    id:             _root
    width:          gimbalIndicatorIcon.width * 1.1 + gimbalTelemetryLayout.childrenRect.width + margins
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property var    gimbalController:       activeVehicle.gimbalController
    property bool   showIndicator:          gimbalController && gimbalController.gimbals.count
    property var    activeGimbal:           gimbalController.activeGimbal
    property var    multiGimbalSetup:       gimbalController.gimbals.count > 1

    property var    margins:                ScreenTools.defaultFontPixelWidth
    property var    panelRadius:            ScreenTools.defaultFontPixelWidth * 0.5
    property var    buttonHeight:           height * 1.6
    property var    separatorHeight:        buttonHeight * 0.9
    property var    settingsPanelVisible:   false

    // Popup panel, appears when clicking top toolbar gimbal indicator
    Component {
        id: gimbalControlsPopup

        Rectangle {
            width:          mainLayout.width   + mainLayout.anchors.margins * 2          
            height:         mainLayout.height  + mainLayout.anchors.margins * 2
            color:          qgcPal.window
            radius:         panelRadius

            GridLayout {
                id:                 mainLayout
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.top:        parent.top
                anchors.right:      parent.right

                // Label indicating the purpose of the panel and active gimbal instance
                QGCLabel {
                    text:                   qsTr("Gimbal ") + 
                                                (multiGimbalSetup ? activeGimbal.deviceId.rawValue : "") + 
                                                    qsTr("<br> Controls")

                    font.pointSize:         ScreenTools.smallFontPointSize
                    Layout.preferredWidth:  buttonHeight * 1.1
                    font.weight:            Font.DemiBold
                }

                // These are simple buttons that can be grouped on this Repeater
                Repeater {
                    id: simpleGimbalButtonsRepeater
                    property var hasControl:              gimbalController && gimbalController.activeGimbal && gimbalController.activeGimbal.gimbalHaveControl
                    property var acqControlButtonEnabled: QGroundControl.settingsManager.gimbalControllerSettings.toolbarIndicatorShowAcquireReleaseControl.rawValue

                    model: [
                        {id: "yawLock",   text: activeGimbal.yawLock ? qsTr("Yaw <br> Follow") : qsTr("Yaw <br> Lock")  , visible: true                    },
                        {id: "center",    text: qsTr("Center")                                                          , visible: true                    },
                        {id: "tilt90",    text: qsTr("Tilt 90")                                                         , visible: true                    },
                        {id: "pointHome", text: qsTr("Point <br> Home")                                                 , visible: true                    },
                        {id: "retract",   text: qsTr("Retract")                                                         , visible: true                    },
                        {id: "acqControl",text: hasControl ? qsTr("Release <br> Control") : qsTr("Acquire <br> Control"), visible: acqControlButtonEnabled }
                    ]

                    QGCButton {
                        property var callbackList: [
                           {"yawLock":      function(){ gimbalController.toggleGimbalYawLock(!activeGimbal.yawLock) }   },
                           {"center":       function(){ gimbalController.centerGimbal() }                               },
                           {"tilt90":       function(){ gimbalController.sendPitchBodyYaw(-90, 0) }                     },
                           {"pointHome":    function(){ activeVehicle.guidedModeROI(activeVehicle.homePosition) }       },
                           {"retract":      function(){ gimbalController.toggleGimbalRetracted(true) }                  },
                           // This button changes its action depending on gimbal being under control or not
                           {"acqControl":   function(){ simpleGimbalButtonsRepeater.hasControl ? 
                                                            gimbalController.releaseGimbalControl() : 
                                                                gimbalController.acquireGimbalControl() }               }
                        ]

                        Layout.preferredWidth: Layout.preferredHeight
                        Layout.preferredHeight: buttonHeight
                        Layout.alignment: Qt.AlignHCenter | Qt.AlignVCenter
                        text: modelData.text
                        fontWeight: Font.DemiBold
                        visible: modelData.visible
                        pointSize: ScreenTools.smallFontPointSize
                        backRadius: panelRadius * 0.5
                        onClicked: {
                            var callback = callbackList.find(function(item) {
                                return item.hasOwnProperty(modelData.id);
                            });
                            if (callback !== undefined) {
                                callback[modelData.id]();
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    Layout.leftMargin:      margins
                    Layout.preferredWidth:  2
                    Layout.preferredHeight: separatorHeight
                    color:                  qgcPal.windowShade
                    visible:                multiGimbalSetup
                }

                // Active gimbal selector section
                QGCLabel {
                    text:                   qsTr("Active <br> Gimbal: ") + activeGimbal.deviceId.rawValue
                    font.pointSize:         ScreenTools.smallFontPointSize
                    Layout.preferredWidth:  buttonHeight * 1.1
                    Layout.leftMargin:      margins
                    font.weight:            Font.DemiBold
                    visible:                multiGimbalSetup
                }
                QGCButton {
                    id:                     gimbalSelectorButton
                    Layout.preferredWidth:  Layout.preferredHeight
                    Layout.preferredHeight: buttonHeight
                    Layout.alignment:       Qt.AlignHCenter | Qt.AlignBottom
                    text:                   qsTr("Select <br> Gimbal")
                    fontWeight:             Font.DemiBold
                    pointSize:              ScreenTools.smallFontPointSize
                    backRadius:             panelRadius * 0.5
                    visible:                multiGimbalSetup
                    checkable:              true

                    // This rectangle is to hide the "roundness" of panels when showing the dropdown, in the join between the 2 panels
                    Rectangle {
                        id:                         hideRoundCornersRectangle
                        anchors.verticalCenter:     gimbalSelectorPanel.top
                        anchors.horizontalCenter:   gimbalSelectorPanel.horizontalCenter
                        width:                      gimbalSelectorPanel.width
                        height:                     panelRadius * 2
                        color:                      qgcPal.window
                        visible:                    gimbalSelectorPanel.visible
                    }
                    
                    Rectangle {
                        id:                         gimbalSelectorPanel
                        width:                      buttonHeight + margins * 2
                        height:                     gimbalSelectorContentGrid.childrenRect.height + margins * 2
                        visible:                    gimbalSelectorButton.checked
                        color:                      qgcPal.window
                        radius:                     panelRadius
                        // We only show border if the extended settings panel is visible
                        border.color:               settingsPanelVisible ? qgcPal.windowShade : qgcPal.window
                        border.width:               5

                        anchors.top:                parent.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.topMargin:          margins

                        property var buttonWidth:    width - margins * 2
                        property var panelHeight:    gimbalSelectorContentGrid.childrenRect.height + margins * 2
                        property var gridRowSpacing: margins
                        property var buttonFontSize: ScreenTools.smallFontPointSize * 0.9

                        GridLayout {
                            id:               gimbalSelectorContentGrid
                            width:            parent.width
                            rowSpacing:       gimbalSelectorPanel.gridRowSpacing
                            columns:          1

                            anchors.horizontalCenter: parent.horizontalCenter
                            anchors.top:              parent.top
                            anchors.topMargin:        margins

                            Repeater {
                                model: gimbalController && gimbalController.gimbals ? gimbalController.gimbals : undefined
                                delegate: QGCButton {
                                    Layout.preferredWidth:  Layout.preferredHeight
                                    Layout.preferredHeight: buttonHeight
                                    Layout.alignment:       Qt.AlignHCenter | Qt.AlignVCenter
                                    fontWeight:             Font.DemiBold
                                    pointSize:              ScreenTools.smallFontPointSize
                                    backRadius:             panelRadius * 0.5
                                    text:                   qsTr("Gimbal ") + object.deviceId.rawValue
                                    checked:                activeGimbal === object
                                    onClicked: {
                                        gimbalController.activeGimbal = object
                                        gimbalSelectorButton.checked = false
                                    }
                                }
                            }
                        }
                    }
                }

                // Separator
                Rectangle {
                    Layout.leftMargin:      margins
                    Layout.preferredWidth:  2
                    Layout.preferredHeight: separatorHeight
                    color:                  qgcPal.windowShade
                }

                // Show settings button. It is thought for persisting popup close actions, hence the visibility
                // based on a _root.settingsPanelVisible It is interesting as users calibrating onscreen controls 
                // will be testing and adjusting these frequently, so this way it is handier for them
                QGCButton {
                    id:                     extendedOptionsButton
                    Layout.leftMargin:      margins
                    Layout.preferredWidth:  Layout.preferredHeight
                    Layout.preferredHeight: buttonHeight
                    Layout.alignment:       Qt.AlignHCenter | Qt.AlignBottom
                    text:                   qsTr("Settings")
                    fontWeight:             Font.DemiBold
                    pointSize:              ScreenTools.smallFontPointSize
                    backRadius:             panelRadius * 0.5
                    checkable:              true
                    checked:                _root.settingsPanelVisible
                    onCheckedChanged: {
                        if (checked !== _root.settingsPanelVisible) {
                            _root.settingsPanelVisible = checked
                        }
                    }
                }

                // Settings panel
                GridLayout {
                    Layout.row:         2
                    Layout.columnSpan:  8
                    Layout.fillWidth:   true
                    height:             buttonHeight * 1.5
                    visible:            settingsPanelVisible
                    columns:            2
                    rowSpacing:         margins

                    // Click on screen settings
                    FactCheckBox {
                        id:                 enableOnScreenControlCheckbox
                        text:               "  " + QGroundControl.settingsManager.gimbalControllerSettings.EnableOnScreenControl.shortDescription
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.EnableOnScreenControl
                        checkedValue:       1
                        uncheckedValue:     0
                        Layout.columnSpan:  2
                    }

                    QGCLabel {
                        id:                 controlTypeLabel
                        text:               qsTr("Control type: ")
                        visible:            enableOnScreenControlCheckbox.checked
                    }
                    FactComboBox {
                        id:                 controlTypeCombo
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.ControlType
                        visible:            enableOnScreenControlCheckbox.checked
                    }

                    QGCLabel {
                        text:               qsTr("Horizontal FOV")
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 0
                    }
                    FactTextField {
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.CameraHFov
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 0
                    }

                    QGCLabel {
                        text:               qsTr("Vertical FOV")
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 0
                    }
                    FactTextField {
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.CameraVFov
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 0
                    }

                    QGCLabel {
                        text:               qsTr("Max speed:")
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 1
                    }
                    FactTextField {
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.CameraSlideSpeed
                        visible:            enableOnScreenControlCheckbox.checked && QGroundControl.settingsManager.gimbalControllerSettings.ControlType.rawValue === 1
                    }

                    // Separator
                    Rectangle {
                        Layout.columnSpan:       2
                        Layout.preferredHeight:  2
                        Layout.preferredWidth:   gimbalAzimuthMapCheckbox.width
                        Layout.margins:          margins * 1.5
                        color:                   qgcPal.windowShade
                    }

                    FactCheckBox {
                        id:                 gimbalAzimuthMapCheckbox
                        text:               "  " + qsTr("Show gimbal Azimuth indicator in map")
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.showAzimuthIndicatorOnMap
                        Layout.columnSpan:  2
                        checkedValue:       1
                        uncheckedValue:     0
                    }

                    FactCheckBox {
                        id:                 gimbalAzimutIndicatorCheckbox
                        text:               "  " + qsTr("Use Azimuth instead of local yaw on top toolbar indicator")
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.toolbarIndicatorShowAzimuth
                        Layout.columnSpan:  2
                        checkedValue:       1
                        uncheckedValue:     0
                    }

                    FactCheckBox {
                        id:                 showAcquireControlCheckbox
                        text:               "  " + qsTr("Show Acquire/Release control button")
                        fact:               QGroundControl.settingsManager.gimbalControllerSettings.toolbarIndicatorShowAcquireReleaseControl
                        Layout.columnSpan:  2
                        checkedValue:       1
                        uncheckedValue:     0
                    }
                }
            }
        }
    }

    // Icon plus active instance indicator
    QGCColoredImage {
        id:                      gimbalIndicatorIcon
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gimbal/payload.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   qgcPal.buttonText

        // Current active gimbal indicator
        QGCLabel {
            id:                  gimbalInstanceIndicatorLabel
            text:                activeGimbal ? activeGimbal.deviceId.rawValue : ""
            visible:             multiGimbalSetup
            anchors.top:         parent.top
            anchors.topMargin:   -margins * 0.5
            anchors.right:       parent.right
            anchors.rightMargin: -margins * 0.5
        }
    }

    // Telemetry and status indicator
    GridLayout {
        id:                        gimbalTelemetryLayout
        anchors.left:              gimbalIndicatorIcon.right
        anchors.leftMargin:        margins
        anchors.verticalCenter:    parent.verticalCenter
        columns:                   2
        rows:                      2
        rowSpacing:                0
        columnSpacing:             margins
        property bool showAzimuth: QGroundControl.settingsManager.gimbalControllerSettings.toolbarIndicatorShowAzimuth.rawValue

        QGCLabel {
            id:                     statusLabel
            text:                   activeGimbal && activeGimbal.retracted ? 
                                        qsTr("Retracted") :
                                        (activeGimbal && activeGimbal.yawLock ? qsTr("Yaw locked") : qsTr("Yaw follow"))
            Layout.columnSpan:      2
            Layout.alignment:       Qt.AlignHCenter
        }
        QGCLabel {
            id:                     pitchLabel
            text:                   activeGimbal ? qsTr("P: ") + activeGimbal.absolutePitch.rawValue.toFixed(1) : ""
        }
        QGCLabel {
            id:                     panLabel
            text:                   activeGimbal ? 
                                        gimbalTelemetryLayout.showAzimuth ? (qsTr("Az: ") + activeGimbal.absoluteYaw.rawValue.toFixed(1)) :
                                            (qsTr("Y: ") + activeGimbal.bodyYaw.rawValue.toFixed(1)) :
                                                ""
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            mainWindow.showIndicatorPopup(_root, gimbalControlsPopup, false)
        }
    }
}
