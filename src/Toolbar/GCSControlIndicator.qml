import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

Item {
    id:             control
    width:          controlIndicatorIconRole.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    activeVehicle:                          QGroundControl.multiVehicleManager.activeVehicle
    property var    gcsControlManager:                      activeVehicle ? activeVehicle.gcsControlManager : null
    property bool   showIndicator:                          gcsControlManager && gcsControlManager.firstControlStatusReceived
    property var    sysidInControl:                         gcsControlManager ? gcsControlManager.sysidInControl : 0
    property var    secondaryGCSList:                       gcsControlManager ? gcsControlManager.secondaryGCSList : []
    property bool   gcsControlStatusFlags_SystemManager:    gcsControlManager ? gcsControlManager.gcsControlStatusFlags_SystemManager : false
    property bool   gcsControlStatusFlags_TakeoverAllowed:  gcsControlManager ? gcsControlManager.gcsControlStatusFlags_TakeoverAllowed : false
    property Fact   requestControlAllowTakeoverFact:        QGroundControl.settingsManager.flyViewSettings.requestControlAllowTakeover
    property bool   requestControlAllowTakeover:            requestControlAllowTakeoverFact.rawValue
    property bool   isThisGCSinControl:                     sysidInControl == QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID.rawValue
    property bool   sendControlRequestAllowed:              gcsControlManager ? gcsControlManager.sendControlRequestAllowed : false
    // When nobody is in control (uncontrolled) or takeover is allowed, the autopilot grants control
    // immediately, so there is no owner to ask and no request countdown
    property bool   controlGrantedImmediately:              sysidInControl == 0 || gcsControlStatusFlags_TakeoverAllowed
    // Someone (anyone) holds control of the vehicle
    property bool   someoneInControl:                       sysidInControl != 0
    // This GCS is a recognized secondary operator: the vehicle lists us in its secondary range.
    // This holds even when uncontrolled (gcs_main == 0): a GCS within the recognized range is an
    // owner that can command the vehicle, just not the one holding manual control.
    property bool   isThisGCSsecondary:                     !isThisGCSinControl &&
                                                            secondaryGCSList.indexOf(Number(QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID.rawValue)) >= 0
    // This GCS has an operator role (primary or secondary) on the vehicle
    property bool   isThisGCSoperator:                      isThisGCSinControl || isThisGCSsecondary

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
        target: gcsControlManager
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
                        control.gcsControlManager.requestOperatorControl(true) // Allow takeover
                        mainWindow.closeIndicatorDrawer()
                        // After allowing takeover, if other GCS does not take control within 10 seconds
                        // takeover will be set to not allowed again. Notify user about this
                        control.gcsControlManager.startTimerRevertAllowTakeover()
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
                timeoutSeconds:         control.gcsControlManager.operatorControlTakeoverTimeoutMsecs * 0.001
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
            showExpand: true

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
                    startProgressTracker(control.gcsControlManager.requestOperatorControlRemainingMsecs * 0.001)
                }
            }

            function startProgressTracker(timeoutSeconds) {
                sendRequestProgressTracker.timeoutSeconds = timeoutSeconds
                sendRequestProgressTracker.start()
            }

            contentComponent: GridLayout {
                id:                 mainLayout
                columns:            2

                // --- Status (read-only) ---
                // 1. My situation: in control (main or secondary), else whether control is acquirable
                QGCLabel {
                    text:                   qsTr("Control status:")
                    font.bold:              true
                }
                QGCLabel {
                    text:                   isThisGCSinControl ? qsTr("In control, full")
                                                : (isThisGCSsecondary ? qsTr("In control, commands only")
                                                : (controlGrantedImmediately ? qsTr("Unlocked") : qsTr("Request needed")))
                    font.bold:              isThisGCSoperator
                    color:                  isThisGCSoperator ? qgcPal.colorGreen : qgcPal.text
                    Layout.alignment:       Qt.AlignRight
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                }
                // 2. Takeover permission
                QGCLabel {
                    text:                   controlGrantedImmediately ? qsTr("Takeover allowed") : qsTr("Takeover not allowed")
                    Layout.columnSpan:      2
                }
                // 3. Ownership roster: the main GCS and any secondaries, each labelled (this GCS marked)
                QGCLabel {
                    text:                   qsTr("Main GCS:")
                    font.bold:              true
                }
                QGCLabel {
                    text:                   isThisGCSinControl ? (sysidInControl + qsTr(" (This GCS)"))
                                                              : (someoneInControl ? ("" + sysidInControl) : qsTr("Nobody"))
                    font.bold:              isThisGCSinControl
                    color:                  isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
                    Layout.alignment:       Qt.AlignRight
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                }
                QGCLabel {
                    text:                   qsTr("Secondary GCS:")
                    font.bold:              true
                    visible:                secondaryGCSList.length > 0
                }
                QGCLabel {
                    visible:                secondaryGCSList.length > 0
                    color:                  qgcPal.text
                    textFormat:             Text.StyledText
                    Layout.alignment:       Qt.AlignRight
                    Layout.fillWidth:       true
                    horizontalAlignment:    Text.AlignRight
                    // List secondaries; colour only the entry that is this GCS green
                    text: {
                        var myId = Number(QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID.rawValue)
                        var out = []
                        for (var i = 0; i < secondaryGCSList.length; i++) {
                            var id = secondaryGCSList[i]
                            if (id === myId) {
                                out.push('<font color="' + qgcPal.colorGreen + '"><b>' + id + qsTr(" (This GCS)") + '</b></font>')
                            } else {
                                out.push("" + id)
                            }
                        }
                        return out.join(", ")
                    }
                }
                // Separator
                Rectangle {
                    Layout.columnSpan:      2
                    Layout.fillWidth:       true
                    color:                  qgcPal.windowShade
                    height:                 outdoorPalette ? 1 : 2
                }
                // --- Actions ---
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
                    // Requesting always targets the main (full) role, so a secondary that already has
                    // command authority sees that this upgrades it to full/manual control
                    text:                   controlGrantedImmediately ? qsTr("Take full control") : qsTr("Request full control")
                    onClicked: {
                        var timeout = controlGrantedImmediately ? 0 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
                        control.gcsControlManager.requestOperatorControl(requestControlAllowTakeoverFact.rawValue, timeout)
                        if (timeout > 0) {
                            startProgressTracker(timeout)
                        }
                    }
                    Layout.alignment:       Qt.AlignRight
                    visible:                !isThisGCSinControl
                    enabled:                !sendRequestProgressTracker.running
                }
                QGCButton {
                    text:                   qsTr("Change")
                    onClicked:              control.gcsControlManager.requestOperatorControl(requestControlAllowTakeoverFact.rawValue)
                    visible:                isThisGCSinControl
                    Layout.alignment:       Qt.AlignRight
                    enabled:                gcsControlStatusFlags_TakeoverAllowed != requestControlAllowTakeoverFact.rawValue
                    // padding to the right, otherwise the panel will get too narrow and the UI will look inconsistent when only this button is present.
                    Layout.leftMargin:      ScreenTools.defaultFontPixelWidth * 5
                }
                QGCButton {
                    text:                   qsTr("Release Control")
                    onClicked: {
                        control.gcsControlManager.releaseOperatorControl()
                    }
                    Layout.columnSpan:      2
                    Layout.alignment:       Qt.AlignRight
                    visible:                isThisGCSinControl
                }
            }

            // Editable settings, shown to the right of the status/actions panel via the standard
            // ToolIndicatorPage expand button, matching the rest of the toolbar indicators.
            // Wrapped in an Item with an explicit implicitWidth: the grid is loaded inside a Loader,
            // so Layout.* on it is ignored. Without a width source the wrapping labels below
            // (preferredWidth 0 + fillWidth) collapse and pile up; the fixed-width wrapper gives the
            // anchored grid a stable width to wrap within regardless of which rows are visible.
            expandedComponent: Item {
                id:             settingsRoot
                implicitWidth:  ScreenTools.defaultFontPixelWidth * 30
                implicitHeight: settingsLayout.implicitHeight

                GridLayout {
                    id:                 settingsLayout
                    anchors.left:       parent.left
                    anchors.right:      parent.right
                    anchors.top:        parent.top
                    columns:            2

                // This GCS system ID setting. Label on its own wrapping row so the long text doesn't
                    // drive the panel width; small field below.
                    QGCLabel {
                        text:                   qsTr("This GCS System ID:")
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  0
                        wrapMode:               Text.WordWrap
                    }
                    FactTextField {
                        fact:                   QGroundControl.settingsManager.mavlinkSettings.gcsMavlinkSystemID
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                        Layout.alignment:       Qt.AlignRight
                    }
                    // Request timeout setting (same wrapping-label treatment)
                    QGCLabel {
                        text:                   qsTr("Takeover request timeout (s):")
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  0
                        wrapMode:               Text.WordWrap
                    }
                    FactTextField {
                        fact:                   QGroundControl.settingsManager.flyViewSettings.requestControlTimeout
                        Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 8
                        Layout.alignment:       Qt.AlignRight
                    }
                    // Multi-owner toggle: reveals the secondary GCS field. The label is a separate wrapping,
                    // clickable QGCLabel (QGCCheckBox text can't wrap) so the long text doesn't drive panel
                    // width. Initialised from whether secondaries are configured; unticking clears the list.
                    RowLayout {
                        Layout.columnSpan:      2
                        Layout.fillWidth:       true
                        spacing:                ScreenTools.defaultFontPixelWidth
                        QGCCheckBox {
                            id:                 multiGcsCheckBox
                            checked:            hasConfiguredSecondaryGCS
                            onCheckedChanged:   if (!checked) secondaryGCSSettingFact.rawValue = ""
                        }
                        QGCLabel {
                            text:               qsTr("This GCS is part of a control group")
                            Layout.fillWidth:   true
                            Layout.preferredWidth: 0
                            wrapMode:           Text.WordWrap
                            MouseArea {
                                anchors.fill:   parent
                                onClicked:      multiGcsCheckBox.checked = !multiGcsCheckBox.checked
                            }
                        }
                    }
                    // Secondary GCS list setting. This label is unusually long, so let it wrap instead of
                    // inflating the panel width: preferredWidth 0 + fillWidth makes it take the width the
                    // rest of the panel already establishes and wrap within it.
                    QGCLabel {
                        text:                   qsTr("Secondary GCS IDs (comma or space separated):")
                        Layout.columnSpan:      2
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  0
                        wrapMode:               Text.WordWrap
                        visible:                multiGcsCheckBox.checked
                    }
                    FactTextField {
                        fact:                   secondaryGCSSettingFact
                        // Full-width own row with preferredWidth 0: keeps the typed list from widening a
                        // shared column (which would shove the right-justified fields on the rows above).
                        Layout.columnSpan:      2
                        Layout.fillWidth:       true
                        Layout.preferredWidth:  0
                        visible:                multiGcsCheckBox.checked
                    }
                    QGCLabel {
                        id:                     rangeSummaryLabel
                        // Full-width own row so it never lands in the field column above and inflate it
                        Layout.columnSpan:      2
                        visible:                multiGcsCheckBox.checked && hasConfiguredSecondaryGCS
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
    }

    // Actual top toolbar indicator. Three stacked layers occupying different quadrants:
    //   aircraft (top-left)  - green when the vehicle has a controller or this GCS is an operator
    //   line     (bottom-left) - green only when THIS GCS has an operator role (its control link)
    //   role glyph (right)   - solid device = primary, outlined device = secondary,
    //                          lock open/closed = no role (open when control is acquirable now).
    //                          Always white: it's your station; green is reserved for the control
    //                          relationship (aircraft + line + MAIN/SEC label).
    QGCColoredImage {
        id:                      controlIndicatorIconAircraft
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gcscontrolIndicator/multigcs_aircraft.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        // Green when the vehicle has a controller (a main) or when this GCS is itself an operator
        // (covers the gcs_main==0 case where this GCS is a recognized secondary)
        color:                   (someoneInControl || isThisGCSoperator) ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconLine
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/gcscontrolIndicator/multigcs_line.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   isThisGCSoperator ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconRole
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  isThisGCSoperator
                                     ? (isThisGCSinControl ? "/gcscontrolIndicator/multigcs_device.svg"
                                                           : "/gcscontrolIndicator/multigcs_device_alt.svg")
                                     : (controlGrantedImmediately ? "/gcscontrolIndicator/multigcs_lock_open.svg"
                                                                  : "/gcscontrolIndicator/multigcs_lock_closed.svg")
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   qgcPal.text

        // MAIN/SEC role label, only shown when this GCS has an operator role
        QGCLabel {
            id:                     roleLabel
            text:                   isThisGCSinControl ? qsTr("MAIN") : qsTr("SEC")
            visible:                isThisGCSoperator
            font.bold:              true
            font.pointSize:         ScreenTools.smallFontPointSize
            color:                  qgcPal.colorGreen
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
