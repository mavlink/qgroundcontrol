/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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

RowLayout {
    id:         control
    spacing:    0

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.flying || _activeVehicle.landing : false
    property bool   _vtolInFWDFlight:   _activeVehicle ? _activeVehicle.vtolInFwdFlight : false
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property bool   _healthAndArmingChecksSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false

    QGCMarqueeLabel {
        id:             mainStatusLabel
        text:           mainStatusText()
        font.pointSize: ScreenTools.largeFontPointSize
        implicitWidth:  maxWidth
        maxWidth:       ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 10

        property string _commLostText:      qsTr("Communication Lost")
        property string _readyToFlyText:    qsTr("Ready To Fly")
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
                        if (_activeVehicle.allSensorsHealthy && _activeVehicle.autopilot.setupComplete) {
                            _mainStatusBGColor = "green"
                            return mainStatusLabel._readyToFlyText
                        } else {
                            _mainStatusBGColor = "yellow"
                            return mainStatusLabel._notReadyToFlyText
                        }
                    }
                }
            } else {
                _mainStatusBGColor = qgcPal.brandingPurple
                return mainStatusLabel._disconnectedText
            }
        }

        QGCMouseArea {
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            height:                 control.height
            onClicked:              mainWindow.showIndicatorDrawer(overallStatusComponent, control)

            property Component overallStatusComponent: _activeVehicle ? overallStatusIndicatorPage : overallStatusOfflineIndicatorPage
        }
    }

    Item {
        implicitWidth:  ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * 1.5
        implicitHeight: 1
        visible:        vtolModeLabel.visible
    }

    QGCLabel {
        id:                     vtolModeLabel
        Layout.alignment:       Qt.AlignVCenter
        text:                   _vtolInFWDFlight ? qsTr("FW(vtol)") : qsTr("MR(vtol)")
        font.pointSize:         enabled ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize
        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * ScreenTools.largeFontPointRatio * text.length
        visible:                _activeVehicle && _activeVehicle.vtol
        enabled:                _activeVehicle && _activeVehicle.vtol && _vehicleInAir

        QGCMouseArea {
            anchors.fill:   parent
            onClicked:      mainWindow.showIndicatorDrawer(vtolTransitionIndicatorPage)
        }
    }

    Component {
        id: overallStatusOfflineIndicatorPage

        MainStatusIndicatorOfflinePage {

        }
    }

    Component {
        id: overallStatusIndicatorPage

        ToolIndicatorPage {
            showExpand:         _activeVehicle.mainStatusIndicatorContentItem ? true : false
            waitForParameters:  _activeVehicle.mainStatusIndicatorContentItem ? true : false
            contentComponent:   mainStatusContentComponent
            expandedComponent:  mainStatusExpandedComponent
        }
    }

    Component {
        id: mainStatusContentComponent

        Column {
            id:         mainLayout
            spacing:    _spacing

            QGCButton {
                // FIXME: forceArm is not possible anymore if _healthAndArmingChecksSupported == true
                enabled:    _armed || !_healthAndArmingChecksSupported || _activeVehicle.healthAndArmingCheckReport.canArm
                text:       _armed ?  qsTr("Disarm") : (forceArm ? qsTr("Force Arm") : qsTr("Arm"))

                property bool forceArm: false

                onPressAndHold: forceArm = true

                onClicked: {
                    if (_armed) {
                        mainWindow.disarmVehicleRequest()
                    } else {
                        if (forceArm) {
                            mainWindow.forceArmVehicleRequest()
                        } else {
                            mainWindow.armVehicleRequest()
                        }
                    }
                    forceArm = false
                    mainWindow.closeIndicatorDrawer()
                }
            }

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                text:                       qsTr("Sensor Status")
                visible:                    !_healthAndArmingChecksSupported
            }

            GridLayout {
                rowSpacing:     _spacing
                columnSpacing:  _spacing
                rows:           _activeVehicle.sysStatusSensorInfo.sensorNames.length
                flow:           GridLayout.TopToBottom
                visible:        !_healthAndArmingChecksSupported

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorNames
                    QGCLabel { text: modelData }
                }

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorStatus
                    QGCLabel { text: modelData }
                }
            }

            QGCLabel {
                text:       qsTr("Overall Status")
                visible:    _healthAndArmingChecksSupported && _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0
            }
            // List health and arming checks
            Repeater {
                visible:    _healthAndArmingChecksSupported
                model:      _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                delegate:   listdelegate
            }

            FactPanelController {
                id: controller
            }

            Component {
                id: listdelegate

                Column {
                    Row {
                        spacing: ScreenTools.defaultFontPixelHeight

                        QGCLabel {
                            id:           message
                            text:         object.message
                            textFormat:   TextEdit.RichText
                            color:        object.severity == 'error' ? qgcPal.colorRed : object.severity == 'warning' ? qgcPal.colorOrange : qgcPal.text
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
                            anchors.verticalCenter: parent.verticalCenter
                            height:                 1.5 * ScreenTools.defaultFontPixelWidth
                            width:                  height
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
                        text:               object.description
                        textFormat:         TextEdit.RichText
                        clip:               true
                        visible:            object.expanded
                        
                        property var fact:  null

                        onLinkActivated: (link) => {
                            if (link.startsWith('param://')) {
                                var paramName = link.substr(8);
                                fact = controller.getParameterFact(-1, paramName, true)
                                if (fact != null) {
                                    paramEditorDialogComponent.createObject(mainWindow).open()
                                }
                            } else {
                                Qt.openUrlExternally(link);
                            }
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

    Component {
        id: mainStatusExpandedComponent

        ColumnLayout {
            Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 60
            spacing:                margins / 2

            property real margins: ScreenTools.defaultFontPixelHeight

            Loader {
                source: _activeVehicle.mainStatusIndicatorContentItem
            }

            SettingsGroupLayout {
                Layout.fillWidth: true

                GridLayout {
                    columns:            2
                    rowSpacing:         ScreenTools.defaultFontPixelHeight / 2
                    columnSpacing:      ScreenTools.defaultFontPixelWidth *2
                    Layout.fillWidth:   true

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Parameters") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {                            
                            mainWindow.showVehicleSetupTool(qsTr("Parameters"))
                            mainWindow.closeIndicatorDrawer()
                        }
                    }

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Initial Vehicle Setup") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {                            
                            mainWindow.showVehicleSetupTool()
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }
            }
        }
    }

    Component {
        id: vtolTransitionIndicatorPage

        ToolIndicatorPage {
            contentComponent: Component {
                QGCButton {
                    text: _vtolInFWDFlight ? qsTr("Transition to Multi-Rotor") : qsTr("Transition to Fixed Wing")

                    onClicked: {
                        if (_vtolInFWDFlight) {
                            mainWindow.vtolTransitionToMRFlightRequest()
                        } else {
                            mainWindow.vtolTransitionToFwdFlightRequest()
                        }
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }
        }
    }
}

