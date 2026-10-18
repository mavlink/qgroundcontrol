import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

RowLayout {
    id:         control
    spacing:    ScreenTools.defaultFontPixelWidth

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    _guidedController:  globals.guidedControllerFlyView
    property bool   _armed:             _activeVehicle ? _activeVehicle.armed : false
    property real   _margins:           ScreenTools.defaultFontPixelWidth
    property real   _spacing:           ScreenTools.defaultFontPixelWidth / 2
    property bool   _healthAndArmingChecksSupported: _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.supported : false
    property var    _vehicleInAir:      _activeVehicle ? _activeVehicle.airborne || _activeVehicle.landing : false
    property bool   _vtolInFWDFlight:   _activeVehicle ? _activeVehicle.vtolInFwdFlight : false
    property bool   _rebootRequired:    _activeVehicle ? _activeVehicle.rebootRequired : false
    property bool   _checklistEnabled:  QGroundControl.settingsManager.appSettings.useChecklist.rawValue
    property bool   _checklistComplete: _activeVehicle ? _activeVehicle.checkListState === Vehicle.CheckListPassed : false
    property bool   _checklistBlocksArming: _checklistEnabled && QGroundControl.settingsManager.appSettings.enforceChecklist.rawValue && !_checklistComplete
    // The checklist is pre-flight, so an unfinished one is not flagged once armed
    property bool   _checklistWarning:  _checklistEnabled && !_checklistComplete && !_armed
    property int    _healthErrorCount:      _healthAndArmingChecksSupported ?
                                                _activeVehicle.healthAndArmingCheckReport.errorCount : 0
    property int    _healthWarningCount:    _healthAndArmingChecksSupported ?
                                                _activeVehicle.healthAndArmingCheckReport.warningCount : 0

    readonly property int tabChecklist: 0
    readonly property int tabMessages:  1
    readonly property int tabStatus:    2

    property int _initialTab: -1

    // Raised when the dropdown is already open and should switch tabs
    signal tabRequested(int tab)

    component TabStatusLabel: StatusPill {
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
    }

    component TabSeverityBadge: SeverityPill {
        anchors.right:          parent.right
        anchors.rightMargin:    ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
    }

    // Measures a StatusPill's text, for reserving its width
    component LabelMetrics: TextMetrics {
        font.family:    ScreenTools.normalFontFamily
        font.pointSize: ScreenTools.smallFontPointSize
        font.bold:      true
    }

    LabelMetrics { id: errorLabelMetrics;   text: qsTr("Error") }
    LabelMetrics { id: warningLabelMetrics; text: qsTr("Warning") }
    LabelMetrics { id: infoLabelMetrics;    text: qsTr("Info") }
    LabelMetrics { id: preFlightLabelMetrics; text: qsTr("Pre-Flight") }

    readonly property real _severityLabelMaxWidth: Math.max(errorLabelMetrics.advanceWidth,
                                                            warningLabelMetrics.advanceWidth,
                                                            infoLabelMetrics.advanceWidth)

    function dropMainStatusIndicator(showMessages = false) {
        _initialTab = showMessages ? tabMessages : -1
        let overallStatusComponent = _activeVehicle ? overallStatusIndicatorPage : overallStatusOfflineIndicatorPage
        mainWindow.showIndicatorDrawer(overallStatusComponent, control)
        if (_initialTab !== -1) {
            tabRequested(_initialTab)
        }
    }

    QGCPalette { id: qgcPal }

    // Lives here rather than in the dropdown so checklist progress survives the dropdown closing
    Loader {
        id:         checklistLoader
        visible:    false
        active:     _checklistEnabled && !!_activeVehicle
        source:     "qrc:/qml/QGroundControl/FlyView/PreFlightCheckList.qml"

        // The vehicle may still hold the Passed state of the previous checklist, so sync it to the new one
        onLoaded: {
            if (_activeVehicle) {
                _activeVehicle.checkListState = item.allChecksPassed === true ? Vehicle.CheckListPassed : Vehicle.CheckListNotSetup
            }
        }
    }

    QGCLabel {
        id:                 mainStatusLabel
        objectName:         "mainStatusText"
        Layout.fillHeight:  true
        // Reserves room for every badge, visible or not, so neighboring toolbar items don't shift as badges come and go
        // and the severity label changes width
        Layout.preferredWidth: Math.max(contentWidth,
                                        Math.max(_severityLabelMaxWidth, preFlightLabelMetrics.advanceWidth) +
                                        ScreenTools.defaultFontPixelWidth +
                                        rebootRequiredIcon.width + firmwareUpdateBadge.width +
                                        ScreenTools.defaultFontPixelWidth * 0.5)
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

                    if (_activeVehicle.underway) {
                        return mainStatusLabel._flyingText
                    } else if (_activeVehicle.landing) {
                        return mainStatusLabel._landingText
                    } else {
                        return mainStatusLabel._armedText
                    }
                } else {
                    if (_checklistBlocksArming) {
                        _mainStatusBGColor = (_healthAndArmingChecksSupported && !_activeVehicle.healthAndArmingCheckReport.canArm) ? "red" : "yellow"
                        return mainStatusLabel._notReadyToFlyText
                    }
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

        // Worst state across the dropdown tabs: messages, health and arming checks, and the pre-flight checklist
        SeverityPill {
            id:                 severityBadge
            objectName:         "mainStatusSeverityBadge"
            anchors.top:        parent.top
            anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.25
            anchors.right:      parent.right
            errorCount:         _activeVehicle ? _activeVehicle.unreadCriticalMessageCount + _healthErrorCount : 0
            warningCount:       _activeVehicle ? _activeVehicle.unreadWarningMessageCount + _healthWarningCount +
                                                 (_checklistWarning ? 1 : 0) : 0
            unreadCount:        _activeVehicle ? _activeVehicle.unreadMessageCount : 0
            warningLabel:       (_checklistWarning && warningCount === 1) ? qsTr("Pre-Flight") : qsTr("Warning")
        }

        QGCColoredImage {
            id:                 rebootRequiredIcon
            objectName:         "mainStatusRebootRequiredIcon"
            anchors.verticalCenter: severityBadge.verticalCenter
            anchors.right:      severityBadge.visible ? severityBadge.left : parent.right
            anchors.rightMargin: severityBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.25 : 0
            width:              ScreenTools.defaultFontPixelHeight * 0.9
            height:             width
            source:             "/res/PowerButton.svg"
            color:              qgcPal.colorOrange
            visible:            _rebootRequired
        }

        Rectangle {
            id:                 firmwareUpdateBadge
            objectName:         "mainStatusFirmwareUpdateBadge"
            anchors.verticalCenter: severityBadge.verticalCenter
            anchors.right:      rebootRequiredIcon.visible ? rebootRequiredIcon.left : (severityBadge.visible ? severityBadge.left : parent.right)
            anchors.rightMargin: (rebootRequiredIcon.visible || severityBadge.visible) ? ScreenTools.defaultFontPixelWidth * 0.25 : 0
            width:              ScreenTools.defaultFontPixelHeight
            height:             width
            radius:             width / 2
            color:              Qt.rgba(1, 1, 1, 0.75)
            visible:            _activeVehicle ? _activeVehicle.newStableFirmwareVersion !== "" && !_activeVehicle.newStableFirmwareVersionAcknowledged : false

            QGCColoredImage {
                anchors.fill:   parent
                source:         "/res/UpdateAvailable.svg"
                color:          qgcPal.colorBlue
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
        }
    }

    Component {
        id: mainStatusContentComponent

        ColumnLayout {
            id:         mainLayout
            spacing:    _spacing

            property bool parametersReady: QGroundControl.multiVehicleManager.parameterReadyVehicleAvailable

            // Only the Messages tab shows critical messages, so only then is the popup redundant
            Binding {
                target:     mainWindow
                property:   "suppressCriticalVehicleMessages"
                value:      tabBar.currentIndex === control.tabMessages
            }

            Connections {
                target: control
                function onTabRequested(tab) { tabBar.currentIndex = tab }
            }

            RowLayout {
                Layout.alignment:   Qt.AlignHCenter
                spacing:            ScreenTools.defaultFontPixelWidth
                // Stop controls must stay reachable without parameters
                visible:            parametersReady || _guidedController.showEmergencyStop || _guidedController.showDisarm || _rebootRequired

                QGCDelayButton {
                    objectName: "mainStatusArmButton"
                    enabled:    _armed || _guidedController.showArm
                    text:       _armed ? qsTr("Disarm") : qsTr("Arm")
                    visible:    _armed ? _guidedController.showDisarm : parametersReady

                    onActivated: {
                        _activeVehicle.armed = !_armed
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCButton {
                    objectName:         "mainStatusEmergencyStopButton"
                    text:               qsTr("Emergency Stop")
                    backgroundColor:    qgcPal.colorRed
                    textColor:          qgcPal.buttonHighlightText
                    fontWeight:         Font.Bold
                    visible:            _guidedController.showEmergencyStop

                    onClicked: {
                        _guidedController.confirmAction(_guidedController.actionEmergencyStop)
                        mainWindow.closeIndicatorDrawer()
                    }
                }

                QGCDelayButton {
                    text:       qsTr("Force Arm")
                    visible:    parametersReady && _guidedController.showForceArm

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

                QGCLabel {
                    Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 16
                    Layout.maximumHeight:   rebootButton.height
                    text:                   qsTr("Reboot required for changes to take effect")
                    font.pointSize:         ScreenTools.smallFontPointSize
                    wrapMode:               Text.WordWrap
                    elide:                  Text.ElideRight
                    horizontalAlignment:    Text.AlignRight
                    visible:                _rebootRequired && !_armed
                }

                QGCLabel {
                    objectName:             "mainStatusRebootDisarmLabel"
                    Layout.maximumWidth:    ScreenTools.defaultFontPixelWidth * 16
                    Layout.maximumHeight:   rebootButton.height
                    text:                   qsTr("Reboot required. Disarm vehicle first.")
                    font.pointSize:         ScreenTools.smallFontPointSize
                    wrapMode:               Text.WordWrap
                    elide:                  Text.ElideRight
                    horizontalAlignment:    Text.AlignRight
                    visible:                _rebootRequired && _armed
                }

                QGCDelayButton {
                    id:         rebootButton
                    objectName: "mainStatusRebootButton"
                    text:       qsTr("Reboot Vehicle")
                    visible:    _rebootRequired && !_armed

                    onActivated: {
                        _activeVehicle.rebootVehicle()
                        mainWindow.closeIndicatorDrawer()
                    }
                }
            }

            RowLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelWidth
                visible:            _activeVehicle && _activeVehicle.newStableFirmwareVersion !== ""

                QGCLabel {
                    objectName:         "mainStatusFirmwareUpdateLabel"
                    text:               _activeVehicle ? qsTr("Firmware update available: %1.%2.%3 → %4")
                                                             .arg(_activeVehicle.firmwareMajorVersion)
                                                             .arg(_activeVehicle.firmwareMinorVersion)
                                                             .arg(_activeVehicle.firmwarePatchVersion)
                                                             .arg(_activeVehicle.newStableFirmwareVersion) : ""
                }

                QGCButton {
                    objectName:     "mainStatusFirmwareUpdateAckButton"
                    small:          true
                    text:           qsTr("Acknowledge and Hide")
                    visible:        _activeVehicle && !_activeVehicle.newStableFirmwareVersionAcknowledged
                    onClicked:      _activeVehicle.acknowledgeNewStableFirmwareVersion()
                }
            }

            QGCTabBar {
                id:                 tabBar
                Layout.fillWidth:   true

                // Keeps the centered tab labels clear of the widest (translated) status label
                readonly property real badgePadding: Math.max(_severityLabelMaxWidth, passedLabelMetrics.advanceWidth,
                                                              inProgressLabelMetrics.advanceWidth) + ScreenTools.defaultFontPixelWidth * 3

                LabelMetrics { id: passedLabelMetrics;     text: qsTr("Passed") }
                LabelMetrics { id: inProgressLabelMetrics; text: qsTr("In Progress") }

                QGCTabButton {
                    objectName:     "mainStatusChecklistTab"
                    text:           qsTr("Pre-Flight Checklist")
                    visible:        _checklistEnabled
                    leftPadding:    tabBar.badgePadding
                    rightPadding:   leftPadding

                    TabStatusLabel {
                        objectName: "mainStatusChecklistTabBadge"
                        text:       _checklistComplete ? qsTr("Passed") : qsTr("In Progress")
                        color:      _checklistComplete ? QGroundControl.globalPalette.successIndicator : QGroundControl.globalPalette.warningIndicator
                    }
                }

                QGCTabButton {
                    objectName:     "mainStatusMessagesTab"
                    text:           qsTr("Vehicle Messages")
                    leftPadding:    tabBar.badgePadding
                    rightPadding:   leftPadding

                    TabSeverityBadge {
                        objectName:     "mainStatusMessagesTabBadge"
                        errorCount:     _activeVehicle ? _activeVehicle.unreadCriticalMessageCount : 0
                        warningCount:   _activeVehicle ? _activeVehicle.unreadWarningMessageCount : 0
                        unreadCount:    _activeVehicle ? _activeVehicle.unreadMessageCount : 0
                    }
                }

                QGCTabButton {
                    objectName:     "mainStatusOverallStatusTab"
                    text:           qsTr("Overall Status")
                    leftPadding:    tabBar.badgePadding
                    rightPadding:   leftPadding

                    TabSeverityBadge {
                        objectName:     "mainStatusOverallStatusTabBadge"
                        errorCount:     _healthErrorCount
                        warningCount:   _healthWarningCount
                    }
                }

                Component.onCompleted: {
                    if (control._initialTab !== -1) {
                        currentIndex = control._initialTab
                    } else {
                        currentIndex = (_checklistEnabled && !_checklistComplete) ? control.tabChecklist : control.tabStatus
                    }
                }
            }

            Item {
                id:                 checklistHost
                Layout.fillWidth:   true
                implicitHeight:     checklistLoader.item ? checklistLoader.item.implicitHeight : 0
                visible:            _checklistEnabled && tabBar.currentIndex === control.tabChecklist

                function adoptChecklist() {
                    if (checklistLoader.item) {
                        checklistLoader.item.parent = checklistHost
                    }
                }

                Connections {
                    target: checklistLoader
                    function onItemChanged() { checklistHost.adoptChecklist() }
                }

                Binding {
                    target:         checklistLoader.item
                    property:       "width"
                    value:          checklistHost.width
                    restoreMode:    Binding.RestoreNone
                }

                Component.onCompleted: adoptChecklist()

                // The checklist keeps its progress, so hand it back before this page is destroyed
                Component.onDestruction: {
                    if (checklistLoader.item) {
                        checklistLoader.item.parent = checklistLoader
                    }
                }
            }

            ColumnLayout {
                Layout.fillWidth:   true
                spacing:            ScreenTools.defaultFontPixelHeight / 4
                visible:            tabBar.currentIndex === control.tabMessages

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth / 2
                    visible:            !vehicleMessageList.noMessages

                    Item { Layout.fillWidth: true }

                    QGCButton {
                        objectName:     "vehicleMessagesMarkReadButton"
                        small:          true
                        text:           qsTr("Mark as Read")
                        visible:        _activeVehicle && _activeVehicle.hasUnreadMessages
                        onClicked:      _activeVehicle.markMessagesRead()
                    }

                    QGCButton {
                        objectName:     "vehicleMessagesClearButton"
                        small:          true
                        text:           qsTr("Clear")
                        onClicked: {
                            _activeVehicle.clearMessages()
                            mainWindow.closeIndicatorDrawer()
                        }
                    }
                }

                VehicleMessageList {
                    id:                 vehicleMessageList
                    Layout.fillWidth:   true
                    visible:            !noMessages
                }

                QGCLabel {
                    text:       qsTr("No new vehicle messages")
                    visible:    vehicleMessageList.noMessages
                }
            }

            GridLayout {
                id:             sensorStatusGrid
                rowSpacing:     _spacing
                columnSpacing:  _spacing
                rows:           _activeVehicle.sysStatusSensorInfo.sensorNames.length
                flow:           GridLayout.TopToBottom
                visible:        tabBar.currentIndex === control.tabStatus && parametersReady && !_healthAndArmingChecksSupported

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorNames
                    QGCLabel { text: modelData }
                }

                Repeater {
                    model: _activeVehicle.sysStatusSensorInfo.sensorStatus
                    QGCLabel { text: modelData }
                }
            }

            ColumnLayout {
                id:                 overallStatusList
                Layout.fillWidth:   true
                spacing:            _spacing
                visible:            tabBar.currentIndex === control.tabStatus && parametersReady && _healthAndArmingChecksSupported &&
                                    _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode.count > 0

                QGCButton {
                    objectName:         "healthChecksAcknowledgeAllButton"
                    Layout.alignment:   Qt.AlignRight
                    small:              true
                    text:               qsTr("Acknowledge All Warnings")
                    visible:            _healthWarningCount > 0
                    onClicked:          _activeVehicle.healthAndArmingCheckReport.acknowledgeAllWarnings()
                }

                // List health and arming checks
                Repeater {
                    model:      _activeVehicle ? _activeVehicle.healthAndArmingCheckReport.problemsForCurrentMode : null
                    delegate:   listdelegate
                }
            }

            QGCLabel {
                text:       parametersReady ? qsTr("No status issues") : qsTr("Waiting for parameters...")
                visible:    tabBar.currentIndex === control.tabStatus && !sensorStatusGrid.visible && !overallStatusList.visible
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
                            text:                   object.message
                            textFormat:             TextEdit.RichText
                            wrapMode:               Text.WordWrap
                            color:                  object.severity == 'error' ? qgcPal.errorText : (object.severity == 'warning' && !object.acknowledged) ? qgcPal.warningText : qgcPal.text
                            opacity:                object.acknowledged ? 0.6 : 1
                            MouseArea {
                                anchors.fill: parent
                                onClicked: {
                                    if (object.description != "")
                                        object.expanded = !object.expanded
                                }
                            }
                        }

                        QGCLabel {
                            Layout.alignment:       Qt.AlignVCenter
                            text:                   qsTr("Acknowledged")
                            font.pointSize:         ScreenTools.smallFontPointSize
                            opacity:                0.6
                            visible:                object.acknowledged
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
