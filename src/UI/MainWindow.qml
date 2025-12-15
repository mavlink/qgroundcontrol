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
import QtQuick.LocalStorage

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
    property bool   loggedIn: false
    property string authMode: "login"
    property string _authError: ""
    property string currentUserEmail: ""

    onLoggedInChanged: {
        if (!loggedIn && profilePopup.visible) {
            profilePopup.close()
        }
    }

    Component.onCompleted: {
        if (loggedIn) {
            firstRunPromptManager.nextPrompt()
        }
        ensureAuthTables()
        QGroundControl.settingsManager.adsbVehicleManagerSettings.adsbServerConnectEnabled.setRawValue(false)
    }

    function db() {
        return LocalStorage.openDatabaseSync("IGCSAuth", "1.0", "IGCS auth", 1000000)
    }

    function ensureAuthTables() {
        var d = db()
        d.transaction(function(tx) {
            tx.executeSql('CREATE TABLE IF NOT EXISTS users (email TEXT PRIMARY KEY NOT NULL, password_hash TEXT NOT NULL, salt TEXT NOT NULL, created_at INTEGER NOT NULL)')
        })
    }

    function hashPassword(email, password, salt) {
        return Qt.md5(email + ':' + password + ':' + salt)
    }

    function performLogin() {
        var email = emailField.text.trim()
        var password = passwordField.text
        if (email.length === 0 || password.length === 0) {
            _authError = qsTr("Please enter email and password")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        var d = db()
        var ok = false
        d.transaction(function(tx) {
            var rs = tx.executeSql('SELECT email, password_hash, salt FROM users WHERE email = ?', [email])
            if (rs.rows.length === 1) {
                var row = rs.rows.item(0)
                var h = hashPassword(email, password, row.salt)
                ok = (h === row.password_hash)
            }
        })
        if (!ok) {
            _authError = qsTr("Incorrect email or password")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        errorLabel.visible = false
        loggedIn = true
        currentUserEmail = email
        QGroundControl.linkManager.enableAutoConnect()
        QGroundControl.settingsManager.adsbVehicleManagerSettings.adsbServerConnectEnabled.setRawValue(true)
        firstRunPromptManager.nextPrompt()
    }

    function isValidEmail(e) {
        var re = /^[^\s@]+@[^\s@]+\.[^\s@]+$/
        return re.test(e)
    }

    function performSignup() {
        var email = signupEmail.text.trim()
        var password = signupPassword.text
        var confirm = signupConfirmPassword.text
        if (!isValidEmail(email)) {
            _authError = qsTr("Enter a valid email address")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        if (password.length < 8) {
            _authError = qsTr("Password must be at least 8 characters")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        if (password !== confirm) {
            _authError = qsTr("Passwords do not match")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        var salt = String(new Date().getTime())
        var hash = hashPassword(email, password, salt)
        var d = db()
        var inserted = false
        d.transaction(function(tx) {
            try {
                tx.executeSql('INSERT INTO users (email, password_hash, salt, created_at) VALUES (?,?,?,?)', [email, hash, salt, Date.now()])
                inserted = true
            } catch (e) {
                inserted = false
            }
        })
        if (!inserted) {
            _authError = qsTr("Account already exists")
            errorLabel.text = _authError
            errorLabel.visible = true
            return
        }
        errorLabel.visible = false
        authMode = "login"
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
        closeTransientPopups()
        flyView.visible = false
        planView.visible = true
        activeFlySidebarTool = "plan"
    }

    function showFlyView() {
        closeTransientPopups()
        flyView.visible = true
        planView.visible = false
        activeFlySidebarTool = ""
    }

    function showTool(toolTitle, toolSource, toolIcon) {
        closeTransientPopups()
        toolDrawer.backIcon     = flyView.visible ? "/qmlimages/PaperPlane.svg" : "/qmlimages/Plan.svg"
        toolDrawer.toolTitle    = toolTitle
        toolDrawer.toolSource   = toolSource
        toolDrawer.toolIcon     = toolIcon
        toolDrawer.visible      = true
    }

    function showAnalyzeTool() {
        closeTransientPopups()
        showTool(qsTr("Analyze Tools"), "qrc:/qml/QGroundControl/AnalyzeView/AnalyzeView.qml", "/qmlimages/Analyze.svg")
        activeFlySidebarTool = "analyze"
    }

    function showVehicleConfig() {
        closeTransientPopups()
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
        closeTransientPopups()
        showTool(qsTr("About"), "qrc:/qml/QGroundControl/AppSettings/AboutTool.qml", "/InstrumentValueIcons/question.svg")
        activeFlySidebarTool = "about"
    }

    property string _pendingSettingsPage: ""

    function showSettingsTool(settingsPage = "") {
        var title = settingsPage === "Video" ? qsTr("Video") : qsTr("Application Settings")
        showTool(title, "qrc:/qml/QGroundControl/Controls/AppSettings.qml", "/res/QGCLogoWhite")
        activeFlySidebarTool = settingsPage === "Video" ? "video" : "settings"
        if (settingsPage !== "") {
            _pendingSettingsPage = settingsPage
            if (toolDrawerLoader.item) {
                toolDrawerLoader.item.showSettingsPage(settingsPage)
                _pendingSettingsPage = ""
            }
        }
    }

    function closeTransientPopups() {
        if (profilePopup.visible) {
            profilePopup.close()
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
                id:                 profileButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_Profile.svg"
                sourceSize:         Qt.size(ScreenTools.defaultFontPixelHeight * 1.15, ScreenTools.defaultFontPixelHeight * 1.15)
                borderWidth:        0
                checked:            false
                onClicked: {
                    if (profilePopup.visible) {
                        profilePopup.close()
                    } else {
                        var pt = profileButton.mapToItem(Overlay.overlay, 0, 0)
                        profilePopup.x = pt.x + profileButton.width + ScreenTools.defaultFontPixelWidth
                        profilePopup.y = pt.y
                        profilePopup.open()
                    }
                }
            }
            QGCLabel { text: qsTr("Profile"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize }

            SubMenuButton {
                id:                 videoButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_Video.svg"
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
            QGCLabel { text: qsTr("video"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize; visible: QGroundControl.settingsManager.videoSettings.visible }

            SubMenuButton {
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_Plan.svg"
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
            QGCLabel { text: qsTr("plan"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize }

            SubMenuButton {
                id:                 analyzeButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_Analyze.svg"
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
            QGCLabel { text: qsTr("Analyze"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize; visible: QGroundControl.corePlugin.showAdvancedUI }

            SubMenuButton {
                id:                 setupButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_VehicleConfig.svg"
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
            QGCLabel { text: qsTr("Configuration"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize }

            SubMenuButton {
                id:                 aboutButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_About.svg"
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
            QGCLabel { text: qsTr("About"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize }

            SubMenuButton {
                id:                 settingsButton
                height:             ScreenTools.defaultFontPixelHeight * 1.6
                Layout.fillWidth:   true
                text:               ""
                imageResource:      "/res/Sidebar_ApplicationSettings.svg"
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
            QGCLabel { text: qsTr("Settings"); Layout.alignment: Qt.AlignHCenter; color: qgcPal.buttonText; font.pointSize: ScreenTools.smallFontPointSize; visible: !QGroundControl.corePlugin.options.combineSettingsAndSetup }
        }
    }

    Popup {
        id:                 profilePopup
        modal:              true
        focus:              true
        parent:             Overlay.overlay
        closePolicy:        Popup.CloseOnEscape | Popup.CloseOnPressOutside
        width:              Math.min(mainWindow.width * 0.4, Math.max(ScreenTools.defaultFontPixelWidth * 20, contentItem.implicitWidth + ScreenTools.defaultFontPixelWidth * 2))
        background: Rectangle { color: qgcPal.window; radius: ScreenTools.defaultFontPixelHeight * 0.3; border.width: 1; border.color: qgcPal.text }
        contentItem: Column {
            spacing: ScreenTools.defaultFontPixelHeight * 0.5
            width: profilePopup.width - ScreenTools.defaultFontPixelWidth * 2
            padding: ScreenTools.defaultFontPixelWidth

            QGCLabel {
                text: mainWindow.loggedIn && mainWindow.currentUserEmail.length > 0
                      ? qsTr("User: ") + mainWindow.currentUserEmail
                      : qsTr("User: Guest")
                wrapMode: Text.WrapAnywhere
            }
            QGCButton {
                text: qsTr("Change Password")
                enabled: mainWindow.loggedIn
                onClicked: changePasswordDialogComponent.createObject(mainWindow).open()
            }
            QGCButton {
                text: qsTr("Sign out")
                onClicked: {
                    mainWindow.loggedIn = false
                    mainWindow.currentUserEmail = ""
                    mainWindow.authMode = "login"
                    QGroundControl.linkManager.disableAutoConnect()
                    QGroundControl.settingsManager.adsbVehicleManagerSettings.adsbServerConnectEnabled.setRawValue(false)
                    profilePopup.close()
                }
            }
        }
    }

    Component {
        id: changePasswordDialogComponent

        QGCPopupDialog {
            title: qsTr("Change Password")
            buttons: Dialog.Apply | Dialog.Close

            ColumnLayout {
                Layout.fillWidth: true
                spacing: ScreenTools.defaultFontPixelHeight / 2

                QGCLabel { text: qsTr("Email") }
                QGCTextField { Layout.fillWidth: true; text: mainWindow.currentUserEmail; enabled: false }

                QGCLabel { text: qsTr("Current Password") }
                QGCTextField { id: curPass; Layout.fillWidth: true; echoMode: TextInput.Password }

                QGCLabel { text: qsTr("New Password") }
                QGCTextField { id: newPass; Layout.fillWidth: true; echoMode: TextInput.Password }

                QGCLabel { text: qsTr("Confirm Password") }
                QGCTextField { id: confPass; Layout.fillWidth: true; echoMode: TextInput.Password }

                QGCLabel { id: changePassError; Layout.fillWidth: true; color: QGroundControl.globalPalette.alertText; visible: false }
            }

            onAccepted: {
                var email = mainWindow.currentUserEmail
                var cur = curPass.text
                var np = newPass.text
                var cp = confPass.text
                if (!mainWindow.loggedIn || email.length === 0) { changePassError.text = qsTr("Not logged in"); changePassError.visible = true; preventClose = true; return }
                var d = mainWindow.db()
                var ok = false
                var salt = ""
                d.transaction(function(tx) {
                    var rs = tx.executeSql('SELECT password_hash, salt FROM users WHERE email = ?', [email])
                    if (rs.rows.length === 1) {
                        var row = rs.rows.item(0)
                        var h = mainWindow.hashPassword(email, cur, row.salt)
                        ok = (h === row.password_hash)
                        salt = row.salt
                    }
                })
                if (!ok) { changePassError.text = qsTr("Incorrect current password"); changePassError.visible = true; preventClose = true; return }
                if (np.length < 8) { changePassError.text = qsTr("Password must be at least 8 characters"); changePassError.visible = true; preventClose = true; return }
                if (np !== cp) { changePassError.text = qsTr("Passwords do not match"); changePassError.visible = true; preventClose = true; return }
                var newSalt = String(new Date().getTime())
                var newHash = mainWindow.hashPassword(email, np, newSalt)
                var updated = false
                d.transaction(function(tx) {
                    tx.executeSql('UPDATE users SET password_hash = ?, salt = ? WHERE email = ?', [newHash, newSalt, email])
                    updated = true
                })
                if (!updated) { changePassError.text = qsTr("Update failed"); changePassError.visible = true; preventClose = true; return }
                changePassError.visible = false
            }
        }
    }

    Connections {
        target: flyView
        function onVisibleChanged() {
            if (!flyView.visible && profilePopup.visible) {
                profilePopup.close()
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
                        imageResource:      "/res/Sidebar_Plan.svg"
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
                        imageResource:      "/res/Sidebar_Analyze.svg"
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
                        imageResource:      "/res/Sidebar_VehicleConfig.svg"
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
                        imageResource:      "/res/Sidebar_ApplicationSettings.svg"
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
        property alias toolSourceComponent: toolDrawerLoader.sourceComponent
        property var toolIcon

        onVisibleChanged: {
            if (!toolDrawer.visible) {
                toolDrawerLoader.source = ""
                toolDrawerLoader.sourceComponent = null
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
            width:                      Math.max(ScreenTools.defaultFontPixelWidth * 60, Math.min(parent.width * 0.95, ScreenTools.defaultFontPixelWidth * 120))
            height:                     Math.max(ScreenTools.defaultFontPixelHeight * 25, Math.min(parent.height * 0.65, ScreenTools.defaultFontPixelHeight * 30))
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

    Rectangle {
        id:             loginOverlay
        anchors.fill:   parent
        color:          Qt.rgba(0.0, 0.09, 0.15, 1)
        z:              QGroundControl.zOrderTopMost + 1
        visible:        !loggedIn

        Rectangle {
            id:                         loginPanel
            anchors.horizontalCenter:   parent.horizontalCenter
            anchors.verticalCenter:     parent.verticalCenter
            width:                      Math.min(parent.width  * 0.9,  ScreenTools.defaultFontPixelWidth  * 80)
            height:                     Math.min(parent.height * 0.85, ScreenTools.defaultFontPixelHeight * 40)
            radius:                     4
            color:                      Qt.rgba(0.02, 0.10, 0.16, 0.94)
            border.color:               "#00F0FF"
            border.width:               1
            antialiasing:               true
            z:                          1
            clip:                       true

            ColumnLayout {
                anchors.fill:       parent
                anchors.margins:    ScreenTools.defaultFontPixelHeight
                spacing:            ScreenTools.defaultFontPixelHeight * 0.6

                Image {
                    id:                     igLogo
                    Layout.alignment:       Qt.AlignHCenter
                    Layout.preferredWidth:  Math.min(ScreenTools.defaultFontPixelWidth * 40, loginPanel.width * 0.5)
                    Layout.preferredHeight: Layout.preferredWidth * 0.4
                    source:                 "/res/IGCSFly.svg"
                    fillMode:               Image.PreserveAspectFit
                    sourceSize.height:      Layout.preferredHeight
                    smooth:                 true
                    mipmap:                 true
                    cache:                  false
                    onStatusChanged:        { if (status === Image.Error) { source = "file:///C:/IGCS/resources/ig-gcs-fly.svg" } }
                }

                

                Item { Layout.fillWidth: true; height: ScreenTools.defaultFontPixelHeight * 0.5 }

                Rectangle { Layout.fillWidth: true; height: 1; color: "#00F0FF"; opacity: 0.25 }

                Item { Layout.fillWidth: true; visible: authMode === "login" }
                QGCLabel { text: qsTr("SYSTEM LOGIN"); Layout.alignment: Qt.AlignHCenter; color: "#00F0FF" }

                // Login fields
                Item { Layout.fillWidth: true; visible: authMode === "login" }
                QGCLabel { text: qsTr("EMAIL ADDRESS"); visible: authMode === "login" }
                Item {
                    Layout.fillWidth:   true
                    visible:            authMode === "login"
                    height:             ScreenTools.defaultFontPixelHeight * 2.4
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(0.02, 0.10, 0.16, 0.9); border.color: "#00F0FF"; border.width: 1 }
                    RowLayout {
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCColoredImage { Layout.alignment: Qt.AlignVCenter; source: "/InstrumentValueIcons/at-symbol.svg"; color: "#00F0FF"; width: ScreenTools.defaultFontPixelHeight; height: width; fillMode: Image.PreserveAspectFit; sourceSize.height: height }
                        QGCTextField { id: emailField; Layout.fillWidth: true; placeholderText: qsTr("operator@igcs.mil"); inputMethodHints: Qt.ImhEmailCharactersOnly }
                    }
                }

                QGCLabel { text: qsTr("PASSWORD"); visible: authMode === "login" }
                Item {
                    Layout.fillWidth:   true
                    visible:            authMode === "login"
                    height:             ScreenTools.defaultFontPixelHeight * 2.4
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(0.02, 0.10, 0.16, 0.9); border.color: "#00F0FF"; border.width: 1 }
                    RowLayout {
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCColoredImage { Layout.alignment: Qt.AlignVCenter; source: "/res/LockClosed.svg"; color: "#00F0FF"; width: ScreenTools.defaultFontPixelHeight; height: width; fillMode: Image.PreserveAspectFit; sourceSize.height: height }
                        QGCTextField { id: passwordField; Layout.fillWidth: true; placeholderText: qsTr("Enter password"); echoMode: TextInput.Password; onAccepted: mainWindow.performLogin() }
                    }
                }

                // Signup fields
                Item { Layout.fillWidth: true; visible: authMode === "signup" }
                QGCLabel { text: qsTr("REGISTER ACCOUNT"); Layout.alignment: Qt.AlignHCenter; color: "#00F0FF"; visible: authMode === "signup" }
                QGCLabel { text: qsTr("EMAIL ADDRESS"); visible: authMode === "signup" }
                Item {
                    Layout.fillWidth:   true
                    visible:            authMode === "signup"
                    height:             ScreenTools.defaultFontPixelHeight * 2.4
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(0.02, 0.10, 0.16, 0.9); border.color: "#00F0FF"; border.width: 1 }
                    RowLayout {
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCColoredImage { Layout.alignment: Qt.AlignVCenter; source: "/InstrumentValueIcons/at-symbol.svg"; color: "#00F0FF"; width: ScreenTools.defaultFontPixelHeight; height: width; fillMode: Image.PreserveAspectFit; sourceSize.height: height }
                        QGCTextField { id: signupEmail; Layout.fillWidth: true; placeholderText: qsTr("operator@igcs.mil"); inputMethodHints: Qt.ImhEmailCharactersOnly }
                    }
                }
                QGCLabel { text: qsTr("PASSWORD"); visible: authMode === "signup" }
                Item {
                    Layout.fillWidth:   true
                    visible:            authMode === "signup"
                    height:             ScreenTools.defaultFontPixelHeight * 2.4
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(0.02, 0.10, 0.16, 0.9); border.color: "#00F0FF"; border.width: 1 }
                    RowLayout {
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCColoredImage { Layout.alignment: Qt.AlignVCenter; source: "/res/LockClosed.svg"; color: "#00F0FF"; width: ScreenTools.defaultFontPixelHeight; height: width; fillMode: Image.PreserveAspectFit; sourceSize.height: height }
                        QGCTextField { id: signupPassword; Layout.fillWidth: true; placeholderText: qsTr("Enter password"); echoMode: TextInput.Password }
                    }
                }
                QGCLabel { text: qsTr("CONFIRM PASSWORD"); visible: authMode === "signup" }
                Item {
                    Layout.fillWidth:   true
                    visible:            authMode === "signup"
                    height:             ScreenTools.defaultFontPixelHeight * 2.4
                    Rectangle { anchors.fill: parent; radius: 8; color: Qt.rgba(0.02, 0.10, 0.16, 0.9); border.color: "#00F0FF"; border.width: 1 }
                    RowLayout {
                        anchors.fill:   parent
                        anchors.margins: ScreenTools.defaultFontPixelWidth
                        spacing:        ScreenTools.defaultFontPixelWidth
                        QGCColoredImage { Layout.alignment: Qt.AlignVCenter; source: "/res/LockClosed.svg"; color: "#00F0FF"; width: ScreenTools.defaultFontPixelHeight; height: width; fillMode: Image.PreserveAspectFit; sourceSize.height: height }
                        QGCTextField { id: signupConfirmPassword; Layout.fillWidth: true; placeholderText: qsTr("Re-enter password"); echoMode: TextInput.Password; onAccepted: mainWindow.performSignup() }
                    }
                }

                QGCLabel {
                    id:                 errorLabel
                    Layout.fillWidth:   true
                    color:              QGroundControl.globalPalette.alertText
                    visible:            false
                    wrapMode:           Text.WordWrap
                }

                RowLayout {
                    Layout.fillWidth:   true
                    spacing:            ScreenTools.defaultFontPixelWidth

                    Item {
                        Layout.fillWidth:   true
                        height:             ScreenTools.defaultFontPixelHeight * 2.4

                        QGCButton {
                            id:                 loginButton
                            anchors.fill:       parent
                            text:               ""
                            iconSource:         ""
                            primary:            true
                            neon:               true
                            pill:               false
                            backRadius:         6
                            heightFactor:       1.0
                            neonBorderWidth:    1
                            neonColor:          "#00F0FF"
                            onClicked:          authMode === "login" ? mainWindow.performLogin() : mainWindow.performSignup()
                        }

                        RowLayout {
                            anchors.centerIn:   parent
                            spacing:            ScreenTools.defaultFontPixelWidth
                            QGCColoredImage {
                                source:             authMode === "login" ? "/res/ArrowRight.svg" : "/InstrumentValueIcons/user-add.svg"
                                color:              "#00F0FF"
                                width:              ScreenTools.defaultFontPixelHeight * 1.2
                                height:             width
                                fillMode:           Image.PreserveAspectFit
                                sourceSize.height:  height
                            }
                        QGCLabel {
                            text:               authMode === "login" ? qsTr("LOGIN") : qsTr("REGISTER")
                            font.pointSize:     ScreenTools.defaultFontPointSize
                            font.weight:        Font.DemiBold
                            color:              "#00F0FF"
                            wrapMode:           Text.NoWrap
                            elide:              Text.ElideRight
                        }
                    }

                        Rectangle { width: 12; height: 1; color: "#00F0FF"; opacity: 0.9; anchors.left: parent.left; anchors.top: parent.top }
                        Rectangle { width: 1; height: 12; color: "#00F0FF"; opacity: 0.9; anchors.left: parent.left; anchors.top: parent.top }
                        Rectangle { width: 12; height: 1; color: "#00F0FF"; opacity: 0.9; anchors.right: parent.right; anchors.bottom: parent.bottom }
                        Rectangle { width: 1; height: 12; color: "#00F0FF"; opacity: 0.9; anchors.right: parent.right; anchors.bottom: parent.bottom }
                    }
                }

                Item {
                    Layout.fillWidth:   true
                    height:             ScreenTools.defaultFontPixelHeight * 2
                    QGCLabel {
                        anchors.centerIn:   parent
                        text:               authMode === "login" ? qsTr("DON'T HAVE AN ACCOUNT? REGISTER") : qsTr("ALREADY HAVE AN ACCOUNT? LOGIN")
                        color:              "#00F0FF"
                        horizontalAlignment: Text.AlignHCenter
                        wrapMode:           Text.NoWrap
                        elide:              Text.ElideRight
                    }
                    MouseArea {
                        anchors.fill:   parent
                        cursorShape:    Qt.PointingHandCursor
                        onClicked:      authMode = authMode === "login" ? "signup" : "login"
                    }
                }

                Item { Layout.fillWidth: true; height: ScreenTools.defaultFontPixelHeight }
                QGCLabel {
                    Layout.alignment:   Qt.AlignHCenter
                    text:               qsTr("CLASSIFIED SYSTEM – AUTHORIZED PERSONNEL ONLY")
                    color:              "#00F0FF"
                }
            }
            Rectangle { width: 6; height: 6; color: "#00F0FF"; anchors.left: parent.left; anchors.top: parent.top }
            Rectangle { width: 6; height: 6; color: "#00F0FF"; anchors.right: parent.right; anchors.top: parent.top }
            Rectangle { width: 6; height: 6; color: "#00F0FF"; anchors.left: parent.left; anchors.bottom: parent.bottom }
            Rectangle { width: 6; height: 6; color: "#00F0FF"; anchors.right: parent.right; anchors.bottom: parent.bottom }
        }
        Repeater {
            model: 40
            delegate: Column {
                width: 12
                anchors.top: parent.top
                anchors.bottom: parent.bottom
                x: index * 20
                spacing: 6
                z: -1
                Repeater {
                    model: 50
                    delegate: Text {
                        text: Math.random() > 0.5 ? '0' : '1'
                        color: "#00F0FF"
                        opacity: 0.06
                        font.pointSize: 8
                    }
                }
            }
        }
        QGCLabel {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom:           parent.bottom
            anchors.bottomMargin:     ScreenTools.defaultFontPixelHeight * 0.5
            text:                     qsTr("© IG Drones, All Rights Reserved")
            color:                    "#00F0FF"
            opacity:                  0.6
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
