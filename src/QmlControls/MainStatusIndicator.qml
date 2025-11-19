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
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

RowLayout {
    id:         control
    spacing:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property bool   _allowForceArm:      false
    property bool   _healthAndArmingChecksSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false
    property bool   _vehicleFlies:      _activeVehicle ? _activeVehicle.airShip || _activeVehicle.fixedWing || _activeVehicle.vtol || _activeVehicle.multiRotor : false
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.flying || _activeVehicle.landing : false
    property bool   _vtolInFWDFlight:   _activeVehicle ? _activeVehicle.vtolInFwdFlight : false
    property bool   showStatusLabel:    true
    property var    _cameraManager:     _activeVehicle ? _activeVehicle.cameraManager : null
    property var    _camera:            _cameraManager ? _cameraManager.currentCameraInstance : null
    // Match legacy PhotoVideoControl availability: show controls when camera can capture video or has a video stream
    property bool   _cameraRecordAvailable: _camera && (_camera.capturesVideo || _camera.hasVideoStream)

    QGCPalette { id: qgcPal }

    function dropMainStatusIndicator() {
        let overallStatusComponent = _activeVehicle ? overallStatusIndicatorPage : overallStatusOfflineIndicatorPage
        mainWindow.showIndicatorDrawer(overallStatusComponent, control)
    }

    // Neon-style connection controls
    RowLayout {
        id:                 videoRecordMini
        Layout.alignment:   Qt.AlignVCenter
        spacing:            ScreenTools.defaultFontPixelWidth * 0.5
        visible:            _cameraRecordAvailable

        Rectangle {
            id:                 videoRecordCircle
            Layout.alignment:   Qt.AlignVCenter
            width:              ScreenTools.defaultFontPixelHeight * 1.4
            height:             width
            radius:             width / 2
            color:              "transparent"
            border.color:       qgcPal.windowTransparentText
            border.width:       2

            property bool _isRecording: control._camera && control._camera.videoCaptureStatus === MavlinkCameraControl.VIDEO_CAPTURE_STATUS_RUNNING

            Rectangle {
                anchors.centerIn:   parent
                width:              parent.width * 0.7
                height:             width
                radius:             width / 2
                color:              videoRecordCircle._isRecording ? qgcPal.colorRed : qgcPal.colorGrey
            }

            QGCMouseArea {
                anchors.fill:   parent
                onClicked: {
                    if (control._camera) {
                        // Ensure camera is in video mode before attempting to record, similar to PhotoVideoControl
                        if (control._camera.hasModes && control._camera.capturesVideo &&
                                control._camera.cameraMode !== MavlinkCameraControl.CAM_MODE_VIDEO) {
                            control._camera.setCameraModeVideo()
                        }
                        control._camera.toggleVideoRecording()
                    }
                }
            }
        }

        QGCLabel {
            id:                 videoRecordTimeLabel
            Layout.alignment:   Qt.AlignVCenter
            text:               control._camera && videoRecordCircle._isRecording ? control._camera.recordTimeStr : "00:00:00"
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
        }

        QGCColoredImage {
            Layout.alignment:       Qt.AlignVCenter
            source:                 "/res/gear-black.svg"
            mipmap:                 true
            Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 1.4
            Layout.preferredWidth:  Layout.preferredHeight
            sourceSize.height:      Layout.preferredHeight
            color:                  qgcPal.text
            fillMode:               Image.PreserveAspectFit

            QGCMouseArea {
                fillItem:   parent
                onClicked:  videoSettingsDialogComponent.createObject(mainWindow).open()
            }
        }
    }

    QGCButton {
        id:                 connectNeon
        Layout.fillHeight:  false
        height:             ScreenTools.defaultFontPixelHeight * 1.9
        text:               qsTr("CONNECT")
        neon:               true
        pill:               true
        neonColor:          qgcPal.colorGreen
        neonBorderWidth:    1
        visible:            !_activeVehicle
        onClicked:          dropMainStatusIndicator()
    }

    QGCButton {
        id:                 disconnectNeon
        Layout.fillHeight:  false
        height:             ScreenTools.defaultFontPixelHeight * 1.9
        text:               qsTr("DISCONNECT")
        neon:               true
        pill:               true
        neonColor:          qgcPal.colorRed
        neonBorderWidth:    1
        visible:            _activeVehicle
        onClicked:          _activeVehicle.closeVehicle()
    }

        QGCLabel {
            id:                 mainStatusLabel
            Layout.fillHeight:  true
            Layout.preferredWidth: contentWidth + (vehicleMessagesIcon.visible ? vehicleMessagesIcon.width + control.spacing : 0)
            verticalAlignment:  Text.AlignVCenter
            text:               mainStatusText().toUpperCase()
            color:              qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
            font.pointSize:     ScreenTools.largeFontPointSize
            font.bold:          true
            visible:            showStatusLabel && _activeVehicle

        property string _commLostText:      qsTr("Comms Lost")
        property string _readyToFlyText:    control._vehicleFlies ? qsTr("Ready To Fly") : qsTr("Ready")
        property string _notReadyToFlyText: qsTr("Not Ready")
        property string _disconnectedText:  qsTr("Disconnected")
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
                _mainStatusBGColor = qgcPal.brandingPurple
                return mainStatusLabel._disconnectedText
            }
        }

        QGCColoredImage {
            id:                     vehicleMessagesIcon
            anchors.verticalCenter: parent.verticalCenter
            anchors.right:          parent.right
            width:                  ScreenTools.defaultFontPixelWidth * 2
            height:                 width
            source:                 "/res/VehicleMessages.png"
            color:                  getIconColor()
            sourceSize.width:       width
            fillMode:               Image.PreserveAspectFit
            visible:                _activeVehicle && _activeVehicle.messageCount > 0

            function getIconColor() {
                let iconColor = qgcPal.windowTransparentText
                if (_activeVehicle) {
                    if (_activeVehicle.messageTypeWarning) {
                        iconColor = qgcPal.colorOrange
                    } else if (_activeVehicle.messageTypeError) {
                        iconColor = qgcPal.colorRed
                    }
                }
                return iconColor
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
        text:               (_vtolInFWDFlight ? qsTr("FW(VTOL)") : qsTr("MR(VTOL)")).toUpperCase()
        color:              qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
        font.pointSize:     _vehicleInAir ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize
        font.bold:          true
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
        id: videoSettingsDialogComponent

        QGCPopupDialog {
            title:      qsTr("Settings")
            buttons:    Dialog.Close

            property var  _videoSettings: QGroundControl.settingsManager.videoSettings

            ColumnLayout {
                spacing: ScreenTools.defaultFontPixelHeight * 0.75

                GridLayout {
                    columns:            2
                    rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.5
                    columnSpacing:      ScreenTools.defaultFontPixelWidth * 2

                    QGCLabel {
                        text:       qsTr("Video Grid Lines")
                    }

                    QGCSwitch {
                        checked:    _videoSettings.gridLines.rawValue
                        onClicked:  _videoSettings.gridLines.rawValue = checked ? 1 : 0
                    }

                    QGCLabel {
                        text:       qsTr("Compact HUD Size")
                    }

                    QGCSwitch {
                        checked:    _videoSettings.hudCompact.rawValue
                        onClicked:  _videoSettings.hudCompact.rawValue = checked ? 1 : 0
                    }

                    QGCLabel {
                        text:       qsTr("Video Screen Fit")
                    }

                    FactComboBox {
                        Layout.fillWidth:   true
                        sizeToContents:     true
                        fact:               _videoSettings.videoFit
                        indexModel:         false
                    }

                    QGCLabel {
                        text:       qsTr("Reset Camera Defaults")
                    }

                    QGCButton {
                        Layout.fillWidth:   true
                        text:               qsTr("Reset")
                        enabled:            control._cameraRecordAvailable && control._camera
                        onClicked: {
                            if (control._camera) {
                                control._camera.resetSettings()
                            }
                        }
                    }
                }
            }
        }
    }

    Component {
        id: overallStatusOfflineIndicatorPage

        MainStatusIndicatorOfflinePage { }
    }

    Component {
        id: overallStatusIndicatorPage

        ToolIndicatorPage {
            showExpand:         true
            waitForParameters:  true
            contentComponent:   mainStatusContentComponent
            expandedComponent:  mainStatusExpandedComponent
        }
    }

    Component {
        id: mainStatusContentComponent

        ColumnLayout {
            id:         mainLayout
            spacing:    _spacing

            RowLayout {
                spacing: ScreenTools.defaultFontPixelWidth

                QGCDelayButton {
                    enabled:    _armed || !_healthAndArmingChecksSupported || _activeVehicle.healthAndArmingCheckReport.canArm
                    text:       _armed ? qsTr("Disarm") : (control._allowForceArm ? qsTr("Force Arm") : qsTr("Arm"))

                    onActivated: {
                        if (_armed) {
                            _activeVehicle.armed = false
                        } else {
                            if (_allowForceArm) {
                                _allowForceArm = false
                                _activeVehicle.forceArm()
                            } else {
                                _activeVehicle.armed = true
                            }
                        }
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                LabelledComboBox {
                    id:                 primaryLinkCombo
                    Layout.alignment:   Qt.AlignTop
                    label:              qsTr("Primary Link")
                    alternateText:      _primaryLinkName
                    visible:            _activeVehicle && _activeVehicle.vehicleLinkManager.linkNames.length > 1

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
                //Layout.fillWidth:   true
                heading:            qsTr("Vehicle Messages")
                visible:            !vehicleMessageList.noMessages

                VehicleMessageList { 
                    id: vehicleMessageList
                }
            }

            SettingsGroupLayout {
                //Layout.fillWidth:   true
                heading:            qsTr("Sensor Status")
                visible:            !_healthAndArmingChecksSupported

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
                //Layout.fillWidth:   true
                heading:            qsTr("Overall Status")
                visible:            _healthAndArmingChecksSupported && _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0

                // List health and arming checks
                Repeater {
                    model:      _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                    delegate:   listdelegate
                }
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

                        FactPanelController {
                            id: controller
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
                Layout.fillWidth:   true
                source:             _activeVehicle.expandedToolbarIndicatorSource("MainStatus")
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                heading:            qsTr("Force Arm")
                headingDescription: qsTr("Force arming bypasses pre-arm checks. Use with caution.")
                visible:            _activeVehicle && !_armed

                QGCCheckBoxSlider {
                    Layout.fillWidth:   true
                    text:               qsTr("Allow Force Arm")
                    checked:            false
                    onClicked:          _allowForceArm = true
                }
            }

            SettingsGroupLayout {
                Layout.fillWidth:   true
                visible:            QGroundControl.corePlugin.showAdvancedUI

                GridLayout {
                    columns:            2
                    rowSpacing:         ScreenTools.defaultFontPixelHeight / 2
                    columnSpacing:      ScreenTools.defaultFontPixelWidth *2
                    Layout.fillWidth:   true

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Parameters") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {                            
                            mainWindow.showVehicleConfigParametersPage()
                            mainWindow.closeIndicatorDrawer()
                        }
                    }

                    QGCLabel { Layout.fillWidth: true; text: qsTr("Vehicle Configuration") }
                    QGCButton {
                        text: qsTr("Configure")
                        onClicked: {                            
                            mainWindow.showVehicleConfig()
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }
            }
        }
    }
}

