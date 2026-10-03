import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id:         control
    spacing:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property bool   _healthAndArmingChecksSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false
    property bool   _vehicleFlies:      _activeVehicle ? !(_activeVehicle.rover || _activeVehicle.sub) : false
    // Rover/Sub report flying whenever armed but accept a normal disarm, so only aircraft lose Disarm in flight
    property bool   _aircraftInFlight:  _armed && _vehicleFlies && _activeVehicle.flying
    property bool   _showEmergencyStop: _aircraftInFlight && QGroundControl.corePlugin.options.flyView.guidedBarShowEmergencyStop
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.flying || _activeVehicle.landing : false
    property bool   _vtolInFWDFlight:   _activeVehicle ? _activeVehicle.vtolInFwdFlight : false

    function dropMainStatusIndicator() {
        let overallStatusComponent = _activeVehicle ? overallStatusIndicatorPage : overallStatusOfflineIndicatorPage
        mainWindow.showIndicatorDrawer(overallStatusComponent, control)
    }

    QGCPalette { id: qgcPal }

    QGCLabel {
        id:                 mainStatusLabel
        Layout.fillHeight:  true
        Layout.preferredWidth: contentWidth + (criticalMessageBadge.visible ? criticalMessageBadge.width / 2 : 0)
        verticalAlignment:  Text.AlignVCenter
        text:               mainStatusText()
        color:              qgcPal.text
        font.pointSize:     ScreenTools.largeFontPointSize

        property string _commLostText:      qsTr("Comms Lost")
        property string _readyToFlyText:    qsTr("Ready")
        property string _notReadyToFlyText: qsTr("Not Ready")
        property string _disconnectedText:  qsTr("Disconnected - Click to manually connect")
        property string _armedText:         qsTr("Armed")
        property string _flyingText:        qsTr("Flying")
        property string _landingText:       qsTr("Landing")

        function mainStatusText() {
            var statusText
            if (_activeVehicle) {
                if (_communicationLost) {
                    _mainStatusBGColor = "red"
                    return mainStatusLabel._commLostText
                }
                if (_activeVehicle.armed) {
                    _mainStatusBGColor = "green"

                    if (_healthAndArmingChecksSupported) {
                        if (_activeVehicle.healthAndArmingCheckReport.canArm) {
                            if (_activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors) {
                                _mainStatusBGColor = "yellow"
                            }
                        } else {
                            _mainStatusBGColor = "red"
                        }
                    }

                    if (_activeVehicle.flying) {
                        return mainStatusLabel._flyingText
                    } else if (_activeVehicle.landing) {
                        return mainStatusLabel._landingText
                    } else {
                        return mainStatusLabel._armedText
                    }
                } else {
                    if (_healthAndArmingChecksSupported) {
                        if (_activeVehicle.healthAndArmingCheckReport.canArm) {
                            if (_activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors) {
                                _mainStatusBGColor = "yellow"
                            } else {
                                _mainStatusBGColor = "green"
                            }
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "red"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    } else if (_activeVehicle.readyToFlyAvailable) {
                        if (_activeVehicle.readyToFly) {
                            _mainStatusBGColor = "green"
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "yellow"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    } else {
                        // Best we can do is determine readiness based on AutoPilot component setup and health indicators from SYS_STATUS
                        if (_activeVehicle.allSensorsHealthy && _activeVehicle.autopilotPlugin.setupComplete) {
                            _mainStatusBGColor = "green"
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "yellow"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    }
                }
            } else {
                _mainStatusBGColor = qgcPal.brandPrimary
                return mainStatusLabel._disconnectedText
            }
        }

        Rectangle {
            id:                 criticalMessageBadge
            anchors.top:        parent.top
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.25
            anchors.right:      parent.right
            width:              ScreenTools.defaultFontPixelHeight
            height:             width
            radius:             width / 2
            color:              qgcPal.colorRed
            visible:            _criticalMessageCount > 0

            property int _criticalMessageCount: _activeVehicle ? _activeVehicle.criticalMessageCount : 0

            QGCLabel {
                anchors.centerIn:   parent
                text:               criticalMessageBadge._criticalMessageCount > 9 ? "!" : criticalMessageBadge._criticalMessageCount.toString()
                color:              qgcPal.buttonHighlightText
                font.pointSize:     ScreenTools.smallFontPointSize
                font.bold:          true
            }
        }

        QGCMouseArea {
            anchors.fill:   parent
            onClicked:      dropMainStatusIndicator()
        }
    }

    QGCLabel {
        id:                 vtolModeLabel
        Layout.fillHeight:  true
        verticalAlignment:  Text.AlignVCenter
        text:               _vtolInFWDFlight ? qsTr("FW(vtol)") : qsTr("MR(vtol)")
        color:              qgcPal.text
        font.pointSize:     _vehicleInAir ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize
        visible:            _activeVehicle && _activeVehicle.vtol

        QGCMouseArea {
            anchors.fill: parent
            onClicked: {
                if (_vehicleInAir) {
                    mainWindow.showIndicatorDrawer(vtolTransitionIndicatorPage)
                }
            }
        }
    }

    Component {
        id: overallStatusOfflineIndicatorPage

        MainStatusIndicatorOfflinePage {
            Component.onCompleted:   mainWindow.suppressCriticalVehicleMessages = true
            Component.onDestruction: mainWindow.suppressCriticalVehicleMessages = false
        }
    }

    Component {
        id: overallStatusIndicatorPage

        ToolIndicatorPage {
            waitForParameters:  false
            fillWindow:         true
            contentComponent:   mainStatusContentComponent

            Component.onCompleted:   mainWindow.suppressCriticalVehicleMessages = true
            Component.onDestruction: mainWindow.suppressCriticalVehicleMessages = false
        }
    }

    Component {
        id: mainStatusContentComponent

        ColumnLayout {
            id:         mainLayout
            spacing:    _spacing

            property bool parametersReady: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth
                // Emergency stop must stay reachable while parameters are still loading
                visible: parametersReady || _showEmergencyStop

                QGCDelayButton {
                    objectName: "mainStatusArmButton"
                    enabled:    _armed || !_healthAndArmingChecksSupported || _activeVehicle.healthAndArmingCheckReport.canArm
                    text:       _armed ? qsTr("Disarm") : qsTr("Arm")
                    visible:    parametersReady && !_aircraftInFlight

                    onActivated: {
                        _activeVehicle.armed = !_armed
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCDelayButton {
                    objectName:         "mainStatusEmergencyStopButton"
                    text:               qsTr("Emergency Stop")
                    backgroundColor:    qgcPal.colorRed
                    textColor:          qgcPal.buttonHighlightText
                    fontWeight:         Font.Bold
                    alwaysShowHelp:     true
                    visible:            _showEmergencyStop

                    onActivated: {
                        _activeVehicle.emergencyStop()
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCDelayButton {
                    text:       qsTr("Force Arm")
                    visible:    parametersReady && !_armed && QGroundControl.settingsManager.flyViewSettings.allowForceArm.rawValue

                    onActivated: {
                        _activeVehicle.forceArm()
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                LabelledComboBox {
                    id:                 primaryLinkCombo
                    Layout.alignment:   Qt.AlignTop
                    label:              qsTr("Primary Link")
                    alternateText:      _primaryLinkName
                    visible:            parametersReady && _activeVehicle && _activeVehicle.vehicleLinkManager.linkNames.length > 1

                    property var    _rgLinkNames:       _activeVehicle ? _activeVehicle.vehicleLinkManager.linkNames : [ ]
                    property var    _rgLinkStatus:      _activeVehicle ? _activeVehicle.vehicleLinkManager.linkStatuses : [ ]
                    property string _primaryLinkName:   _activeVehicle ? _activeVehicle.vehicleLinkManager.primaryLinkName : ""

                    function updateComboModel() {
                        let linkModel = []
                        for (let i = 0; i < _rgLinkNames.length; i++) {
                            let linkStatus = _rgLinkStatus[i]
                            linkModel.push(_rgLinkNames[i] + (linkStatus === "" ? "" : " " + _rgLinkStatus[i]))
                        }
                        primaryLinkCombo.model = linkModel
                        primaryLinkCombo.currentIndex = -1
                    }

                    Component.onCompleted:  updateComboModel()
                    on_RgLinkNamesChanged:  updateComboModel()
                    on_RgLinkStatusChanged: updateComboModel()

                    onActivated:    (index) => {
                        _activeVehicle.vehicleLinkManager.primaryLinkName = _rgLinkNames[index]; currentIndex = -1
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Vehicle Messages")

                VehicleMessageList {
                    id:                 vehicleMessageList
                    Layout.fillWidth:   true
                    visible:            !noMessages
                }

                QGCLabel {
                    text: qsTr("No new vehicle messages")
                    visible: vehicleMessageList.noMessages
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Sensor Status")
                visible:            parametersReady && !_healthAndArmingChecksSupported

                GridLayout {
                    rowSpacing:     _spacing
                    columnSpacing:  _spacing
                    rows:           _activeVehicle.sysStatusSensorInfo.sensorNames.length
                    flow:           GridLayout.TopToBottom

                    Repeater {
                        model: _activeVehicle.sysStatusSensorInfo.sensorNames
                        QGCLabel { text: modelData }
                    }

                    Repeater {
                        model: _activeVehicle.sysStatusSensorInfo.sensorStatus
                        QGCLabel { text: modelData }
                    }
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Overall Status")
                visible:            parametersReady && _healthAndArmingChecksSupported && _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0

                // List health and arming checks
                Repeater {
                    model:      _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                    delegate:   listdelegate
                }
            }

            Component {
                id: listdelegate

                ColumnLayout {
                    Layout.fillWidth:   true
                    spacing:            0

                    RowLayout {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelHeight

                        QGCLabel {
                            id:                     message
                            Layout.fillWidth:       true
                            // Rounded up: the layout's whole-pixel width would otherwise wrap a short message
                            Layout.maximumWidth:    Math.ceil(implicitWidth)
                            text:                   object.message
                            textFormat:             TextEdit.RichText
                            wrapMode:               Text.WordWrap
                            color:                  object.severity == 'error' ? qgcPal.colorRed : object.severity == 'warning' ? qgcPal.colorOrange : qgcPal.text
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (object.description != "")
                                        object.expanded = !object.expanded
                                }
                            }
                        }

                        QGCColoredImage {
                            id:                     arrowDownIndicator
                            Layout.alignment:       Qt.AlignVCenter
                            Layout.preferredHeight: 1.5 * ScreenTools.defaultFontPixelWidth
                            Layout.preferredWidth:  Layout.preferredHeight
                            source:                 "/qmlimages/arrow-down.png"
                            color:                  qgcPal.text
                            visible:                object.description != ""
                            MouseArea {
                                anchors.fill:       parent
                                onClicked:          object.expanded = !object.expanded
                            }
                        }
                    }

                    QGCLabel {
                        id:                 description
                        Layout.fillWidth:   true
                        text:               object.description
                        textFormat:         TextEdit.RichText
                        wrapMode:           Text.WordWrap
                        clip:               true
                        visible:            object.expanded

                        property var fact:  null

                        onLinkActivated: (link) => {
                            if (link.startsWith('param://')) {
                                var paramName = link.substr(8);
                                fact = controller.getParameterFact(-1, paramName, true)
                                if (fact != null) {
                                    paramEditorDialogFactory.open()
                                }
                            } else {
                                Qt.openUrlExternally(link);
                            }
                        }

                        FactPanelController {
                            id: controller
                        }

                        QGCPopupDialogFactory {
                            id: paramEditorDialogFactory

                            dialogComponent: paramEditorDialogComponent
                        }

                        Component {
                            id: paramEditorDialogComponent

                            ParameterEditorDialog {
                                title:          qsTr("Edit Parameter")
                                fact:           description.fact
                                destroyOnClose: true
                            }
                        }
                    }
                }
            }
        }
    }
}
