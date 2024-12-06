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
    property var    receivedRequestTimeoutMs:               QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.defaultValue // Use this as default in case something goes wrong
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

            property var progress:      0
            SequentialAnimation on progress { 
                id:         progressAnimation
                running:    false
                loops:      1
                NumberAnimation { target: controlRequestRectangle; property: "progress"; to: 1; duration: receivedRequestTimeoutMs }
            }

            property var progresTimeLabel
            property double lastUpdateTime: 0
            onProgressChanged: {
                // Only update each 0.2 seconds
                const currentTime = Date.now() * 0.001;
                if (currentTime - lastUpdateTime < 0.1) {
                    return
                }
                var currentCount = (progress * receivedRequestTimeoutMs * 0.001)
                progresTimeLabel = (receivedRequestTimeoutMs * 0.001 - currentCount).toFixed(1)
                lastUpdateTime = currentTime;
            }

            Component.onCompleted: {
                requestTimer.restart()
                progressAnimation.restart()
            }

            Timer {
                id:                     requestTimer
                interval:               receivedRequestTimeoutMs
                repeat:                 false
                running:                false
                onTriggered: {
                    // Sanity check, only hide if this panel is visible
                    if (!controlRequestRectangle.visible) {
                        return
                    }
                    mainWindow.hideIndicatorPopup()
                }
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
                        requestControlAllowTakeoverFact.rawValue = true
                        mainWindow.hideIndicatorPopup()
                    }
                }
                // Action label
                QGCLabel {
                    font.pointSize:         ScreenTools.defaultFontPointSize * 1.1
                    text:                   qsTr("Ignoring automatically in ") + progresTimeLabel + qsTr(" seconds")
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
                    width:                  parent.width * controlRequestRectangle.progress
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
                FactCheckBox {
                    text:                   qsTr("Allow takeover")
                    fact:                   requestControlAllowTakeoverFact
                    enabled:                gcsControlStatusFlags_TakeoverAllowed || isThisGCSinControl
                }
                QGCButton {
                    text:                   gcsControlStatusFlags_TakeoverAllowed ? qsTr("Adquire Control") : qsTr("Send Request")
                    onClicked:              requestControl()
                    Layout.alignment:       Qt.AlignRight
                    visible:                !isThisGCSinControl
                    // If requesting control, we need to take care of sending the timeout, so the progress bar is on sync in requestor and GCS in control. Only needed if takeover isn't allowed
                    function requestControl() {
                        var timeout = gcsControlStatusFlags_TakeoverAllowed ? 0 : QGroundControl.settingsManager.flyViewSettings.requestControlTimeout.rawValue
                        activeVehicle.requestOperatorControl(requestControlAllowTakeoverFact.rawValue, timeout)
                    }
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
