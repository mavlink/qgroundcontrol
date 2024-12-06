import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Palette
import QGroundControl.FactSystem
import QGroundControl.FactControls

Item {
    id:             _root
    width:          controlIndicatorIconGCS.width * 1.1
    anchors.top:    parent.top
    anchors.bottom: parent.bottom

    property var    activeVehicle:                          QGroundControl.multiVehicleManager.activeVehicle
    property bool   showIndicator:                          activeVehicle && activeVehicle.firstControlStatusReceived
    property var    sysidInControl:                         activeVehicle ?  activeVehicle.sysidInControl : 0
    property bool   gcsControlStatusFlags_SystemManager:    activeVehicle ? activeVehicle.gcsControlStatusFlags_SystemManager : false
    property bool   gcsControlStatusFlags_TakeoverAllowed:  activeVehicle ? activeVehicle.gcsControlStatusFlags_TakeoverAllowed : false
    property Fact   requestControlAllowTakeoverFact:        QGroundControl.settingsManager.flyViewSettings.requestControlAllowTakeover
    property bool   requestControlAllowTakeover:            requestControlAllowTakeoverFact.rawValue
    property bool   isThisGCSinControl:                     sysidInControl == QGroundControl.mavlinkSystemID

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
        onRequestOperatorControlReceived: (sysIdRequestingControl, allowTakeover, requestTimeoutSecs) => {
            // If we don't have the indicator visible ( not receiving CONTROL_STATUS ) don't proceed
            if (!_root.showIndicator) {
                return
            }
            requestSysIdRequestingControl = sysIdRequestingControl
            requestAllowTakeover = allowTakeover
            // If request came without request timeout, use our default one
            receivedRequestTimeoutMs = requestTimeoutSecs !== 0 ? requestTimeoutSecs * 1000 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.defaultValue
            // First hide current popup, in case the normal control panel is visible
            mainWindow.hideIndicatorPopup()
            // When showing the popup, the component will automatically start the count down in controlRequestPopup
            mainWindow.showIndicatorPopup(_root, controlRequestPopup, false)
        }
        // Animation to blink indicator when any related info changes
        onGcsControlStatusChanged: {
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
        Item {
            id:                         controlRequestRectangle
            width:                      mainLayout.width + margins * 4
            height:                     mainLayout.height + margins * 4

            Rectangle {
                anchors.fill:           parent
                radius:                 ScreenTools.defaultFontPixelWidth / 2
                color:                  qgcPal.window
                opacity:                0.7
            }

            ProgressTracker {
                id:                     requestProgressTracker
                timeoutSeconds:         receivedRequestTimeoutMs * 0.001
                onTimeout:              mainWindow.hideIndicatorPopup()
            }

            Component.onCompleted: {
                requestProgressTracker.start()
            }

            GridLayout {
                id:                 mainLayout
                anchors.margins:    margins * 2
                anchors.top:        parent.top
                anchors.right:      parent.right
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
                        activeVehicle.requestOperatorControl(true) // Allow takeover
                        mainWindow.hideIndicatorPopup()
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
                    onClicked:              mainWindow.hideIndicatorPopup()
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

    // Popup panel, appears when clicking top control indicator
    Component {
        id: controlPopup

        Rectangle {
            id:             popupBackground
            width:          mainLayout.width   + mainLayout.anchors.margins * 2          
            height:         mainLayout.height  + mainLayout.anchors.margins * 2
            color:          qgcPal.window
            radius:         panelRadius

            Connections {
                target:              _root
                onTriggerAnimations: doColorAnimation()
            }
            function doColorAnimation() { colorAnimation.restart() }
            SequentialAnimation on color { 
                id:         colorAnimation
                running:    false
                loops:      1
                PropertyAnimation { to: qgcPal.windowShade; duration: 200 }
                PropertyAnimation { to: qgcPal.window;      duration: 200 }
            }

            ProgressTracker {
                id:                     sendRequestProgressTracker
                timeoutSeconds:         QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
            }
            // If a request was sent, and we get feedback that takeover has been allowed, stop the progress tracker as the request has been granted
            property var takeoverAllowedLocal: _root.gcsControlStatusFlags_TakeoverAllowed
            onTakeoverAllowedLocalChanged: {
                if (takeoverAllowedLocal && sendRequestProgressTracker.running) {
                    sendRequestProgressTracker.stop()
                }
            }

            GridLayout {
                id:                 mainLayout
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                anchors.top:        parent.top
                anchors.right:      parent.right
                columns:            2

                QGCLabel {
                    text:                   qsTr("System in control: ")
                    font.bold:              true
                }
                QGCLabel {
                    text:                   isThisGCSinControl ? (qsTr("This GCS") + " (" + sysidInControl + ")" ) : sysidInControl
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
                    text:                   gcsControlStatusFlags_TakeoverAllowed ? qsTr("Adquire Control") : qsTr("Send Request")
                    onClicked: {
                        var timeout = gcsControlStatusFlags_TakeoverAllowed ? 0 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
                        activeVehicle.requestOperatorControl(requestControlAllowTakeoverFact.rawValue, timeout)
                        if (timeout > 0) {
                            // Start UI timeout animation
                            sendRequestProgressTracker.start()
                        }
                    }
                    Layout.alignment:       Qt.AlignRight
                    visible:                !isThisGCSinControl
                    enabled:                !sendRequestProgressTracker.running
                }
                QGCLabel {
                    text:                   qsTr("Request Timeout (sec):")
                    visible:                !isThisGCSinControl && !gcsControlStatusFlags_TakeoverAllowed
                }
                FactTextField {
                    fact:                   QGroundControl.settingsManager.flyViewSettings.requestControlTimeout
                    visible:                !isThisGCSinControl && !gcsControlStatusFlags_TakeoverAllowed
                    Layout.alignment:       Qt.AlignRight
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 7
                }
                QGCButton {
                    text:                   qsTr("Change")
                    onClicked:              activeVehicle.requestOperatorControl(requestControlAllowTakeoverFact.rawValue)
                    visible:                isThisGCSinControl
                    Layout.alignment:       Qt.AlignRight
                    enabled:                gcsControlStatusFlags_TakeoverAllowed != requestControlAllowTakeoverFact.rawValue
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
                    text:                   qsTr("This GCS Mavlink System ID: ")
                }
                QGCTextField {
                    text:                   QGroundControl.mavlinkSystemID.toString()
                    numericValuesOnly:      true
                    Layout.alignment:       Qt.AlignRight
                    Layout.preferredWidth:  ScreenTools.defaultFontPixelWidth * 7
                    horizontalAlignment:    TextInput.AlignHCenter
                    onEditingFinished: {
                        if (parseInt(text) > 0 && parseInt(text) < 256) {
                            QGroundControl.mavlinkSystemID = parseInt(text)
                        } else {
                            mainWindow.showMessageDialog(qsTr("Invalid System ID"), qsTr("System ID must be in the range of 1 - 255"))
                        }
                    }
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
        source:                  "/controlIndicator/gcscontrol_line.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   isThisGCSinControl ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconAircraft
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/controlIndicator/gcscontrol_device.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   (isThisGCSinControl || gcsControlStatusFlags_TakeoverAllowed) ? qgcPal.colorGreen : qgcPal.text
    }
    QGCColoredImage {
        id:                      controlIndicatorIconGCS
        width:                   height
        anchors.top:             parent.top
        anchors.bottom:          parent.bottom
        source:                  "/controlIndicator/gcscontrol_gcs.svg"
        fillMode:                Image.PreserveAspectFit
        sourceSize.height:       height
        color:                   qgcPal.text

        // Current GCS in control indicator
        QGCLabel {
            id:                     gcsInControlIndicator
            text:                   sysidInControl
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
            mainWindow.showIndicatorPopup(_root, controlPopup, false)
        }
    }
}
