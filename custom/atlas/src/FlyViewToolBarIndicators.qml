import QtQuick

import QGroundControl
import QGroundControl.Controls

Item {
    id: control

    objectName: "flyViewToolBarIndicators"
    implicitWidth: mainLayout.implicitWidth
    implicitHeight: ScreenTools.toolbarHeight

    property real _dotSize: Math.max(12, ScreenTools.defaultFontPixelHeight * 0.58)

    component StatusPill: Rectangle {
        id: pill

        property string label: ""
        property string detail: ""
        property bool online: false
        property bool warning: false
        property bool danger: false
        property bool bypassed: false

        width: 122
        height: mainLayout.height
        radius: 5
        color: "#24081720"
        border.width: 1
        border.color: pill.bypassed ? "#355C73" : "#263A4A"

        Rectangle {
            id: statusDot
            anchors.left: parent.left
            anchors.leftMargin: 11
            anchors.top: parent.top
            anchors.topMargin: 12
            width: control._dotSize
            height: width
            radius: width / 2
            color: pill.danger ? "#FF3040" :
                   pill.warning ? "#FFB020" :
                   pill.online ? "#39D353" :
                   pill.bypassed ? "#46A8E5" : "#7C8A94"
        }

        Column {
            anchors.left: statusDot.right
            anchors.leftMargin: 9
            anchors.right: parent.right
            anchors.rightMargin: 6
            anchors.verticalCenter: parent.verticalCenter
            spacing: 2

            QGCLabel {
                width: parent.width
                text: pill.label
                color: "#F4F8FA"
                font.bold: true
                font.pointSize: ScreenTools.smallFontPointSize * 1.20
                horizontalAlignment: Text.AlignLeft
            }

            QGCLabel {
                width: parent.width
                text: pill.detail
                color: pill.bypassed ? "#A9D8F3" : "#C7D8E2"
                font.pointSize: ScreenTools.smallFontPointSize * 0.98
                horizontalAlignment: Text.AlignLeft
                elide: Text.ElideRight
            }
        }
    }

    Row {
        id: mainLayout
        anchors.verticalCenter: parent.verticalCenter
        height: Math.max(52, parent.height - 10)
        spacing: 4

        StatusPill {
            label: "Jetson"
            detail: atlasPayload.directMode
                    ? "BYPASSED"
                    : (atlasPayload.connected ? "GPU " + atlasPayload.jetsonGpuPercent.toFixed(0) + "%" : "OFFLINE")
            online: atlasPayload.jetsonMode && atlasPayload.connected
            bypassed: atlasPayload.directMode
            warning: atlasPayload.jetsonMode && atlasPayload.connected && atlasPayload.jetsonTempC >= 75.0
            danger: atlasPayload.jetsonMode && atlasPayload.connected &&
                    (atlasPayload.jetsonTempC >= 80.0 || atlasPayload.jetsonThrottled)
        }

        StatusPill {
            label: "HM30"
            detail: atlasPayload.directMode
                    ? (atlasPayload.a8Connected ? "IP PATH OK" : "NO A8 LINK")
                    : (atlasPayload.hm30Connected ? atlasPayload.hm30RssiDbm.toFixed(0) + " dBm" : "OFFLINE")
            online: atlasPayload.directMode ? atlasPayload.a8Connected : atlasPayload.hm30Connected
            warning: atlasPayload.jetsonMode && atlasPayload.hm30Connected && atlasPayload.hm30PacketLossPercent >= 3.0
            danger: atlasPayload.jetsonMode && atlasPayload.hm30Connected && atlasPayload.hm30PacketLossPercent >= 8.0
        }

        StatusPill {
            label: "A8"
            detail: atlasPayload.a8Connected
                    ? atlasPayload.zoomLevel.toFixed(1) + "x" + (atlasPayload.directMode ? " DIRECT" : "")
                    : "OFFLINE"
            online: atlasPayload.a8Connected
        }

        StatusPill {
            label: "Video"
            detail: atlasVideoManager.decoding
                    ? (atlasPayload.directMode ? "LIVE" : atlasPayload.videoFps.toFixed(0) + " FPS")
                    : "WAITING"
            online: atlasVideoManager.decoding
            warning: atlasPayload.connected && !atlasVideoManager.decoding
        }

        StatusPill {
            label: "AI"
            detail: atlasPayload.directMode
                    ? "N/A"
                    : (atlasPayload.connected && atlasPayload.aiEnabled ? atlasPayload.aiFps.toFixed(0) + " FPS" : "OFF")
            online: atlasPayload.jetsonMode && atlasPayload.connected && atlasPayload.aiEnabled
            bypassed: atlasPayload.directMode
            warning: atlasPayload.jetsonMode && atlasPayload.connected && atlasPayload.aiEnabled &&
                     atlasPayload.aiFpsTarget > 0 &&
                     atlasPayload.aiFps < atlasPayload.aiFpsTarget * 0.55
        }
    }
}
