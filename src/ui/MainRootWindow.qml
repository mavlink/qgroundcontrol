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

    QtObject {
        id: globals

        readonly property var       activeVehicle:                  QGroundControl.multiVehicleManager.activeVehicle
        readonly property real      defaultTextHeight:              ScreenTools.defaultFontPixelHeight
        readonly property real      defaultTextWidth:               ScreenTools.defaultFontPixelWidth
        readonly property var       planMasterControllerFlyView:    flightView.planController
        readonly property var       guidedControllerFlyView:        flightView.guidedController

        property var                planMasterControllerPlanView:   null
        property var                currentPlanMissionItem:         planMasterControllerPlanView ? planMasterControllerPlanView.missionController.currentPlanViewItem : null
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
        toolDrawer.visible      = false
        toolDrawer.toolSource   = ""
        flightView.visible      = false
        planView.visible        = false
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
        planView.visible = true
    }

    function showTool(toolTitle, toolSource, toolIcon) {
        toolDrawer.backIcon     = flightView.visible ? "/qmlimages/PaperPlane.svg" : "/qmlimages/Plan.svg"
        toolDrawer.toolTitle    = toolTitle
        toolDrawer.toolSource   = toolSource
        toolDrawer.toolIcon     = toolIcon
        toolDrawer.visible      = true
    }

    function showAnalyzeTool() {
        showTool(qsTr("Analyze Tools"), "AnalyzeView.qml", "/qmlimages/Analyze.svg")
    }

    function showSetupTool() {
        showTool(qsTr("Vehicle Setup"), "SetupView.qml", "/qmlimages/Gears.svg")
    }

    function showSettingsTool() {
        showTool(qsTr("Application Settings"), "AppSettings.qml", "/res/QGCLogoWhite")
    }

    //-------------------------------------------------------------------------
    //-- Global simple message dialog

    function showMessageDialog(dialogTitle, dialogText) {
        showPopupDialogFromComponent(simpleMessageDialog, { title: dialogTitle, text: dialogText })
    }

    Component {
        id: simpleMessageDialog

        QGCPopupDialog {
            title:      dialogProperties.title
            buttons:    StandardButton.Ok

            ColumnLayout {
                QGCLabel {
                    id:                     textLabel
                    wrapMode:               Text.WordWrap
                    text:                   dialogProperties.text
                    Layout.fillWidth:       true
                    Layout.maximumWidth:    mainWindow.width / 2
                }
            }
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
        var dialogWidth = charWidth === showDialogFullWidth ? mainWindow.width : ScreenTools.defaultFontPixelWidth * charWidth
        var dialog = dialogDrawerComponent.createObject(mainWindow, { width: dialogWidth, dialogComponent: component, dialogTitle: title, dialogButtons: buttons })
        mainWindow.pushPreventViewSwitch()
        dialog.open()
    }

    Component {
        id: dialogDrawerComponent
        QGCViewDialogContainer {
            y:          mainWindow.header.height
            height:     mainWindow.height - mainWindow.header.height
            onClosed:   mainWindow.popPreventViewSwitch()
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
            if (globals.planMasterControllerPlanView && globals.planMasterControllerPlanView.dirty) {
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

    function showToolSelectDialog() {
        if (!mainWindow.preventViewSwitch()) {
            showPopupDialogFromComponent(toolSelectDialogComponent)
        }
    }

    Component {
        id: toolSelectDialogComponent

        QGCPopupDialog {
            id:         toolSelectDialog
            title:      qsTr("Select Tool")
            buttons:    StandardButton.Close

            property real _toolButtonHeight:    ScreenTools.defaultFontPixelHeight * 3
            property real _margins:             ScreenTools.defaultFontPixelWidth

            ColumnLayout {
                width:  innerLayout.width + (_margins * 2)
                height: innerLayout.height + (_margins * 2)

                ColumnLayout {
                    id:             innerLayout
                    Layout.margins: _margins
                    spacing:        ScreenTools.defaultFontPixelWidth

                    SubMenuButton {
                        id:                 setupButton
                        height:             _toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Vehicle Setup")
                        imageColor:         qgcPal.text
                        imageResource:      "/qmlimages/Gears.svg"
                        onClicked: {
                            if (!mainWindow.preventViewSwitch()) {
                                toolSelectDialog.hideDialog()
                                mainWindow.showSetupTool()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 analyzeButton
                        height:             _toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Analyze Tools")
                        imageResource:      "/qmlimages/Analyze.svg"
                        imageColor:         qgcPal.text
                        visible:            QGroundControl.corePlugin.showAdvancedUI
                        onClicked: {
                            if (!mainWindow.preventViewSwitch()) {
                                toolSelectDialog.hideDialog()
                                mainWindow.showAnalyzeTool()
                            }
                        }
                    }

                    SubMenuButton {
                        id:                 settingsButton
                        height:             _toolButtonHeight
                        Layout.fillWidth:   true
                        text:               qsTr("Application Settings")
                        imageResource:      "/res/QGCLogoFull"
                        imageColor:         "transparent"
                        visible:            !QGroundControl.corePlugin.options.combineSettingsAndSetup
                        onClicked: {
                            if (!mainWindow.preventViewSwitch()) {
                                toolSelectDialog.hideDialog()
                                mainWindow.showSettingsTool()
                            }
                        }
                    }

                    ColumnLayout {
                        width:      innerLayout.width
                        spacing:    0

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

                            QGCMouseArea {
                                id:                 easterEggMouseArea
                                anchors.topMargin:  -versionLabel.height
                                anchors.fill:       parent

                                onClicked: {
                                    if (mouse.modifiers & Qt.ControlModifier) {
                                        QGroundControl.corePlugin.showTouchAreas = !QGroundControl.corePlugin.showTouchAreas
                                    } else if (mouse.modifiers & Qt.ShiftModifier) {
                                        if(!QGroundControl.corePlugin.showAdvancedUI) {
                                            advancedModeConfirmation.open()
                                        } else {
                                            QGroundControl.corePlugin.showAdvancedUI = false
                                        }
                                    }
                                }

                                MessageDialog {
                                    id:                 advancedModeConfirmation
                                    title:              qsTr("Advanced Mode")
                                    text:               QGroundControl.corePlugin.showAdvancedUIMessage
                                    standardButtons:    StandardButton.Yes | StandardButton.No
                                    onYes: {
                                        QGroundControl.corePlugin.showAdvancedUI = true
                                        advancedModeConfirmation.close()
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    FlyView {
        id:             flightView
        anchors.fill:   parent
    }

    PlanView {
        id:             planView
        anchors.fill:   parent
        visible:        false
    }

    Drawer {
        id:             toolDrawer
        width:          mainWindow.width
        height:         mainWindow.height
        edge:           Qt.LeftEdge
        dragMargin:     0
        closePolicy:    Drawer.NoAutoClose
        interactive:    false
        visible:        false

        property alias backIcon:    backIcon.source
        property alias toolTitle:   toolbarDrawerText.text
        property alias toolSource:  toolDrawerLoader.source
        property alias toolIcon:    toolIcon.source

        Rectangle {
            id:             toolDrawerToolbar
            anchors.left:   parent.left
            anchors.right:  parent.right
            anchors.top:    parent.top
            height:         ScreenTools.toolbarHeight
            color:          qgcPal.toolbarBackground

            RowLayout {
                anchors.leftMargin: ScreenTools.defaultFontPixelWidth
                anchors.left:       parent.left
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                spacing:            ScreenTools.defaultFontPixelWidth

                QGCColoredImage {
                    id:                     backIcon
                    width:                  ScreenTools.defaultFontPixelHeight * 2
                    height:                 ScreenTools.defaultFontPixelHeight * 2
                    fillMode:               Image.PreserveAspectFit
                    mipmap:                 true
                    color:                  qgcPal.text
                }

                QGCLabel {
                    id:     backTextLabel
                    text:   qsTr("Back")
                }

                QGCLabel {
                    font.pointSize: ScreenTools.largeFontPointSize
                    text:           "<"
                }

                QGCColoredImage {
                    id:                     toolIcon
                    width:                  ScreenTools.defaultFontPixelHeight * 2
                    height:                 ScreenTools.defaultFontPixelHeight * 2
                    fillMode:               Image.PreserveAspectFit
                    mipmap:                 true
                    color:                  qgcPal.text
                }

                QGCLabel {
                    id:             toolbarDrawerText
                    font.pointSize: ScreenTools.largeFontPointSize
                }
            }

            QGCMouseArea {
                anchors.top:        parent.top
                anchors.bottom:     parent.bottom
                x:                  parent.mapFromItem(backIcon, backIcon.x, backIcon.y).x
                width:              (backTextLabel.x + backTextLabel.width) - backIcon.x
                onClicked: {
                    toolDrawer.visible      = false
                    toolDrawer.toolSource   = ""
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
                onPopout:               toolDrawer.visible = false
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Critical Vehicle Message Popup

    property var    _vehicleMessageQueue:      []
    property string _vehicleMessage:     ""

    function showCriticalVehicleMessage(message) {
        indicatorPopup.close()
        if (criticalVehicleMessagePopup.visible || QGroundControl.videoManager.fullScreen) {
            _vehicleMessageQueue.push(message)
        } else {
            _vehicleMessage = message
            criticalVehicleMessagePopup.open()
        }
    }

    Popup {
        id:                 criticalVehicleMessagePopup
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
            criticalVehicleMessageText.text = mainWindow._vehicleMessage
        }

        onClosed: {
            //-- Are there messages in the waiting queue?
            if(mainWindow._vehicleMessageQueue.length) {
                mainWindow._vehicleMessage = ""
                //-- Show all messages in queue
                for (var i = 0; i < mainWindow._vehicleMessageQueue.length; i++) {
                    var text = mainWindow._vehicleMessageQueue[i]
                    if(i) mainWindow._vehicleMessage += "<br>"
                    mainWindow._vehicleMessage += text
                }
                //-- Clear it
                mainWindow._vehicleMessageQueue = []
                criticalVehicleMessagePopup.open()
            } else {
                mainWindow._vehicleMessage = ""
            }
        }

        Flickable {
            id:                 criticalVehicleMessageFlick
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
            anchors.fill:       parent
            contentHeight:      criticalVehicleMessageText.height
            contentWidth:       criticalVehicleMessageText.width
            boundsBehavior:     Flickable.StopAtBounds
            pixelAligned:       true
            clip:               true
            TextEdit {
                id:             criticalVehicleMessageText
                width:          criticalVehicleMessagePopup.width - criticalVehicleMessageClose.width - (ScreenTools.defaultFontPixelHeight * 2)
                anchors.centerIn: parent
                readOnly:       true
                textFormat:     TextEdit.RichText
                font.pointSize: ScreenTools.defaultFontPointSize
                font.family:    ScreenTools.demiboldFontFamily
                wrapMode:       TextEdit.WordWrap
                color:          qgcPal.alertText
            }
        }

        //-- Dismiss Vehicle Message
        QGCColoredImage {
            id:                 criticalVehicleMessageClose
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
                    criticalVehicleMessagePopup.close()
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
            visible:            criticalVehicleMessageText.lineCount > 5
            color:              qgcPal.alertText
            MouseArea {
                anchors.fill:   parent
                onClicked: {
                    criticalVehicleMessageFlick.flick(0,-500)
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Indicator Popups

    function showIndicatorPopup(item, dropItem) {
        indicatorPopup.currentIndicator = dropItem
        indicatorPopup.currentItem = item
        indicatorPopup.open()
    }

    function hideIndicatorPopup() {
        indicatorPopup.close()
        indicatorPopup.currentItem = null
        indicatorPopup.currentIndicator = null
    }

    Popup {
        id:             indicatorPopup
        padding:        ScreenTools.defaultFontPixelWidth * 0.75
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
                var centerX = mainWindow.contentItem.mapFromItem(indicatorPopup.currentItem, 0, 0).x - (loader.width * 0.5)
                if((centerX + indicatorPopup.width) > (mainWindow.width - ScreenTools.defaultFontPixelWidth)) {
                    centerX = mainWindow.width - indicatorPopup.width - ScreenTools.defaultFontPixelWidth
                }
                indicatorPopup.x = centerX
            }
        }
        onOpened: {
            loader.sourceComponent = indicatorPopup.currentIndicator
        }
        onClosed: {
            loader.sourceComponent = null
            indicatorPopup.currentIndicator = null
        }
    }
}
