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
    signal shortcutPressed(shortcut :string, result: string)

    //-------------------------------------------------------------------------
    //-- Global Scope Shortcuts

    ListModel{
        property var parameterSetter: QGroundControl.corePlugin.parameterSetter
        property var onboardComputersManager: globals.activeVehicle.autopilotPlugin.onboardComputersManager
        property var activeCamera: globals.activeVehicle.cameraManager.currentCameraInstance
        property var currentComputerId: onboardComputersManager.currentComputerComponent
        id:globalShortcuts
        ListElement{
            description:"Open Parameters"
            sequence:"Ctrl+P"
            action:function(){

                if(globalShortcuts.onboardComputersManager == undefined){
                    showVehicleConfigParametersPage()
                } else {
                    showVehicleConfigParametersPageComponent(qsTr("Component ")+ globalShortcuts.currentComputerId)
                }
                return ""
            }
        }
        ListElement{
            description:"Switch tracking selection"
            sequence:"Ctrl+T"
            action:function(){

                if( globalShortcuts.activeCamera == undefined ||
                        !globalShortcuts.activeCamera.hasTracking){
                    return
                }
                let tracking = globalShortcuts.activeCamera.trackingEnabled
                globalShortcuts.activeCamera.trackingEnabled = !globalShortcuts.activeCamera.trackingEnabled;
                if(!globalShortcuts.activeCamera.trackingEnabled){
                    globalShortcuts.activeCamera.stopTracking()
                }
                return (tracking + " -> " + !tracking)
            }
        }

        ListElement{
            description:"Zoom In"
            sequence: "Ctrl++"
            action:function(){
                if(globalShortcuts.activeCamera == undefined){
                    return
                }
                let zoom = globalShortcuts.activeCamera.zoomLevel
                let newZoom = zoom +1
                if (newZoom > 32 ){
                    return
                }

                globalShortcuts.activeCamera.zoomLevel = newZoom
                return (zoom + "->" + newZoom)
            }
        }

        ListElement{
            description:"Zoom Out"
            sequence: "Ctrl+-"
            action:function(){
                if(globalShortcuts.activeCamera == undefined){
                    return
                }
                let zoom = globalShortcuts.activeCamera.zoomLevel
                let newZoom = zoom -1
                if (newZoom < 0 ){
                    return
                }
                globalShortcuts.activeCamera.zoomLevel = newZoom
                return (zoom + "->" + newZoom)
            }
        }

        ListElement{
            description:"Reset zoom"
            sequence:"Ctrl+="
            action:function(){
                if( globalShortcuts.activeCamera == undefined){
                    return
                }
                let zoom = globalShortcuts.activeCamera.zoomLevel
                globalShortcuts.activeCamera.zoomLevel = 1
                return (zoom + "-> 1")
            }
        }
        ListElement{
            description:"Toggle recording"
            sequence:"Ctrl+Space"
            action:function(){
                if( globalShortcuts.activeCamera === undefined){
                    return;
                }
                globalShortcuts.activeCamera.toggleVideoRecording();
                return ""
            }
        }

        ListElement{
            description:"Exposure Up"
            sequence:"Ctrl+E"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let exposure = Math.floor(paramSetter.getParameter(compId, "CAM_EXPOSURE"))
                let newExposure = exposure + 100
                if( newExposure > 20000){
                    return
                }
                paramSetter.setParameter(compId, "CAM_EXPOSURE", newExposure)
                return exposure + " -> " + newExposure

            }
        }

        ListElement{
            description:"Exposure Down"
            sequence:"Ctrl+Shift+E"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let exposure = Math.floor(paramSetter.getParameter(compId, "CAM_EXPOSURE"))
                let newExposure = exposure - 100
                if( newExposure < 0){
                    return
                }
                paramSetter.setParameter(compId, "CAM_EXPOSURE", newExposure)
                return exposure + " -> " + newExposure
            }
        }

        ListElement{
            description:"Guide type"
            sequence:"Ctrl+G"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let guideType = paramSetter.getParameter(compId, "MISSN_GUID_TYPE")
                let newGuideType = guideType
                if(newGuideType != 1){
                    newGuideType = 1
                } else {
                    newGuideType = 0
                }
                paramSetter.setParameter(compId, "MISSN_GUID_TYPE", newGuideType)
                return guideType + "->" + newGuideType
            }
        }

        ListElement{
            description:"Terminal velocity Up"
            sequence:"Ctrl+S"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let velocity = Math.floor(paramSetter.getParameter(compId, "MISSN_TERM_VEL"))
                let newVelocity = velocity + 5
                if( newVelocity > 100){
                    return
                }
                paramSetter.setParameter(compId,"MISSN_TERM_VEL",newVelocity)
                return velocity + " -> " + newVelocity
            }
        }
        ListElement{
            description:"Terminal velocity Down"
            sequence:"Ctrl+Shift+S"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let velocity = Math.floor(paramSetter.getParameter(compId, "MISSN_TERM_VEL"))
                let newVelocity = velocity - 5
                if(newVelocity < 0){
                    return
                }
                paramSetter.setParameter(compId,"MISSN_TERM_VEL",newVelocity)
                return velocity + " -> " + newVelocity
            }
        }

        ListElement{
            description:"Mission Autonomy Switch"
            sequence:"Ctrl+Shift+A"
            action:function(){
                let compId = globalShortcuts.currentComputerId
                let paramSetter = globalShortcuts.parameterSetter
                let mode = Math.floor(paramSetter.getParameter(compId, "MISSN_AUTONOMY"))
                let newMode = mode + 1
                if (newMode > 2){
                    newMode = 0
                }
                paramSetter.setParameter(compId,"MISSN_AUTONOMY",newMode)
                return mode + " -> " + newMode

            }
        }

        ListElement{
            description:"SCR_USER1 Switch"
            sequence:"Ctrl+Tab"
            action:function(){
                let paramSetter = globalShortcuts.parameterSetter
                let mode = Math.floor(paramSetter.getParameter(1, "SCR_USER1"))
                let newMode = mode + 1
                if (newMode > 2){
                    newMode = 0
                }
                paramSetter.setParameter(1,"SCR_USER1",newMode)
                return qsTr(mode +" -> "+ newMode)
            }
        }
    }

    Instantiator{
        model:globalShortcuts
        delegate: Action{
            shortcut: sequence
            onTriggered:{
                console.log("Shortcut pressed: "+sequence)
                let result = action()
                shortcutPressed(description,result)
            }

        }
    }



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

    function checkForVGM(compsInfo){
        for (let i = 0; i < compsInfo.length; i++) {
            let item = compsInfo[i];  // each item is a QVariantMap

            let vendor = item["Vendor Id"];
            if (vendor === 0xf4) {
                return true;
            }
        }
        return false;
    }
    
    function showVehicleConfigParametersPageComponent(comp = ""){
        showVehicleConfigParametersPage();
        toolDrawerLoader.item.showParametersPanelComponent(comp)
    }

    function showPlanView() {
        flyView.visible = false
        planView.visible = true
    }

    function showFlyView() {
        flyView.visible = true
        planView.visible = false
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
    }

    function showVehicleConfig() {
        showTool(qsTr("Vehicle Configuration"), "qrc:/qml/QGroundControl/VehicleSetup/SetupView.qml", "/qmlimages/Gears.svg")
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

    function showSettingsTool(settingsPage = "") {
        showTool(qsTr("Application Settings"), "qrc:/qml/QGroundControl/Controls/AppSettings.qml", "/res/QGCLogoWhite")
        if (settingsPage !== "") {
            toolDrawerLoader.item.showSettingsPage(settingsPage)
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
                        imageResource:      "/qmlimages/Gears.svg"
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
                        imageResource:      "/res/QGCLogoFull.svg"
                        imageColor:         "transparent"
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
        color:          qgcPal.window

        property var backIcon
        property string toolTitle
        property alias toolSource:  toolDrawerLoader.source
        property var toolIcon

        onVisibleChanged: {
            if (!toolDrawer.visible) {
                toolDrawerLoader.source = ""
            }
        }

        // This need to block click event leakage to underlying map.
        DeadMouseArea {
            anchors.fill: parent
        }

        Rectangle {
            id:             toolDrawerToolbar
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.top:    parent.top
            height:         ScreenTools.toolbarHeight
            color:          qgcPal.toolbarBackground

            RowLayout {
                id:                 toolDrawerToolbarLayout
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                anchors.left:       parent.left
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCLabel {
                    font.pointSize: ScreenTools.largeFontPointSize
                    text:           "<"
                }

                QGCLabel {
                    id:             toolbarDrawerText
                    text:           qsTr("Exit") + " " + toolDrawer.toolTitle
                    font.pointSize: ScreenTools.largeFontPointSize
                }
            }

            QGCMouseArea {
                anchors.fill: toolDrawerToolbarLayout
                onClicked: {
                    if (mainWindow.allowViewSwitch()) {
                        toolDrawer.visible = false
                    }
                }
            }
        }

        Loader {
            id:             toolDrawerLoader
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.top:    toolDrawerToolbar.bottom
            anchors.bottom: parent.bottom

            Connections {
                target:                 toolDrawerLoader.item
                ignoreUnknownSignals:   true
                function onPopout() { toolDrawer.visible = false }
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
            color:          qgcPal.alertBackground
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            border.color:   qgcPal.alertBorder
            border.width:   2

            Rectangle {
                anchors.horizontalCenter:   parent.horizontalCenter
                anchors.top:                parent.top
                anchors.topMargin:          -(height / 2)
                color:                      qgcPal.alertBackground
                radius:                     ScreenTools.defaultFontPixelHeight * 0.25
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
                radius:                     ScreenTools.defaultFontPixelHeight * 0.25
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
