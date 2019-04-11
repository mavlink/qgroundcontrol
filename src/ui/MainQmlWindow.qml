/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick          2.12
import QtQuick.Controls 2.4
import QtQuick.Dialogs  1.3
import QtQuick.Layouts  1.12

import QGroundControl               1.0
import QGroundControl.Palette       1.0
import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.FlightDisplay 1.0
import QGroundControl.FlightMap     1.0

/// Native QML top level window
ApplicationWindow {
    id:         mainWindow
    width:      1024
    height:     768
    visible:    true

    Component.onCompleted: {
        toolbarIndicators.source = _mainToolbarIndicators
    }

    readonly property real      _topBottomMargins:      ScreenTools.defaultFontPixelHeight * 0.5
    readonly property string    _mainToolbarIndicators: QGroundControl.corePlugin.options.mainToolbarIndicatorsUrl
    readonly property string    _settingsViewSource:    "AppSettings.qml"
    readonly property string    _setupViewSource:       "SetupView.qml"
    readonly property string    _planViewSource:        "PlanView.qml"
    readonly property string    _analyzeViewSource:     !ScreenTools.isMobile ? "AnalyzeView.qml" : "MavlinkConsolePage.qml"

    //-------------------------------------------------------------------------
    //-- Global Scope Variables

    property var                activeVehicle:          QGroundControl.multiVehicleManager.activeVehicle
    property bool               communicationLost:      activeVehicle ? activeVehicle.connectionLost : false
    property string             formatedMessage:        activeVehicle ? activeVehicle.formatedMessage : ""

    readonly property string    navButtonWidth:         ScreenTools.defaultFontPixelWidth * 24
    readonly property real      defaultTextHeight:      ScreenTools.defaultFontPixelHeight
    readonly property real      defaultTextWidth:       ScreenTools.defaultFontPixelWidth

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    //-------------------------------------------------------------------------
    //-- Global Scope Functions

    function disableToolbar() {
        drawer.enabled = false
    }

    function enableToolbar() {
        drawer.enabled = true
    }

    function viewSwitch(isPlanView) {
        enableToolbar()
        drawer.close()
        if(isPlanView) {

        } else {
            if(toolbarIndicators.source !== _mainToolbarIndicators) {
                toolbarIndicators.source  = _mainToolbarIndicators
            }
        }
    }

    function showFlyView() {
        viewSwitch(false)
        mainContentWindow.source = ""
        if(toolbarIndicators.source !== _mainToolbarIndicators) {
            toolbarIndicators.source  = _mainToolbarIndicators
        }
    }

    function showPlanView() {
        viewSwitch(true)
        if (mainContentWindow.source !== _planViewSource) {
            mainContentWindow.source  = _planViewSource
        }
    }

    function showAnalyzeView() {
        viewSwitch(false)
        if (mainContentWindow.source !== _analyzeViewSource) {
            mainContentWindow.source  = _analyzeViewSource
        }
    }

    function showSetupView() {
        viewSwitch(false)
        if (mainContentWindow.source !== _setupViewSource) {
            mainContentWindow.source  = _setupViewSource
        }
    }

    function showSettingsView() {
        viewSwitch(false)
        if (mainContentWindow.source !== _settingsViewSource) {
            mainContentWindow.source  = _settingsViewSource
        }
    }

    //-------------------------------------------------------------------------
    //-- Global simple message dialog

    function showMessageDialog(title, text) {
        if(simpleMessageDialog.visible) {
            simpleMessageDialog.close()
        }
        simpleMessageDialog.title = title
        simpleMessageDialog.text  = text
        simpleMessageDialog.open()
    }

    MessageDialog {
        id:                 simpleMessageDialog
        standardButtons:    StandardButton.Ok
        modality:           Qt.ApplicationModal
        visible:            false
    }

    //-------------------------------------------------------------------------
    //-- Global complex dialog

    /// Shows a QGCViewDialog component
    ///     @param component QGCViewDialog component
    ///     @param title Title for dialog
    ///     @param charWidth Width of dialog in characters
    ///     @param buttons Buttons to show in dialog using StandardButton enum

    readonly property int showDialogFullWidth:      -1  ///< Use for full width dialog
    readonly property int showDialogDefaultWidth:   40  ///< Use for default dialog width

    function showDialog(component, title, charWidth, buttons) {
        var dialogWidth = charWidth === showDialogFullWidth ? mainWindow.width : ScreenTools.defaultFontPixelWidth * charWidth
        mainWindowDialog.width = dialogWidth
        mainWindowDialog.dialogComponent = component
        mainWindowDialog.dialogTitle = title
        mainWindowDialog.dialogButtons = buttons
        mainWindowDialog.open()
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
            dlgLoader.source = ""
        }
    }

    //-------------------------------------------------------------------------
    //-- Weird hack that has to be fixed elsewhere and have this removed

    property bool _forceClose: false

    onClosing: {
        if (!_forceClose) {
            activeConnectionsCloseDialog.check()
            close.accepted = false
        }
    }

    function reallyClose() {
        _forceClose = true
        mainWindow.close()
    }

    function finishCloseProcess() {
        QGroundControl.linkManager.shutdown()
        // The above shutdown causes a flurry of activity as the vehicle components are removed. This in turn
        // causes the Windows Version of Qt to crash if you allow the close event to be accepted. In order to prevent
        // the crash, we ignore the close event and setup a delayed timer to close the window after things settle down.
        if(ScreenTools.isWindows) {
            delayedWindowCloseTimer.start()
        } else {
            reallyClose()
        }
    }

    Timer {
        id:         delayedWindowCloseTimer
        interval:   1500
        running:    false
        repeat:     false
        onTriggered: {
            reallyClose()
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
    //-- Main, full window background
    background: Item {
        id:             rootBackground
        anchors.fill:   parent
        FlightDisplayView {
            id:             flightView
            anchors.fill:   parent
        }
    }

    //-------------------------------------------------------------------------
    //-- Global Indicator Bar
    header: ToolBar {
        height:         ScreenTools.toolbarHeight
        visible:        !QGroundControl.videoManager.fullScreen
        background:     Rectangle {
            color:      qgcPal.globalTheme === QGCPalette.Light ? Qt.rgba(1,1,1,0.8) : Qt.rgba(0,0,0,0.75)
        }
        RowLayout {
            anchors.fill:               parent
            Rectangle {
                height:                 parent.height
                width:                  height
                color:                  qgcPal.brandingPurple
                QGCColoredImage {
                    anchors.centerIn:       parent
                    height:                 ScreenTools.defaultFontPixelHeight * 2
                    width:                  height
                    sourceSize.height:      parent.height
                    fillMode:               Image.PreserveAspectFit
                    source:                 "/res/QGCLogoWhite"
                    color:                  "white"
                }
                MouseArea {
                    anchors.fill:       parent
                    onClicked:{
                        if(drawer.visible) {
                            drawer.close()
                        } else {
                            drawer.open()
                        }
                    }
                }
            }
            Item {
                height:             1
                width:              ScreenTools.defaultFontPixelWidth * 2
            }
            Loader {
                id:                 toolbarIndicators
                height:             parent.height
                Layout.fillWidth:   true
            }
        }
    }

    //-------------------------------------------------------------------------
    // Small parameter download progress bar
    Rectangle {
        x:              0
        y:              header.height
        height:         ScreenTools.toolbarHeight * 0.05
        width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * mainWindow.width : 0
        color:          qgcPal.colorGreen
        visible:        !largeProgressBar.visible
    }

    //-------------------------------------------------------------------------
    // Large parameter download progress bar
    Rectangle {
        id:             largeProgressBar
        x:              0
        y:              header.height
        height:         ScreenTools.toolbarHeight
        width:          mainWindow.width
        color:          qgcPal.window
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: activeVehicle ? activeVehicle.parameterManager.parametersReady : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target:                 QGroundControl.multiVehicleManager
            onActiveVehicleChanged: largeProgressBar._userHide = false
        }
        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          activeVehicle ? activeVehicle.parameterManager.loadProgress * mainWindow.width : 0
            color:          qgcPal.colorGreen
        }
        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading Parameters")
            font.pointSize:     ScreenTools.largeFontPointSize
        }
        QGCLabel {
            anchors.margins:    _margin
            anchors.right:      parent.right
            anchors.bottom:     parent.bottom
            text:               qsTr("Click anywhere to hide")

            property real _margin: ScreenTools.defaultFontPixelWidth * 0.5
        }
        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }

    //-------------------------------------------------------------------------
    //-- Navigation Drawer (Left to Right, on command or using touch gestures)
    Drawer {
        id:         drawer
        y:          header.height
        width:      navButtonWidth
        height:     mainWindow.height - header.height
        background: Rectangle {
            color:  qgcPal.globalTheme === QGCPalette.Light ? "white" : "black"
        }
        ButtonGroup {
            buttons: buttons.children
        }
        ColumnLayout {
            id:                     buttons
            anchors.top:            parent.top
            anchors.topMargin:      ScreenTools.defaultFontPixelHeight * 0.5
            anchors.left:           parent.left
            anchors.right:          parent.right
            spacing:                ScreenTools.defaultFontPixelHeight * 0.5
            QGCToolBarButton {
                text:               "Fly"
                icon.source:        "/qmlimages/PaperPlane.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showFlyView()
                }
            }
            QGCToolBarButton {
                text:               "Plan"
                icon.source:        "/qmlimages/Plan.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showPlanView()
                }
            }
            QGCToolBarButton {
                text:               "Analyze"
                icon.source:        "/qmlimages/Analyze.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showAnalyzeView()
                }
            }
            QGCToolBarButton {
                text:               "Vehicle Setup"
                icon.source:        "/qmlimages/Gears.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showSetupView()
                }
            }
            QGCToolBarButton {
                text:               "Settings"
                icon.source:        "/qmlimages/Gears.svg"
                Layout.fillWidth:   true
                onClicked: {
                    checked = true
                    showSettingsView()
                }
            }
        }
    }

    //-------------------------------------------------------------------------
    //-- Current content
    Loader {
        id:             mainContentWindow
        anchors.fill:   parent
    }

    //-------------------------------------------------------------------------
    //-- Loader helper for any child, no matter how deep can display an element
    //   in the middle of the main window.
    Loader {
        id:             rootLoader
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
        closePolicy:        Popup.CloseOnEscape | Popup.CloseOnPressOutside
        anchors.centerIn:   parent
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
    property string _systemMessage:   ""

    function showMessage(message) {
        vehicleMessageArea.close()
        if(systemMessageArea.visible || QGroundControl.videoManager.fullScreen) {
            _messageQueue.push(message)
        } else {
            _systemMessage = message
            systemMessageArea.open()
        }
    }

    function showMissingParameterOverlay(missingParamName) {
        showError(qsTr("Parameters missing: %1").arg(missingParamName))
    }

    function showFactError(errorMsg) {
        showError(qsTr("Fact error: %1").arg(errorMsg))
    }

    Popup {
        id:                 systemMessageArea
        y:                  ScreenTools.defaultFontPixelHeight
        x:                  (mainWindow.width - width) * 0.5
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
                    mainWindow._systemMessage.append(text)
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
            anchors.margins:    ScreenTools.defaultFontPixelHeight
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
                anchors.margins:    ScreenTools.isMobile ? -ScreenTools.defaultFontPixelHeight : 0
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

    function showPopUp(dropItem, centerX) {
        indicatorDropdown.centerX = centerX
        indicatorDropdown.currentIndicator = dropItem
        indicatorDropdown.open()
    }

    Popup {
        id:             indicatorDropdown
        y:              ScreenTools.defaultFontPixelHeight
        modal:          true
        focus:          true
        closePolicy:    Popup.CloseOnEscape | Popup.CloseOnPressOutside
        property var    currentIndicator: null
        property real   centerX: 0
        background: Rectangle {
            width:  loader.width
            height: loader.height
            color:  Qt.rgba(0,0,0,0)
        }
        Loader {
            id:             loader
            onLoaded: {
                indicatorDropdown.x = mapFromGlobal(indicatorDropdown.centerX, 0).x
                console.log(indicatorDropdown.x)
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

