import QtQuick

import QGroundControl
import QGroundControl.Controls
import QGroundControl.SynclairVisionUI

ListView {
    id: notificationStack

    implicitWidth:  SVUnits.objectWidth * 6
    implicitHeight: contentHeight
    height:         contentHeight

    model:                   SVNotificationManager.model
    spacing:                 SVUnits.margin
    verticalLayoutDirection: ListView.TopToBottom
    interactive:             false
    clip:                    false

    readonly property var _severityColors: ({
        "success": qgcPalette.colorGreen,
        "info":    qgcPalette.colorGrey,
        "warning": qgcPalette.colorOrange,
        "error":   qgcPalette.colorRed
    })

    readonly property var _typeIcons: ({
        "network_connecting":   "qrc:/qmlimages/network_connected.svg",
        "network_connected":    "qrc:/qmlimages/network_connected.svg",
        "network_disconnected": "qrc:/qmlimages/network_disconnected.svg",
        "network_error":        "qrc:/qmlimages/network_disconnected.svg",

        "tracking_started":     "qrc:/qmlimages/crosshair.svg",
        "tracking_locked":      "qrc:/qmlimages/crosshair.svg",
        "tracking_lost":        "qrc:/qmlimages/crosshair.svg",

        "recording_started":    "qrc:/qmlimages/record.svg",
        "recording_stopped":    "qrc:/qmlimages/camera_record.svg",
        "recording_saved":      "qrc:/qmlimages/check.svg",
        "recording_failed":     "qrc:/qmlimages/record.svg"
    })

    function colorForSeverity(severity) {
        return _severityColors[severity] || _severityColors["info"]
    }

    function iconForType(type) {
        return _typeIcons[type] || "qrc:/qmlimages/info.svg"
    }

    // Only keep addDisplaced on the view level so lower cards shift down smoothly
    addDisplaced: Transition {
        NumberAnimation { properties: "y"; duration: 220; easing.type: Easing.OutCubic }
    }

    delegate: Item {
        id: delegateWrapper

        width: notificationStack.width

        readonly property real fullHeight: Math.max(SVUnits.objectWidth * 0.8, contentColumn.implicitHeight + (SVUnits.bigMargin * 2))

        // Height collapsing logic on dismissal
        height: toastDismissing ? 0 : fullHeight
        clip:   true

        Behavior on height {
            NumberAnimation { duration: 220; easing.type: Easing.OutCubic }
        }

        // Delete from ListModel ONLY after the height animation completes
        onHeightChanged: {
            if (toastDismissing && height === 0) {
                SVNotificationManager._remove(toastId)
            }
        }

        // Reset state cleanly if Qt reuses this delegate instance
        ListView.onReused: {
            background.state = "ENTERING"
            background.state = "VISIBLE"
        }

        SVBackground {
            id: background

            anchors.left:  parent.left
            anchors.right: parent.right
            anchors.top:   parent.top
            height:        delegateWrapper.fullHeight

            borderWidth: 0
            radius:      SVUnits.radius

            // Default state
            state: "ENTERING"

            Component.onCompleted: {
                state = "VISIBLE"
            }

            // Sync dismissal state with model property
            Binding {
                target: background
                property: "state"
                value: "DISMISSING"
                when: toastDismissing
            }

            states: [
                State {
                    name: "ENTERING"
                    PropertyChanges { target: background; opacity: 0.0; x: 40 }
                },
                State {
                    name: "VISIBLE"
                    PropertyChanges { target: background; opacity: 1.0; x: 0 }
                },
                State {
                    name: "DISMISSING"
                    PropertyChanges { target: background; opacity: 0.0; x: -40 }
                }
            ]

            transitions: [
                Transition {
                    from: "ENTERING"; to: "VISIBLE"
                    NumberAnimation { properties: "opacity"; duration: 220; easing.type: Easing.OutCubic }
                    NumberAnimation { properties: "x"; duration: 220; easing.type: Easing.OutCubic }
                },
                Transition {
                    from: "VISIBLE"; to: "DISMISSING"
                    NumberAnimation { properties: "opacity"; duration: 180 }
                    NumberAnimation { properties: "x"; duration: 220; easing.type: Easing.InCubic }
                }
            ]

            // Left severity bar
            Rectangle {
                id: severityBar
                anchors.left:   parent.left
                anchors.top:    parent.top
                anchors.bottom: parent.bottom
                width:          4
                radius:         SVUnits.radius
                color:          "#333333"
            }

            // Notification Icon
            QGCColoredImage {
                id: toastIcon
                width:  SVUnits.width * 2
                height: SVUnits.width * 2

                anchors.left:           severityBar.right
                anchors.leftMargin:     SVUnits.bigMargin * 2
                anchors.verticalCenter: parent.verticalCenter

                source: iconForType(toastType)
                color:  colorForSeverity(toastSeverity)
            }

            // Text content
            Column {
                id: contentColumn

                anchors.left:           toastIcon.right
                anchors.leftMargin:     SVUnits.bigMargin * 2
                anchors.right:          parent.right
                anchors.rightMargin:    SVUnits.bigMargin * 2
                anchors.verticalCenter: parent.verticalCenter
                spacing:                2

                QGCLabel {
                    width:          parent.width
                    text:           toastTitle
                    font.bold:      true
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          qgcPalette.text
                    elide:          Text.ElideRight
                }

                QGCLabel {
                    width:          parent.width
                    text:           toastMessage
                    wrapMode:       Text.WordWrap
                    font.pointSize: ScreenTools.smallFontPointSize
                    color:          qgcPalette.text
                }
            }
        }
    }
}