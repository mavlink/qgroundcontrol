import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:             control
    width:          controlIndicatorIconGCS.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    activeVehicle:                          QGroundControl.multiVehicleManager.activeVehicle
    property bool   showIndicator:                          activeVehicle && activeVehicle.firstControlStatusReceived
    property var    sysidInControl:                         activeVehicle ?  activeVehicle.sysidInControl : 0
    property var    secondaryGCSList:                       activeVehicle ? activeVehicle.secondaryGCSList : []
    property bool   gcsControlStatusFlags_SystemManager:    activeVehicle ? activeVehicle.gcsControlStatusFlags_SystemManager : false
    property bool   gcsControlStatusFlags_TakeoverAllowed:  activeVehicle ? activeVehicle.gcsControlStatusFlags_TakeoverAllowed : false
    property Fact   requestControlAllowTakeoverFact:        QGroundControl.settingsManager.flyViewSettings.requestControlAllowTakeover
    property bool   requestControlAllowTakeover:            requestControlAllowTakeoverFact.rawValue
    property bool   isThisGCSinControl:                     gcsMain == QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID.rawValue
    property bool   sendControlRequestAllowed:              activeVehicle ? activeVehicle.sendControlRequestAllowed : false
    // When nobody is in control (uncontrolled) or takeover is allowed, the autopilot grants control
    // immediately, so there is no owner to ask and no request countdown
    property bool   controlGrantedImmediately:              sysidInControl == 0 || gcsControlStatusFlags_TakeoverAllowed

    property Fact   secondaryGCSSettingFact:                QGroundControl.settingsManager.flyViewSettings.operatorControlSecondaryGCS
    property string secondaryGCSSetting:                    secondaryGCSSettingFact.rawValue
    property bool   hasConfiguredSecondaryGCS:              secondaryGCSSetting.length > 0

    property var    margins:                                ScreenTools.defaultFontPixelWidth
    property var    panelRadius:                            ScreenTools.defaultFontPixelWidth * 0.5
    property var    buttonHeight:                           height * 1.6
    property var    squareButtonPadding:                    ScreenTools.defaultFontPixelWidth
    property var    separatorHeight:                        buttonHeight * 0.9
    property var    settingsPanelVisible:                   false
    property bool   outdoorPalette:                         qgcPal.globalTheme === QGCPalette.Light

    // Used by control request popup, when other GCS ask us for control
    property var    receivedRequestTimeoutMs:               QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.defaultValue // Use this as default in case something goes wrong. Usually it will be overriden on onRequestOperatorControlReceived
    property var    requestSysIdRequestingControl:          0
    property var    requestAllowTakeover:                   false

    signal triggerAnimations // Used to trigger animation inside the popup component

    Connections {
        target: activeVehicle
        // Popup prompting user to accept control from other GCS
        function onRequestOperatorControlReceived(sysIdRequestingControl, allowTakeover, requestTimeoutSecs) {
            // If we don't have the indicator visible ( not receiving CONTROL_STATUS ) don't proceed
            if (!control.showIndicator) {
                return
            }
            requestSysIdRequestingControl = sysIdRequestingControl
            requestAllowTakeover = allowTakeover
            // If request came without request timeout, use our default one
            receivedRequestTimeoutMs = requestTimeoutSecs !== 0 ? requestTimeoutSecs * 1000 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.defaultValue
            // First hide current popup, in case the normal control panel is visible
            mainWindow.closeIndicatorDrawer()
            // When showing the popup, the component will automatically start the count down in controlRequestPopup
            mainWindow.showIndicatorDrawer(controlRequestPopup, control)
        }
        // Animation to blink indicator when any related info changes
        function onGcsControlStatusChanged() {
            backgroundRectangle.doOpacityAnimation()
            triggerAnimations() // Needed for animation inside the popup component
        }
    }

    // Background to have animation when current system in control changes
    Rectangle {
        id:                 backgroundRectangle
        anchors.centerIn:   parent
        width:              height
        height:             parent.height * 1.4
        color:              isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
        radius:             margins
        opacity:            0.0

        function doOpacityAnimation() { opacityAnimation.restart() }
        SequentialAnimation on opacity {
            id:         opacityAnimation
            running:    false
            loops:      1
            PropertyAnimation { to: 0.4; duration: 150 }
            PropertyAnimation { to: 0.5; duration: 100 }
            PropertyAnimation { to: 0.0; duration: 150 }
        }
    }

    // Control request popup. Appears when other GCS requests control, so operator on this one can accept or deny it
    Component {
        id: controlRequestPopup
        ToolIndicatorPage {

            TimedProgressTracker {
                id:                     requestProgressTracker
                timeoutSeconds:         receivedRequestTimeoutMs * 0.001
                onTimeout:              mainWindow.closeIndicatorDrawer()
            }

            Component.onCompleted: {
                requestProgressTracker.start()
            }

            contentComponent: GridLayout {
                id:                 mainLayout
                columns:            3

                // Action label
                QGCLabel {
                    font.pointSize:     ScreenTools.defaultFontPointSize * 1.1
                    text:               qsTr("GCS ") + requestSysIdRequestingControl + qsTr(" is requesting control")
                    font.bold:          true
                    Layout.columnSpan:  2
                }
                QGCButton {
                    text:                   qsTr("Allow <br> takeover")
                    Layout.rowSpan:         2
                    Layout.leftMargin:      margins * 2
                    Layout.alignment:       Qt.AlignBottom
                    Layout.fillHeight:      true
                    onClicked: {
                        control.activeVehicle.requestOperatorControl(true) // Allow takeover
                        mainWindow.closeIndicatorDrawer()
                        // After allowing takeover, if other GCS does not take control within 10 seconds
                        // takeover will be set to not allowed again. Notify user about this
                        control.activeVehicle.startTimerRevertAllowTakeover()
                        mainWindow.showIndicatorDrawer(allowTakeoverExpirationPopup, control)
                    }
                }
                // Action label
                QGCLabel {
                    font.pointSize:         ScreenTools.defaultFontPointSize * 1.1
                    text:                   qsTr("Ignoring automatically in ") + requestProgressTracker.progressLabel + qsTr(" seconds")
                }
                QGCButton {
                    id:                     ignoreButton
                    text:                   qsTr("Ignore")
                    onClicked:              mainWindow.closeIndicatorDrawer()
                    Layout.alignment:       Qt.AlignHCenter
                }
                // Actual progress bar
                Rectangle {
                    id:                     overlayRectangle
                    height:                 ScreenTools.defaultFontPixelWidth
                    width:                  parent.width * requestProgressTracker.progress
                    color:                  qgcPal.buttonHighlight
                    Layout.columnSpan:      2
                }
            }
        }
    }

    // Allow takeover expiration time popup. When a request is received and takeover was allowed, this popup alerts
    // that after vehicle::REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS seconds, this GCS will change back to takeover not allowed, as per mavlink specs
    Component {
        id: allowTakeoverExpirationPopup

            ToolIndicatorPage {
            // Allow takeover expiration time popup. When a request is received and takeover was allowed, this popup alerts
            // that after vehicle::REQUEST_OPERATOR_CONTROL_ALLOW_TAKEOVER_TIMEOUT_MSECS seconds, this GCS will change back to takeover not allowed, as per mavlink specs
            TimedProgressTracker {
                id:                     revertTakeoverProgressTracker
                timeoutSeconds:         control.activeVehicle.operatorControlTakeoverTimeoutMsecs * 0.001
                onTimeout:              {
                    mainWindow.closeIndicatorDrawer()
                }
            }
            // After accepting allow takeover after a request, we show the 10 seconds countdown after which takeover will be set again to not allowed.
            // If during this time another GCS takes control, which is what we are expecting, remove this popup
            property var isThisGCSinControlLocal: control.isThisGCSinControl
            onIsThisGCSinControlLocalChanged: {
                if (visible && !isThisGCSinControlLocal) {
                    mainWindow.closeIndicatorDrawer()
                }
            }

            Component.onCompleted: {
                revertTakeoverProgressTracker.start()
            }

            contentComponent: GridLayout {
                id:                 mainLayout
                columns:            3

                // Action label
                QGCLabel {
                    font.pointSize:         ScreenTools.defaultFontPointSize * 1.1
                    text:                   qsTr("Reverting back to takeover not allowed if GCS ") + requestSysIdRequestingControl +
                                            qsTr(" doesn't take control in ") + revertTakeoverProgressTracker.progressLabel +
                                            qsTr(" seconds ...")
                }
                QGCButton {
                    id:                     ignoreButton
                    text:                   qsTr("Ignore")
                    onClicked:              mainWindow.closeIndicatorDrawer()
                    Layout.alignment:       Qt.AlignHCenter
                }
            }
        }
    }

    // Popup panel, appears when clicking top control indicator
    Component {
        id: controlPopup

        ToolIndicatorPage {

            TimedProgressTracker {
                id:                     sendRequestProgressTracker
                timeoutSeconds:         QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
            }
            // If a request was sent, and we get feedback that takeover has been allowed, stop the progress tracker as the request has been granted
            property var takeoverAllowedLocal: control.gcsControlStatusFlags_TakeoverAllowed
            onTakeoverAllowedLocalChanged: {
                if (takeoverAllowedLocal && sendRequestProgressTracker.running) {
                    sendRequestProgressTracker.stop()
                }
            }
            // Also stop it if we gained control, or the vehicle became uncontrolled/the request expired,
            // which the C++ side reports through sendControlRequestAllowed
            property bool isThisGCSinControlLocal: control.isThisGCSinControl
            onIsThisGCSinControlLocalChanged: {
                if (isThisGCSinControlLocal && sendRequestProgressTracker.running) {
                    sendRequestProgressTracker.stop()
                }
            }
            property bool sendControlRequestAllowedLocal: control.sendControlRequestAllowed
            onSendControlRequestAllowedLocalChanged: {
                if (sendControlRequestAllowedLocal && sendRequestProgressTracker.running) {
                    sendRequestProgressTracker.stop()
                }
            }

            Component.onCompleted: {
                // If send control request is not allowed it means we recently sent a request, closed the popup, and opened again
                // before the other request timeout expired. This way we can keep track of the time remaining and update UI accordingly
                if (!sendControlRequestAllowed) {
                    // vehicle.requestOperatorControlRemainingMsecs holds the time remaining for the current request
                    startProgressTracker(control.activeVehicle.requestOperatorControlRemainingMsecs * 0.001)
                }
            }

            function startProgressTracker(timeoutSeconds) {
                sendRequestProgressTracker.timeoutSeconds = timeoutSeconds
                sendRequestProgressTracker.start()
            }

            contentComponent: GridLayout {
                id:                 mainLayout
                columns:            2

                QGCLabel {
                    text:                   qsTr("System in control: ")
                    font.bold:              true
                }
                QGCLabel {
                    text:                   isThisGCSinControl ? (qsTr("This GCS") + " (" + gcsMain + ")" ) : gcsMain
                    font.bold:              isThisGCSinControl
                    color:                  isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
                    Layout.alignment:       Qt.AlignRight
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                }
                QGCLabel {
                    text:                   gcsControlStatusFlags_TakeoverAllowed ? qsTr("Takeover allowed") : qsTr("Takeover NOT allowed")
                    Layout.columnSpan:      2
                    Layout.alignment:       Qt.AlignRight
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                    color:                  gcsControlStatusFlags_TakeoverAllowed ? qgcPal.colorGreen : qgcPal.text
                }
                QGCLabel {
                    text:                   qsTr("Secondary GCS: ") + secondaryGCSList.join(", ")
                    Layout.columnSpan:      2
                    visible:                secondaryGCSList.length > 0
                }
                // Separator
                Rectangle {
                    Layout.columnSpan:      2
                    Layout.preferredWidth:  parent.width
                    Layout.alignment:       Qt.AlignHCenter
                    color:                  qgcPal.windowShade
                    height:                 outdoorPalette ? 1 : 2
                }
                QGCLabel {
                    text:                   qsTr("Send Control Request:")
                    Layout.columnSpan:      2
                    visible:                !isThisGCSinControl
                }
                QGCLabel {
                    text:                   qsTr("Change takeover condition:")
                    Layout.columnSpan:      2
                    visible:                isThisGCSinControl
                }
                QGCLabel {
                    id:                     requestSentTimeoutLabel
                    text:                   qsTr("Request sent: ") + sendRequestProgressTracker.progressLabel
                    Layout.columnSpan:      2
                    visible:                sendRequestProgressTracker.running
                }
                FactCheckBox {
                    text:                   qsTr("Allow takeover")
                    fact:                   requestControlAllowTakeoverFact
                    enabled:                gcsControlStatusFlags_TakeoverAllowed || isThisGCSinControl
                }
                QGCButton {
                    text:                   controlGrantedImmediately ? qsTr("Acquire Control") : qsTr("Send Request")
                    onClicked: {
                        var timeout = controlGrantedImmediately ? 0 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
                        control.activeVehicle.requestOperatorControl(requestControlAllowTakeoverFact.rawValue, timeout)
                        if (timeout > 0) {
                            startProgressTracker(timeout)
                        }
                    }
                    Layout.alignment:       Qt.AlignRight
                    visible:                !isThisGCSinControl
                    enabled:                !sendRequestProgressTracker.running
                }
                QGCLabel {
                    text:                   qsTr("Request Timeout (sec):")
                    visible:                !isThisGCSinControl && !controlGrantedImmediately
                }
                FactTextField {
                    fact:                   QGroundControl.settingsManager.flyViewSettings.requestControlTimeout
                    visible:                !isThisGCSinControl && !controlGrantedImmediately
                    Layout.alignment:       Qt.AlignRight
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 7
                }
                QGCButton {
                    text:                   qsTr("Change")
                    onClicked:              control.activeVehicle.requestOperatorControl(requestControlAllowTakeoverFact.rawValue)
                    visible:                isThisGCSinControl
                    Layout.alignment:       Qt.AlignRight
                    enabled:                gcsControlStatusFlags_TakeoverAllowed != requestControlAllowTakeoverFact.rawValue
                }
                QGCButton {
                    text:                   qsTr("Release Control")
                    onClicked: {
                        control.activeVehicle.releaseOperatorControl()
                    }
                    Layout.columnSpan:      2
                    Layout.alignment:       Qt.AlignRight
                    visible:                isThisGCSinControl
                }
                // Separator
                Rectangle {
                    Layout.columnSpan:      2
                    Layout.preferredWidth:  parent.width
                    Layout.alignment:       Qt.AlignHCenter
                    color:                  qgcPal.windowShade
                    height:                 outdoorPalette ? 1 : 2
                }
                LabelledFactTextField {
                    Layout.fillWidth:       true
                    Layout.columnSpan:      2
                    label:                  qsTr("This GCS Mavlink System ID: ")
                    fact:                   QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID
                }
                // Separator
                Rectangle {
                    Layout.columnSpan:      2
                    Layout.preferredWidth:  parent.width
                    Layout.alignment:       Qt.AlignHCenter
                    color:                  qgcPal.windowShade
                    height:                 outdoorPalette ? 1 : 2
                }
                // Compact hint when secondary GCS are configured (collapsed)
                QGCLabel {
                    property bool expanded: false
                    id:                     rangeSettingsToggle
                    text:                   hasConfiguredSecondaryGCS
                                                ? qsTr("Control range configured (click to edit)")
                                                : qsTr("Configure secondary GCS range")
                    Layout.columnSpan:      2
                    color:                  hasConfiguredSecondaryGCS ? qgcPal.buttonHighlight : qgcPal.text
                    font.underline:         true
                    MouseArea {
                        anchors.fill:       parent
                        cursorShape:        Qt.PointingHandCursor
                        onClicked:          rangeSettingsToggle.expanded = !rangeSettingsToggle.expanded
                    }
                }
                // Expanded settings
                QGCLabel {
                    text:                   qsTr("Secondary GCS IDs (comma or space separated):")
                    Layout.columnSpan:      2
                    visible:                rangeSettingsToggle.expanded
                }
                FactTextField {
                    fact:                   secondaryGCSSettingFact
                    Layout.columnSpan:      2
                    Layout.fillWidth:       true
                    visible:                rangeSettingsToggle.expanded
                }
                QGCLabel {
                    id:                     rangeSummaryLabel
                    visible:                rangeSettingsToggle.expanded && hasConfiguredSecondaryGCS
                    Layout.columnSpan:      2
                    color:                  qgcPal.buttonHighlight
                    // Number of sysids inside the computed range which are neither this GCS nor a configured secondary.
                    // The protocol encodes the request as a contiguous range, so these would be granted control too
                    property int unconfiguredIdsInRange: 0
                    text: {
                        var myId = QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID.rawValue
                        // Accept any non-digit separator (commas, spaces, or a mix), matching _computeOperatorControlRange
                        var parts = secondaryGCSSetting.match(/\d+/g) || []
                        var ids = [ myId ]
                        var lo = myId
                        var hi = myId
                        for (var i = 0; i < parts.length; i++) {
                            var val = parseInt(parts[i])
                            if (!isNaN(val) && val >= 1 && val <= 255) {
                                if (val < lo) lo = val
                                if (val > hi) hi = val
                                if (ids.indexOf(val) < 0) ids.push(val)
                            }
                        }
                        rangeSummaryLabel.unconfiguredIdsInRange = (hi - lo + 1) - ids.length
                        return qsTr("Request range: ") + lo + " - " + hi
                    }
                }
                QGCLabel {
                    visible:                rangeSummaryLabel.visible && rangeSummaryLabel.unconfiguredIdsInRange > 0
                    Layout.columnSpan:      2
                    // preferredWidth 0 keeps the long warning text from inflating the
                    // panel's natural width; fillWidth then stretches it to whatever
                    // width the other panel elements already establish, wrapping inside it
                    Layout.fillWidth:       true
                    Layout.preferredWidth:  0
                    wrapMode:               Text.WordWrap
                    color:                  qgcPal.colorOrange
                    text:                   qsTr("Warning: %1 other GCS id(s) inside this range will also be accepted as operators").arg(rangeSummaryLabel.unconfiguredIdsInRange)
                }
            }
        }
    }

    // Actual top toolbar indicator
    QGCColoredImage {
        id:                      controlIndicatorIconLine
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gcscontrolIndicator/gcscontrol_line.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconAircraft
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gcscontrolIndicator/gcscontrol_device.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   (isThisGCSinControl || gcsControlStatusFlags_TakeoverAllowed) ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconGCS
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gcscontrolIndicator/gcscontrol_gcs.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   qgcPal.text

        // Current GCS in control indicator
        QGCLabel {
            id:                     gcsInControlIndicator
            text:                   gcsMain
            font.bold:              true
            font.pointSize:         ScreenTools.smallFontPointSize * 1.1
            color:                  isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
            anchors.bottom:         parent.bottom
            anchors.bottomMargin:   -margins * 0.7
            anchors.right:          parent.right
            anchors.rightMargin:    -margins * 0.1
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            mainWindow.showIndicatorDrawer(controlPopup, control)
        }
    }
}
