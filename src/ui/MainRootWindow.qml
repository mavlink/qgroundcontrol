/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick          2.11
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.11
import QtQuick.Window   2.11

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0

/// @brief Native QML top level window
/// All properties defined here are visible to all QML pages.
ApplicationWindow {
    id:             mainWindow
    minimumWidth:   ScreenTools.isMobile ? Screen.width  : Math.min(ScreenTools.defaultFontPixelWidth * 100, Screen.width)
    minimumHeight:  ScreenTools.isMobile ? Screen.height : Math.min(ScreenTools.defaultFontPixelWidth * 50, Screen.height)
    visible:        true

    Component.onCompleted: {
        //-- Full screen on mobile or tiny screens
        if (ScreenTools.isMobile || Screen.height / ScreenTools.realPixelDensity < 120) {
            mainWindow.showFullScreen()
        } else {
            width   = ScreenTools.isMobile ? Screen.width  : Math.min(250 * Screen.pixelDensity, Screen.width)
            height  = ScreenTools.isMobile ? Screen.height : Math.min(150 * Screen.pixelDensity, Screen.height)
        }

        // Start the sequence of first run prompt(s)
        firstRunPromptManager.nextPrompt()
    }

    QtObject {
        id: firstRunPromptManager

        property var currentDialog:     null
        property var rgPromptIds:       QGroundControl.corePlugin.firstRunPromptsToShow()
        property int nextPromptIdIndex: 0

        onRgPromptIdsChanged: console.log(QGroundControl.corePlugin, QGroundControl.corePlugin.firstRunPromptsToShow())

        function clearNextPromptSignal() {
            if (currentDialog) {
                currentDialog.closed.disconnect(nextPrompt)
            }
        }

        function nextPrompt() {
            if (nextPromptIdIndex < rgPromptIds.length) {
                currentDialog = showPopupDialogFromSource(QGroundControl.corePlugin.firstRunPromptResource(rgPromptIds[nextPromptIdIndex]))
                currentDialog.closed.connect(nextPrompt)
                nextPromptIdIndex++
            } else {
                currentDialog = null
                showPreFlightChecklistIfNeeded()
            }
        }
    }

    property var                _rgPreventViewSwitch:       [ false ]

    readonly property real      _topBottomMargins:          ScreenTools.defaultFontPixelHeight * 0.5

    //-------------------------------------------------------------------------
    //-- Global Scope Variables

    /// Current active Vehicle
    property var                activeVehicle:                  QGroundControl.multiVehicleManager.activeVehicle
    property string             formatedMessage:                activeVehicle ? activeVehicle.formatedMessage : ""
    /// Indicates usable height between toolbar and footer
    property real               availableHeight:                mainWindow.height - mainWindow.header.height - mainWindow.footer.height

    property var                currentPlanMissionItem:         planMasterControllerPlanView ? planMasterControllerPlanView.missionController.currentPlanViewItem : null
    property var                planMasterControllerPlanView:   null
    property var                planMasterControllerFlyView:    null

    readonly property string    navButtonWidth:                 ScreenTools.defaultFontPixelWidth * 24
    readonly property real      defaultTextHeight:              ScreenTools.defaultFontPixelHeight
    readonly property real      defaultTextWidth:               ScreenTools.defaultFontPixelWidth

    /// Default color palette used throughout the UI
    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    //-------------------------------------------------------------------------
    //-- Actions

    signal armVehicleRequest
    signal disarmVehicleRequest
    signal vtolTransitionToFwdFlightRequest
    signal vtolTransitionToMRFlightRequest
    signal showPreFlightChecklistIfNeeded

    //-------------------------------------------------------------------------
    //-- Global Scope Functions

    /// Prevent view switching
    function pushPreventViewSwitch() {
        _rgPreventViewSwitch.push(true)
    }

    /// Allow view switching
    function popPreventViewSwitch() {
        if (_rgPreventViewSwitch.length == 1) {
            console.warn("mainWindow.popPreventViewSwitch called when nothing pushed")
            return
        }
        _rgPreventViewSwitch.pop()
    }

    /// @return true: View switches are not currently allowed
    function preventViewSwitch() {
        return _rgPreventViewSwitch[_rgPreventViewSwitch.length - 1]
    }

    function viewSwitch(currentToolbar) {
        settingsWindow.visible  = false
        setupWindow.visible     = false
        analyzeWindow.visible   = false
        flightView.visible      = false
        planViewLoader.visible  = false
        toolbar.currentToolbar  = currentToolbar
    }

    function showFlyView() {
        if (!flightView.visible) {
            mainWindow.showPreFlightChecklistIfNeeded()
        }
        viewSwitch(toolbar.flyViewToolbar)
        flightView.visible = true
    }

    function showPlanView() {
        viewSwitch(toolbar.planViewToolbar)
        planViewLoader.visible = true
    }

    function showAnalyzeView() {
        viewSwitch(toolbar.simpleToolbar)
        analyzeWindow.visible = true
    }

    function showSetupView() {
        viewSwitch(toolbar.simpleToolbar)
        setupWindow.visible = true
    }

    function showSettingsView() {
        viewSwitch(toolbar.simpleToolbar)
        settingsWindow.visible = true
    }

    //-------------------------------------------------------------------------
    //-- Global simple message dialog

    function showMessageDialog(title, text) {
        var dialog = simpleMessageDialog.createObject(mainWindow, { title: title, text: text })
        dialog.open()
    }

    Component {
        id: simpleMessageDialog

        MessageDialog {
            standardButtons:    StandardButton.Ok
            modality:           Qt.ApplicationModal
            visible:            false
        }
    }

    /// Saves main window position and size
    MainWindowSavedState {
        window: mainWindow
    }

    //-------------------------------------------------------------------------
    //-- Global complex dialog

    /// Shows a QGCViewDialogContainer based dialog
    ///     @param component The dialog contents
    ///     @param title Title for dialog
    ///     @param charWidth Width of dialog in characters
    ///     @param buttons Buttons to show in dialog using StandardButton enum

    readonly property int showDialogFullWidth:      -1  ///< Use for full width dialog
    readonly property int showDialogDefaultWidth:   40  ///< Use for default dialog width

    function showComponentDialog(component, title, charWidth, buttons) {
        if (mainWindowDialog.visible) {
            console.warn(("showComponentDialog called while dialog is already visible"))
            return
        }
        var dialogWidth = charWidth === showDialogFullWidth ? mainWindow.width : ScreenTools.defaultFontPixelWidth * charWidth
        mainWindowDialog.width = dialogWidth
        mainWindowDialog.dialogComponent = component
        mainWindowDialog.dialogTitle = title
        mainWindowDialog.dialogButtons = buttons
        mainWindow.pushPreventViewSwitch()
        mainWindowDialog.open()
        if (buttons & StandardButton.Cancel || buttons & StandardButton.Close || buttons & StandardButton.Discard || buttons & StandardButton.Abort || buttons & StandardButton.Ignore) {
            mainWindowDialog.closePolicy = Popup.NoAutoClose;
            mainWindowDialog.interactive = false;
        } else {
            mainWindowDialog.closePolicy = Popup.CloseOnEscape | Popup.CloseOnPressOutside;
            mainWindowDialog.interactive = true;
        }
    }

    Drawer {
        id:             mainWindowDialog
        y:              mainWindow.header.height
        height:         mainWindow.height - mainWindow.header.height
        edge:           Qt.RightEdge
        interactive:    false
        background: Rectangle {
            color:  qgcPal.windowShadeDark
        }
        property var    dialogComponent: null
        property var    dialogButtons: null
        property string dialogTitle: ""
        Loader {
            id:             dlgLoader
            anchors.fill:   parent
            onLoaded: {
                item.setupDialogButtons()
            }
        }
        onOpened: {
            dlgLoader.source = "QGCViewDialogContainer.qml"
        }
        onClosed: {
            //console.log("View switch ok")
            mainWindow.popPreventViewSwitch()
            dlgLoader.source = ""
        }
    }

    // Dialogs based on QGCPopupDialog

    function showPopupDialogFromComponent(component, properties) {
        var dialog = popupDialogContainerComponent.createObject(mainWindow, { dialogComponent: component, dialogProperties: properties })
        dialog.open()
        return dialog
    }

    function showPopupDialogFromSource(source, properties) {
        var dialog = popupDialogContainerComponent.createObject(mainWindow, { dialogSource: source, dialogProperties: properties })
        dialog.open()
        return dialog
    }

    Component {
        id: popupDialogContainerComponent
        QGCPopupDialogContainer { }
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

    // On attempting an application close we check for:
    //  Unsaved missions - then
    //  Pending parameter writes - then
    //  Active connections
    onClosing: {
        if (!_forceClose) {
            unsavedMissionCloseDialog.check()
            close.accepted = false
        }
    }

    MessageDialog {
        id:                 unsavedMissionCloseDialog
        title:              qsTr("%1 close").arg(QGroundControl.appName)
        text:               qsTr("You have a mission edit in progress which has not been saved/sent. If you close you will lose changes. Are you sure you want to close?")
        standardButtons:    StandardButton.Yes | StandardButton.No
        modality:           Qt.ApplicationModal
        visible:            false
        onYes:              pendingParameterWritesCloseDialog.check()
        function check() {
            if (planMasterControllerPlanView && planMasterControllerPlanView.dirty) {
                unsavedMissionCloseDialog.open()
            } else {
                pendingParameterWritesCloseDialog.check()
            }
        }
    }

    MessageDialog {
        id:                 pendingParameterWritesCloseDialog
        title:              qsTr("%1 close").arg(QGroundControl.appName)
        text:               qsTr("You have pending parameter updates to a vehicle. If you close you will lose changes. Are you sure you want to close?")
        standardButtons:    StandardButton.Yes | StandardButton.No
        modality:           Qt.ApplicationModal
        visible:            false
        onYes:              activeConnectionsCloseDialog.check()
        function check() {
            for (var index=0; index<QGroundControl.multiVehicleManager.vehicles.count; index++) {
                if (QGroundControl.multiVehicleManager.vehicles.get(index).parameterManager.pendingWrites) {
                    pendingParameterWritesCloseDialog.open()
                    return
                }
            }
            activeConnectionsCloseDialog.check()
        }
    }

    MessageDialog {
        id:                 activeConnectionsCloseDialog
        title:              qsTr("%1 close").arg(QGroundControl.appName)
        text:               qsTr("There are still active connections to vehicles. Are you sure you want to exit?")
        standardButtons:    StandardButton.Yes | StandardButton.Cancel
        modality:           Qt.ApplicationModal
        visible:            false
        onYes:              finishCloseProcess()
        function check() {
            if (QGroundControl.multiVehicleManager.activeVehicle) {
                activeConnectionsCloseDialog.open()
            } else {
                finishCloseProcess()
            }
        }
    }

    //-------------------------------------------------------------------------
    /// Main, full window background (Fly View)
    background: Item {
        id:             rootBackground
        anchors.fill:   parent
    }

    //-------------------------------------------------------------------------
    /// Toolbar
    header: MainToolBar {
        id:         toolbar
        height:     ScreenTools.toolbarHeight
        visible:    !QGroundControl.videoManager.fullScreen
    }

    footer: LogReplayStatusBar {
        visible: QGroundControl.settingsManager.flyViewSettings.showLogReplayStatusBar.rawValue
    }

    //-------------------------------------------------------------------------
    /// Fly View
    FlyView {
        id:             flightView
        anchors.fill:   parent
    }

    //-------------------------------------------------------------------------
    /// Plan View
    Loader {
        id:             planViewLoader
        anchors.fill:   parent
        visible:        false
        source:         "PlanView.qml"
    }

    //-------------------------------------------------------------------------
    /// Settings
    Loader {
        id:             settingsWindow
        anchors.fill:   parent
        visible:        false
        source:         "AppSettings.qml"
    }

    //-------------------------------------------------------------------------
    /// Setup
    Loader {
        id:             setupWindow
        anchors.fill:   parent
        visible:        false
        source:         "SetupView.qml"
    }

    //-------------------------------------------------------------------------
    /// Analyze
    Loader {
        id:             analyzeWindow
        anchors.fill:   parent
        visible:        false
        source:         "AnalyzeView.qml"
    }

    //-------------------------------------------------------------------------
    //   @brief Loader helper for any child, no matter how deep, to display elements
    //   on top of the main window.
    //   This is DEPRECATED. Use Popup instead.
    Loader {
        id: rootLoader
        anchors.centerIn: parent
    }

    //-------------------------------------------------------------------------
    //-- Vehicle Messages

    function formatMessage(message) {
        message = message.replace(new RegExp("<#E>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        message = message.replace(new RegExp("<#I>", "g"), "color: " + qgcPal.warningText + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        message = message.replace(new RegExp("<#N>", "g"), "color: " + qgcPal.text + "; font: " + (ScreenTools.defaultFontPointSize.toFixed(0) - 1) + "pt monospace;");
        return message;
    }

    function showVehicleMessages() {
        if(!vehicleMessageArea.visible) {
            if(QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                messageText.text = formatMessage(activeVehicle.formatedMessages)
                //-- Hack to scroll to last message
                for (var i = 0; i < activeVehicle.messageCount; i++)
                    messageFlick.flick(0,-5000)
                activeVehicle.resetMessages()
            } else {
                messageText.text = qsTr("No Messages")
            }
            vehicleMessageArea.open()
        }
    }

    onFormatedMessageChanged: {
        if(vehicleMessageArea.visible) {
            messageText.append(formatMessage(formatedMessage))
            //-- Hack to scroll down
            messageFlick.flick(0,-500)
        }
    }

    Popup {
        id:                 vehicleMessageArea
        width:              mainWindow.width  * 0.666
        height:             mainWindow.height * 0.666
        modal:              true
        focus:              true
        x:                  Math.round((mainWindow.width  - width)  * 0.5)
        y:                  Math.round((mainWindow.height - height) * 0.5)
        closePolicy:        Popup.CloseOnEscape | Popup.CloseOnPressOutside
        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.window
            border.color:   qgcPal.text
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
        }
        QGCFlickable {
            id:                 messageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight
            anchors.fill:       parent
            contentHeight:      messageText.height
            contentWidth:       messageText.width
            pixelAligned:       true
            clip:               true
            TextEdit {
                id:             messageText
                readOnly:       true
                textFormat:     TextEdit.RichText
                color:          qgcPal.text
            }
        }
        //-- Dismiss Vehicle Messages
        QGCColoredImage {
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            anchors.top:        parent.top
            anchors.right:      parent.right
            width:              ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.5 : ScreenTools.defaultFontPixelHeight
            height:             width
            sourceSize.height:  width
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            color:              qgcPal.text
            MouseArea {
                anchors.fill:       parent
                anchors.margins:    ScreenTools.isMobile ? -ScreenTools.defaultFontPixelHeight : 0
                onClicked: {
                    vehicleMessageArea.close()
                }
            }
        }
        //-- Clear Messages
        QGCColoredImage {
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            height:             ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.5 : ScreenTools.defaultFontPixelHeight
            width:              height
            sourceSize.height:   height
            source:             "/res/TrashDelete.svg"
            fillMode:           Image.PreserveAspectFit
            mipmap:             true
            smooth:             true
            color:              qgcPal.text
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    if(QGroundControl.multiVehicleManager.activeVehicleAvailable) {
                        activeVehicle.clearMessages();
                        vehicleMessageArea.close()
                    }
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- System Messages

    property var    _messageQueue:      []
    property string _systemMessage:     ""

    function showVehicleMessage(message) {
        vehicleMessageArea.close()
        if(systemMessageArea.visible || QGroundControl.videoManager.fullScreen) {
            _messageQueue.push(message)
        } else {
            _systemMessage = message
            systemMessageArea.open()
        }
    }

    Popup {
        id:                 systemMessageArea
        y:                  ScreenTools.defaultFontPixelHeight
        x:                  Math.round((mainWindow.width - width) * 0.5)
        width:              mainWindow.width  * 0.55
        height:             ScreenTools.defaultFontPixelHeight * 6
        modal:              false
        focus:              true
        closePolicy:        Popup.CloseOnEscape

        background: Rectangle {
            anchors.fill:   parent
            color:          qgcPal.alertBackground
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            border.color:   qgcPal.alertBorder
            border.width:   2
        }

        onOpened: {
            systemMessageText.text = mainWindow._systemMessage
        }

        onClosed: {
            //-- Are there messages in the waiting queue?
            if(mainWindow._messageQueue.length) {
                mainWindow._systemMessage = ""
                //-- Show all messages in queue
                for (var i = 0; i < mainWindow._messageQueue.length; i++) {
                    var text = mainWindow._messageQueue[i]
                    if(i) mainWindow._systemMessage += "<br>"
                    mainWindow._systemMessage += text
                }
                //-- Clear it
                mainWindow._messageQueue = []
                systemMessageArea.open()
            } else {
                mainWindow._systemMessage = ""
            }
        }

        Flickable {
            id:                 systemMessageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            anchors.fill:       parent
            contentHeight:      systemMessageText.height
            contentWidth:       systemMessageText.width
            boundsBehavior:     Flickable.StopAtBounds
            pixelAligned:       true
            clip:               true
            TextEdit {
                id:             systemMessageText
                width:          systemMessageArea.width - systemMessageClose.width - (ScreenTools.defaultFontPixelHeight * 2)
                anchors.centerIn: parent
                readOnly:       true
                textFormat:     TextEdit.RichText
                font.pointSize: ScreenTools.defaultFontPointSize
                font.family:    ScreenTools.demiboldFontFamily
                wrapMode:       TextEdit.WordWrap
                color:          qgcPal.alertText
            }
        }

        //-- Dismiss Critical Message
        QGCColoredImage {
            id:                 systemMessageClose
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            anchors.top:        parent.top
            anchors.right:      parent.right
            width:              ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.5 : ScreenTools.defaultFontPixelHeight
            height:             width
            sourceSize.height:  width
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.alertText
            MouseArea {
                anchors.fill:       parent
                anchors.margins:    -ScreenTools.defaultFontPixelHeight
                onClicked: {
                    systemMessageArea.close()
                }
            }
        }

        //-- More text below indicator
        QGCColoredImage {
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            anchors.bottom:     parent.bottom
            anchors.right:      parent.right
            width:              ScreenTools.isMobile ? ScreenTools.defaultFontPixelHeight * 1.5 : ScreenTools.defaultFontPixelHeight
            height:             width
            sourceSize.height:  width
            source:             "/res/ArrowDown.svg"
            fillMode:           Image.PreserveAspectFit
            visible:            systemMessageText.lineCount > 5
            color:              qgcPal.alertText
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    systemMessageFlick.flick(0,-500)
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Indicator Popups

    function showPopUp(item, dropItem) {
        indicatorDropdown.currentIndicator = dropItem
        indicatorDropdown.currentItem = item
        indicatorDropdown.open()
    }

    function hidePopUp() {
        indicatorDropdown.close()
        indicatorDropdown.currentItem = null
        indicatorDropdown.currentIndicator = null
    }

    Popup {
        id:             indicatorDropdown
        y:              ScreenTools.defaultFontPixelHeight
        modal:          true
        focus:          true
        closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside
        property var    currentItem:        null
        property var    currentIndicator:   null
        background: Rectangle {
            width:  loader.width
            height: loader.height
            color:  Qt.rgba(0,0,0,0)
        }
        Loader {
            id:             loader
            onLoaded: {
                var centerX = mainWindow.contentItem.mapFromItem(indicatorDropdown.currentItem, 0, 0).x - (loader.width * 0.5)
                if((centerX + indicatorDropdown.width) > (mainWindow.width - ScreenTools.defaultFontPixelWidth)) {
                    centerX = mainWindow.width - indicatorDropdown.width - ScreenTools.defaultFontPixelWidth
                }
                indicatorDropdown.x = centerX
            }
        }
        onOpened: {
            loader.sourceComponent = indicatorDropdown.currentIndicator
        }
        onClosed: {
            loader.sourceComponent = null
            indicatorDropdown.currentIndicator = null
        }
    }
}
