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
import QtQuick.Dialogs
import QtQuick.Layouts
import QtQuick.Window

import QGroundControl

import QGroundControl.Controls
import QGroundControl.FactControls

import QGroundControl.FlightDisplay
import QGroundControl.FlightMap

/// @brief Native QML top level window
/// All properties defined here are visible to all QML pages.
ApplicationWindow {
    id:             mainWindow
    visible:        true
    title:          QGroundControl.appName
    font.family:    ScreenTools.normalFontFamily

    property bool   _utmspSendActTrigger

    Component.onCompleted: {
        // Start the sequence of first run prompt(s)
        firstRunPromptManager.nextPrompt()
    }

    /// Saves main window position and size and re-opens it in the same position and size next time
    MainWindowSavedState {
        window: mainWindow
    }

    QtObject {
        id: firstRunPromptManager

        property var currentDialog:     null
        property var rgPromptIds:       QGroundControl.corePlugin.firstRunPromptsToShow()
        property int nextPromptIdIndex: 0

        function clearNextPromptSignal() {
            if (currentDialog) {
                currentDialog.closed.disconnect(nextPrompt)
            }
        }

        function nextPrompt() {
            if (nextPromptIdIndex < rgPromptIds.length) {
                var component = Qt.createComponent(QGroundControl.corePlugin.firstRunPromptResource(rgPromptIds[nextPromptIdIndex]));
                currentDialog = component.createObject(mainWindow)
                currentDialog.closed.connect(nextPrompt)
                currentDialog.open()
                nextPromptIdIndex++
            } else {
                currentDialog = null
                showPreFlightChecklistIfNeeded()
            }
        }
    }

    readonly property real      _topBottomMargins:          ScreenTools.defaultFontPixelHeight * 0.5

    // Tracks which fly-view sidebar tool is currently active ("video", "plan", "analyze", "setup", "settings", or "")
    property string activeFlySidebarTool: ""

    //-------------------------------------------------------------------------
    //-- Global Scope Variables

    QtObject {
        id: globals

        readonly property var       activeVehicle:                  QGroundControl.multiVehicleManager.activeVehicle
        readonly property real      defaultTextHeight:              ScreenTools.defaultFontPixelHeight
        readonly property real      defaultTextWidth:               ScreenTools.defaultFontPixelWidth
        readonly property var       planMasterControllerFlyView:    flyView.planController
        readonly property var       guidedControllerFlyView:        flyView.guidedController

        // Number of QGCTextField's with validation errors. Used to prevent closing panels with validation errors.
        property int                validationErrorCount:           0 

        // Property to manage RemoteID quick access to settings page
        property bool               commingFromRIDIndicator:        false
    }

    /// Default color palette used throughout the UI
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    //-------------------------------------------------------------------------
    //-- Actions

    signal armVehicleRequest
    signal forceArmVehicleRequest
    signal disarmVehicleRequest
    signal vtolTransitionToFwdFlightRequest
    signal vtolTransitionToMRFlightRequest
    signal showPreFlightChecklistIfNeeded

    //-------------------------------------------------------------------------
    //-- Global Scope Functions

    // This function is used to prevent view switching if there are validation errors
    function allowViewSwitch(previousValidationErrorCount = 0) {
        // Run validation on active focus control to ensure it is valid before switching views
        if (mainWindow.activeFocusControl instanceof FactTextField) {
            mainWindow.activeFocusControl._onEditingFinished()
        }
        return globals.validationErrorCount <= previousValidationErrorCount
    }

    function showPlanView() {
        flyView.visible = false
        planView.visible = true
        activeFlySidebarTool = "plan"
    }

    function showFlyView() {
        flyView.visible = true
        planView.visible = false
        activeFlySidebarTool = ""
    }

    function showTool(toolTitle, toolSource, toolIcon) {
        toolDrawer.backIcon     = flyView.visible ? "/qmlimages/PaperPlane.svg" : "/qmlimages/Plan.svg"
        toolDrawer.toolTitle    = toolTitle
        toolDrawer.toolSource   = toolSource
        toolDrawer.toolIcon     = toolIcon
        toolDrawer.visible      = true
    }

    function showAnalyzeTool() {
        showTool(qsTr("Analyze Tools"), "qrc:/qml/QGroundControl/AnalyzeView/AnalyzeView.qml", "/qmlimages/Analyze.svg")
        activeFlySidebarTool = "analyze"
    }

    function showVehicleConfig() {
        showTool(qsTr("Vehicle Configuration"), "qrc:/qml/QGroundControl/VehicleSetup/SetupView.qml", "/qmlimages/Gears.svg")
        activeFlySidebarTool = "setup"
    }

    function showVehicleConfigParametersPage() {
        showVehicleConfig()
        toolDrawerLoader.item.showParametersPanel()
    }

    function showKnownVehicleComponentConfigPage(knownVehicleComponent) {
        showVehicleConfig()
        let vehicleComponent = globals.activeVehicle.autopilotPlugin.findKnownVehicleComponent(knownVehicleComponent)
        if (vehicleComponent) {
            toolDrawerLoader.item.showVehicleComponentPanel(vehicleComponent)
        }
    }

    function showAboutTool() {
        showTool(qsTr("About"), "qrc:/qml/QGroundControl/AppSettings/HelpSettings.qml", "/InstrumentValueIcons/question.svg")
        activeFlySidebarTool = "about"
    }

    property string _pendingSettingsPage: ""

    function showSettingsTool(settingsPage = "") {
        showTool(qsTr("Application Settings"), "qrc:/qml/QGroundControl/Controls/AppSettings.qml", "/res/QGCLogoWhite")
        activeFlySidebarTool = settingsPage === "Video" ? "video" : "settings"
        if (settingsPage !== "") {
            _pendingSettingsPage = settingsPage
            if (toolDrawerLoader.item) {
                toolDrawerLoader.item.showSettingsPage(settingsPage)
                _pendingSettingsPage = ""
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Global simple message dialog

    function showMessageDialog(dialogTitle, dialogText, buttons = Dialog.Ok, acceptFunction = null, closeFunction = null) {
        simpleMessageDialogComponent.createObject(mainWindow, { title: dialogTitle, text: dialogText, buttons: buttons, acceptFunction: acceptFunction, closeFunction: closeFunction }).open()
    }

    // This variant is only meant to be called by QGCApplication
    function _showMessageDialog(dialogTitle, dialogText) {
        showMessageDialog(dialogTitle, dialogText)
    }

    Component {
        id: simpleMessageDialogComponent

        QGCSimpleMessageDialog {
        }
    }

    property bool _forceClose: false

    function finishCloseProcess() {
        _forceClose = true
        // For some reason on the Qml side Qt doesn't automatically disconnect a signal when an object is destroyed.
        // So we have to do it ourselves otherwise the signal flows through on app shutdown to an object which no longer exists.
        firstRunPromptManager.clearNextPromptSignal()
        QGroundControl.linkManager.shutdown()
        QGroundControl.videoManager.stopVideo();
        mainWindow.close()
    }

    // Check for things which should prevent the app from closing
    //  Returns true if it is OK to close
    readonly property int _skipUnsavedMissionCheckMask: 0x01
    readonly property int _skipPendingParameterWritesCheckMask: 0x02
    readonly property int _skipActiveConnectionsCheckMask: 0x04
    property int _closeChecksToSkip: 0
    function performCloseChecks() {
        if (!(_closeChecksToSkip & _skipUnsavedMissionCheckMask) && !checkForUnsavedMission()) {
            return false
        }
        if (!(_closeChecksToSkip & _skipPendingParameterWritesCheckMask) && !checkForPendingParameterWrites()) {
            return false
        }
        if (!(_closeChecksToSkip & _skipActiveConnectionsCheckMask) && !checkForActiveConnections()) {
            return false
        }
        finishCloseProcess()
        return true
    }

    property string closeDialogTitle: qsTr("Close %1").arg(QGroundControl.appName)

    function checkForUnsavedMission() {
        if (planView._planMasterController.dirty) {
            showMessageDialog(closeDialogTitle,
                              qsTr("You have a mission edit in progress which has not been saved/sent. If you close you will lose changes. Are you sure you want to close?"),
                              Dialog.Yes | Dialog.No,
                              function() { _closeChecksToSkip |= _skipUnsavedMissionCheckMask; performCloseChecks() })
            return false
        } else {
            return true
        }
    }

    function checkForPendingParameterWrites() {
        for (var index=0; index<QGroundControl.multiVehicleManager.vehicles.count; index++) {
            if (QGroundControl.multiVehicleManager.vehicles.get(index).parameterManager.pendingWrites) {
                mainWindow.showMessageDialog(closeDialogTitle,
                    qsTr("You have pending parameter updates to a vehicle. If you close you will lose changes. Are you sure you want to close?"),
                    Dialog.Yes | Dialog.No,
                    function() { _closeChecksToSkip |= _skipPendingParameterWritesCheckMask; performCloseChecks() })
                return false
            }
        }
        return true
    }

    function checkForActiveConnections() {
        if (QGroundControl.multiVehicleManager.activeVehicle) {
            mainWindow.showMessageDialog(closeDialogTitle,
                qsTr("There are still active connections to vehicles. Are you sure you want to exit?"),
                Dialog.Yes | Dialog.No,
                function() { _closeChecksToSkip |= _skipActiveConnectionsCheckMask; performCloseChecks() })
            return false
        } else {
            return true
        }
    }

    onClosing: (close) => {
        if (!_forceClose) {
            _closeChecksToSkip = 0
            close.accepted = performCloseChecks()
        }
    }

    background: Rectangle {
        anchors.fill:   parent
        color:          QGroundControl.globalPalette.window
    }

    FlyView { 
        id:                     flyView
        anchors.fill:           parent
    }

    PlanView {
        id:             planView
        anchors.fill:   parent
        visible:        false
    }

    // Permanent sidebar with menu buttons - visible in all views
    Rectangle {
        id:                     permanentSidebar
        anchors.left:           parent.left
        anchors.leftMargin:     ScreenTools.defaultFontPixelWidth
        anchors.verticalCenter: parent.verticalCenter
        width:                  Math.max(ScreenTools.defaultFontPixelWidth * 1.0, sidebarColumn.implicitWidth + (ScreenTools.defaultFontPixelWidth * 0.5))
        height:                 sidebarColumn.implicitHeight + (ScreenTools.defaultFontPixelHeight * 0.6)
        property color _cyan: "#00F0FF"
        property color _panelBg: Qt.rgba(0.02, 0.10, 0.16, 0.7)
        color:                  _panelBg
        radius:                 4
        border.color:           _cyan
        border.width:           1
        z:                      QGroundControl.zOrderTopMost
        // Show only on the Fly map screen to avoid overlapping in Settings/Analyze/Setup/Plan views
        visible:                flyView.visible && !QGroundControl.videoManager.fullScreen

        ColumnLayout {
            id:                     sidebarColumn
            anchors.centerIn:       parent
            anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.15
            spacing:                ScreenTools.defaultFontPixelWidth * 0.2

            SubMenuButton {
                id:                 videoButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/InstrumentValueIcons/camera.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                // borderColor:        permanentSidebar._cyan
                borderWidth:        0
                checked:            mainWindow.activeFlySidebarTool === "video"
                visible:            QGroundControl.settingsManager.videoSettings.visible
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "video"
                        mainWindow.showSettingsTool("Video")
                    }
                }
            }

            SubMenuButton {
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/qmlimages/Plan.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                // borderColor:        permanentSidebar._cyan
                borderWidth:        0
                checked:            mainWindow.activeFlySidebarTool === "plan"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "plan"
                        mainWindow.showPlanView()
                    }
                }
            }

            SubMenuButton {
                id:                 analyzeButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/qmlimages/Analyze.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                // borderColor:        permanentSidebar._cyan
                borderWidth:        0
                visible:            QGroundControl.corePlugin.showAdvancedUI
                checked:            mainWindow.activeFlySidebarTool === "analyze"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "analyze"
                        mainWindow.showAnalyzeTool()
                    }
                }
            }

            SubMenuButton {
                id:                 setupButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/VehicleConfig.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                // borderColor:        permanentSidebar._cyan
                borderWidth:        0
                checked:            mainWindow.activeFlySidebarTool === "setup"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "setup"
                        mainWindow.showVehicleConfig()
                    }
                }
            }

            SubMenuButton {
                id:                 aboutButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/InstrumentValueIcons/question.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                borderWidth:        0
                checked:            mainWindow.activeFlySidebarTool === "about"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "about"
                        mainWindow.showAboutTool()
                    }
                }
            }

            SubMenuButton {
                id:                 settingsButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/qmlimages/Gears.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                imageColor:         undefined
                // borderColor:        permanentSidebar._cyan
                borderWidth:        0
                visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
                checked:            mainWindow.activeFlySidebarTool === "settings"
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        mainWindow.activeFlySidebarTool = "settings"
                        mainWindow.showSettingsTool()
                    }
                }
            }
        }
    }

    footer: LogReplayStatusBar {
        visible: QGroundControl.settingsManager.flyViewSettings.showLogReplayStatusBar.rawValue
    }

    MessageDialog {
        id:                 showTouchAreasNotification
        title:              qsTr("Debug Touch Areas")
        text:               qsTr("Touch Area display toggled")
        buttons:            MessageDialog.Ok
    }

    MessageDialog {
        id:                 advancedModeOnConfirmation
        title:              qsTr("Advanced Mode")
        text:               QGroundControl.corePlugin.showAdvancedUIMessage
        buttons:            MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Yes) {
                QGroundControl.corePlugin.showAdvancedUI = true
            }
        }
    }

    MessageDialog {
        id:                 advancedModeOffConfirmation
        title:              qsTr("Advanced Mode")
        text:               qsTr("Turn off Advanced Mode?")
        buttons:            MessageDialog.Yes | MessageDialog.No
        onButtonClicked: function (button, role) {
            if (button === MessageDialog.Yes) {
                QGroundControl.corePlugin.showAdvancedUI = false
            }
        }
    }

    function showToolSelectDialog() {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.showIndicatorDrawer(toolSelectComponent, null)
        }
    }

    Component {
        id: toolSelectComponent

        ToolIndicatorPage {
            id:         toolSelectDialog
            //title:      qsTr("Select Tool")

            property real _toolButtonHeight:    ScreenTools.defaultFontPixelHeight * 3
            property real _margins:             ScreenTools.defaultFontPixelWidth

            contentComponent: Component {
                ColumnLayout {
                    spacing: ScreenTools.defaultFontPixelWidth

                    SubMenuButton {
                        height:             toolSelectDialog._toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Plan Flight")
                        imageResource:      "/qmlimages/Plan.svg"
                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                mainWindow.closeIndicatorDrawer()
                                mainWindow.showPlanView()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 analyzeButton
                        height:             toolSelectDialog._toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Analyze Tools")
                        imageResource:      "/qmlimages/Analyze.svg"
                        visible:            QGroundControl.corePlugin.showAdvancedUI
                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                mainWindow.closeIndicatorDrawer()
                                mainWindow.showAnalyzeTool()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 setupButton
                        height:             toolSelectDialog._toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Vehicle Configuration")
                        imageResource:      "/res/VehicleConfig.svg"
                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                mainWindow.closeIndicatorDrawer()
                                mainWindow.showVehicleConfig()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 settingsButton
                        height:             toolSelectDialog._toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Application Settings")
                        imageResource:      "/qmlimages/Gears.svg"
                        imageColor:         undefined
                        visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                drawer.close()
                                mainWindow.showSettingsTool()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 closeButton
                        height:             toolSelectDialog._toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Close %1").arg(QGroundControl.appName)
                        imageResource:      "/res/cancel.svg"
                        visible:            mainWindow.visibility === Window.FullScreen
                        onClicked: {
                            if (mainWindow.allowViewSwitch()) {
                                mainWindow.finishCloseProcess()
                            }
                        }
                    }

                    ColumnLayout {
                        id:                     versionColumnLayout
                        Layout.preferredWidth:  parent.width
                        spacing:                0
                        Layout.alignment:       Qt.AlignHCenter

                        QGCLabel {
                            id:                     versionLabel
                            text:                   qsTr("%1 Version").arg(QGroundControl.appName)
                            font.pointSize:         ScreenTools.smallFontPointSize
                            wrapMode:               QGCLabel.WordWrap
                            Layout.maximumWidth:    parent.width
                            Layout.alignment:       Qt.AlignHCenter
                        }

                        QGCLabel {
                            text:                   QGroundControl.qgcVersion
                            font.pointSize:         ScreenTools.smallFontPointSize
                            wrapMode:               QGCLabel.WrapAnywhere
                            Layout.maximumWidth:    parent.width
                            Layout.alignment:       Qt.AlignHCenter
                        }

                        QGCLabel {
                            text:                   QGroundControl.qgcAppDate
                            font.pointSize:         ScreenTools.smallFontPointSize
                            wrapMode:               QGCLabel.WrapAnywhere
                            Layout.maximumWidth:    parent.width
                            Layout.alignment:       Qt.AlignHCenter
                            visible:                QGroundControl.qgcDailyBuild

                            QGCMouseArea {
                                anchors.topMargin:  -(parent.y - versionLabel.y)
                                anchors.fill:       parent

                                onClicked: (mouse) => {
                                    if (mouse.modifiers & Qt.ControlModifier) {
                                        QGroundControl.corePlugin.showTouchAreas = !QGroundControl.corePlugin.showTouchAreas
                                        showTouchAreasNotification.open()
                                    } else if (ScreenTools.isMobile || mouse.modifiers & Qt.ShiftModifier) {
                                        mainWindow.closeIndicatorDrawer()
                                        if(!QGroundControl.corePlugin.showAdvancedUI) {
                                            advancedModeOnConfirmation.open()
                                        } else {
                                            advancedModeOffConfirmation.open()
                                        }
                                    }
                                }

                                // This allows you to change this on mobile
                                onPressAndHold: {
                                    QGroundControl.corePlugin.showTouchAreas = !QGroundControl.corePlugin.showTouchAreas
                                    showTouchAreasNotification.open()
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    Rectangle {
        id:             toolDrawer
        anchors.fill:   parent
        visible:        false
        color:          Qt.rgba(0, 0, 0, 0.6)

        property var backIcon
        property string toolTitle
        property alias toolSource:  toolDrawerLoader.source
        property var toolIcon

        onVisibleChanged: {
            if (!toolDrawer.visible) {
                toolDrawerLoader.source = ""
                mainWindow._pendingSettingsPage = ""
            }
        }

        // This need to block click event leakage to underlying map.
        MouseArea {
            anchors.fill: parent
            onClicked: (mouse) => {
                var pt = toolDrawerPanel.mapFromItem(this, mouse.x, mouse.y)
                if (pt.x < 0 || pt.y < 0 || pt.x > toolDrawerPanel.width || pt.y > toolDrawerPanel.height) {
                    if (mainWindow.allowViewSwitch()) {
                        toolDrawer.visible = false
                    }
                }
            }
        }

        Rectangle {
            id:                         toolDrawerPanel
            anchors.horizontalCenter:   parent.horizontalCenter
            anchors.verticalCenter:     parent.verticalCenter
            width:                      Math.min(parent.width  * 0.9,  ScreenTools.defaultFontPixelWidth  * 100)
            height:                     Math.min(parent.height * 0.65, ScreenTools.defaultFontPixelHeight * 30)
            radius:                     permanentSidebar.radius
            color:                      permanentSidebar._panelBg
            border.color:               permanentSidebar._cyan
            border.width:               1
            antialiasing:               true
            clip:                       true

            Rectangle {
                id:             toolDrawerToolbar
                anchors.left:   parent.left
                anchors.right:  parent.right
                anchors.top:    parent.top
                height:         ScreenTools.toolbarHeight
                color:          qgcPal.toolbarBackground
                border.color:   qgcPal.toolbarDivider
                border.width:   0

                Rectangle {
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.bottom: parent.bottom
                    height:         1
                    color:          qgcPal.toolbarDivider
                }

                RowLayout {
                    id:                 toolDrawerToolbarLayout
                    anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                    anchors.left:       parent.left
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    spacing:            ScreenTools.defaultFontPixelWidth

                    QGCLabel {
                        id:             toolbarDrawerText
                        text:           toolDrawer.toolTitle
                        font.pointSize: ScreenTools.largeFontPointSize
                    }
                }
            }

            Loader {
                id:                 toolDrawerLoader
                anchors.left:       parent.left
                anchors.right:      parent.right
                anchors.top:        toolDrawerToolbar.bottom
                anchors.bottom:     parent.bottom
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.4

                onItemChanged: {
                    if (item && _pendingSettingsPage !== "") {
                        if (item.showSettingsPage) {
                            item.showSettingsPage(_pendingSettingsPage)
                            _pendingSettingsPage = ""
                        }
                    }
                }

                Connections {
                    target:                 toolDrawerLoader.item
                    ignoreUnknownSignals:   true
                    function onPopout() { toolDrawer.visible = false }
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Critical Vehicle Message Popup

    function showCriticalVehicleMessage(message) {
        closeIndicatorDrawer()
        if (criticalVehicleMessagePopup.visible || QGroundControl.videoManager.fullScreen) {
            // We received additional warning message while an older warning message was still displayed.
            // When the user close the older one drop the message indicator tool so they can see the rest of them.
            criticalVehicleMessagePopup.additionalCriticalMessagesReceived = true
        } else {
            criticalVehicleMessagePopup.criticalVehicleMessage      = message
            criticalVehicleMessagePopup.additionalCriticalMessagesReceived = false
            criticalVehicleMessagePopup.open()
        }
    }

    Popup {
        id:                 criticalVehicleMessagePopup
        y:                  ScreenTools.toolbarHeight + ScreenTools.defaultFontPixelHeight
        x:                  Math.round((mainWindow.width - width) * 0.5)
        width:              mainWindow.width  * 0.55
        height:             criticalVehicleMessageText.contentHeight + ScreenTools.defaultFontPixelHeight * 2
        modal:              false
        focus:              true

        property alias  criticalVehicleMessage:             criticalVehicleMessageText.text
        property bool   additionalCriticalMessagesReceived: false

        background: Rectangle {
            anchors.fill:   parent
            color:          Qt.rgba(qgcPal.alertBackground.r,
                                   qgcPal.alertBackground.g,
                                   qgcPal.alertBackground.b,
                                   0.7)
            radius:         ScreenTools.defaultFontPixelHeight
            border.color:   qgcPal.alertBorder
            border.width:   2

            Rectangle {
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.top:                parent.top
                anchors.topMargin:          -(height / 2)
                color:                      Qt.rgba(qgcPal.alertBackground.r,
                                                   qgcPal.alertBackground.g,
                                                   qgcPal.alertBackground.b,
                                                   0.7)
                radius:                     ScreenTools.defaultFontPixelHeight * 0.5
                border.color:               qgcPal.alertBorder
                border.width:               1
                width:                      vehicleWarningLabel.contentWidth + _margins
                height:                     vehicleWarningLabel.contentHeight + _margins

                property real _margins: ScreenTools.defaultFontPixelHeight * 0.25

                QGCLabel {
                    id:                 vehicleWarningLabel
                    anchors.centerIn:   parent
                    text:               qsTr("Vehicle Error")
                    font.pointSize:     ScreenTools.smallFontPointSize
                    color:              qgcPal.alertText
                }
            }

            Rectangle {
                id:                         additionalErrorsIndicator
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.bottom:             parent.bottom
                anchors.bottomMargin:       -(height / 2)
                color:                      qgcPal.alertBackground
                radius:                     ScreenTools.defaultFontPixelHeight * 0.5
                border.color:               qgcPal.alertBorder
                border.width:               1
                width:                      additionalErrorsLabel.contentWidth + _margins
                height:                     additionalErrorsLabel.contentHeight + _margins
                visible:                    criticalVehicleMessagePopup.additionalCriticalMessagesReceived

                property real _margins: ScreenTools.defaultFontPixelHeight * 0.25

                QGCLabel {
                    id:                 additionalErrorsLabel
                    anchors.centerIn:   parent
                    text:               qsTr("Additional errors received")
                    font.pointSize:     ScreenTools.smallFontPointSize
                    color:              qgcPal.alertText
                }
            }
        }

        QGCLabel {
            id:                 criticalVehicleMessageText
            width:              criticalVehicleMessagePopup.width - ScreenTools.defaultFontPixelHeight
            anchors.centerIn:   parent
            wrapMode:           Text.WordWrap
            color:              qgcPal.alertText
            textFormat:         TextEdit.RichText
        }

        MouseArea {
            anchors.fill: parent
            onClicked: {
                criticalVehicleMessagePopup.close()
                if (criticalVehicleMessagePopup.additionalCriticalMessagesReceived) {
                    criticalVehicleMessagePopup.additionalCriticalMessagesReceived = false;
                    flyView.dropMainStatusIndicatorTool();
                } else {
                    QGroundControl.multiVehicleManager.activeVehicle.resetErrorLevelMessages();
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Indicator Drawer

    function showIndicatorDrawer(drawerComponent, indicatorItem) {
        indicatorDrawer.sourceComponent = drawerComponent
        indicatorDrawer.indicatorItem = indicatorItem
        indicatorDrawer.open()
    }

    function closeIndicatorDrawer() {
        indicatorDrawer.close()
    }

    Popup {
        id:             indicatorDrawer
        x:              calcXPosition()
        y:              ScreenTools.toolbarHeight + _margins
        leftInset:      0
        rightInset:     0
        topInset:       0
        bottomInset:    0
        padding:        _margins * 2
        visible:        false
        modal:          true
        focus:          true
        closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside

        property var sourceComponent
        property var indicatorItem

        property bool _expanded:    false
        property real _margins:     ScreenTools.defaultFontPixelHeight / 4

        function calcXPosition() {
            if (indicatorItem) {
                var xCenter = indicatorItem.mapToItem(mainWindow.contentItem, indicatorItem.width / 2, 0).x
                return Math.max(_margins, Math.min(xCenter - (contentItem.implicitWidth / 2), mainWindow.contentItem.width - contentItem.implicitWidth - _margins - (indicatorDrawer.padding * 2) - (ScreenTools.defaultFontPixelHeight / 2)))
            } else {
                return _margins
            }
        }

        onOpened: {
            _expanded                               = false;
            indicatorDrawerLoader.sourceComponent   = indicatorDrawer.sourceComponent
        }
        onClosed: {
            _expanded                               = false
            indicatorItem                           = undefined
            indicatorDrawerLoader.sourceComponent   = undefined
        }

        background: Item {
            Rectangle {
                id:             backgroundRect
                anchors.fill:   parent
                color:          QGroundControl.globalPalette.window
                radius:         indicatorDrawer._margins
                opacity:        0.85
            }

            Rectangle {
                anchors.horizontalCenter:   backgroundRect.right
                anchors.verticalCenter:     backgroundRect.top
                width:                      ScreenTools.largeFontPixelHeight
                height:                     width
                radius:                     width / 2
                color:                      QGroundControl.globalPalette.button
                border.color:               QGroundControl.globalPalette.buttonText
                visible:                    indicatorDrawerLoader.item && indicatorDrawerLoader.item.showExpand && !indicatorDrawer._expanded

                QGCLabel {
                    anchors.centerIn:   parent
                    text:               ">"
                    color:              QGroundControl.globalPalette.buttonText
                }  

                QGCMouseArea {
                    fillItem: parent
                    onClicked: indicatorDrawer._expanded = true
                }
            }
        }

        contentItem: QGCFlickable {
            id:             indicatorDrawerLoaderFlickable
            implicitWidth:  Math.min(mainWindow.contentItem.width - (2 * indicatorDrawer._margins) - (indicatorDrawer.padding * 2), indicatorDrawerLoader.width)
            implicitHeight: Math.min(mainWindow.contentItem.height - ScreenTools.toolbarHeight - (2 * indicatorDrawer._margins) - (indicatorDrawer.padding * 2), indicatorDrawerLoader.height)
            contentWidth:   indicatorDrawerLoader.width
            contentHeight:  indicatorDrawerLoader.height

            Loader {
                id: indicatorDrawerLoader

                Binding {
                    target:     indicatorDrawerLoader.item
                    property:   "expanded"
                    value:      indicatorDrawer._expanded
                }

                Binding {
                    target:     indicatorDrawerLoader.item
                    property:   "drawer"
                    value:      indicatorDrawer
                }
            }
        }
    }

    // We have to create the popup windows for the Analyze pages here so that the creation context is rooted
    // to mainWindow. Otherwise if they are rooted to the AnalyzeView itself they will die when the analyze viewSwitch
    // closes.

    function createrWindowedAnalyzePage(title, source) {
        var windowedPage = windowedAnalyzePage.createObject(mainWindow)
        windowedPage.title = title
        windowedPage.source = source
    }

    Component {
        id: windowedAnalyzePage

        Window {
            width:      ScreenTools.defaultFontPixelWidth  * 100
            height:     ScreenTools.defaultFontPixelHeight * 40
            visible:    true

            property alias source: loader.source

            Rectangle {
                color:          QGroundControl.globalPalette.window
                anchors.fill:   parent

                Loader {
                    id:             loader
                    anchors.fill:   parent
                    onLoaded:       item.popped = true
                }
            }

            onClosing: {
                visible = false
                source = ""
            }
        }
    }
}
