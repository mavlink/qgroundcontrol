/****************************************************************************
 *
 * USV Fly View Custom Layer
 *
 ****************************************************************************/
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

Item {
    id: _root

    property var parentToolInsets
    property var totalToolInsets:   _toolInsets
    property var mapControl

    property var  activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real _toolsMargin: ScreenTools.defaultFontPixelWidth * 0.75

    // 姿态 - 安全访问 rawValue
    property real roll:  (activeVehicle && activeVehicle.roll  && activeVehicle.roll.rawValue  !== undefined) ? activeVehicle.roll.rawValue  : 0
    property real pitch: (activeVehicle && activeVehicle.pitch && activeVehicle.pitch.rawValue !== undefined) ? activeVehicle.pitch.rawValue : 0
    property bool isAttitudeCritical: Math.abs(roll) > 25 || Math.abs(pitch) > 20

    // Fact 缓存
    property bool _hasCapsulePayload: activeVehicle && activeVehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var  _capsuleStatusFact: _hasCapsulePayload ? activeVehicle.getFact("usvPayload.status") : null

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       Math.max(parentToolInsets.leftEdgeTopInset, payloadPanel.visible ? payloadPanel.x + payloadPanel.width + _toolsMargin : 0)
        leftEdgeCenterInset:    Math.max(parentToolInsets.leftEdgeCenterInset, payloadPanel.visible ? payloadPanel.x + payloadPanel.width + _toolsMargin : 0)
        leftEdgeBottomInset:    parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset:      parentToolInsets.rightEdgeTopInset
        rightEdgeCenterInset:   parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset:   Math.max(parentToolInsets.rightEdgeBottomInset, instrumentPanel.height + _toolsMargin * 2)
        topEdgeLeftInset:       parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset:     parentToolInsets.topEdgeCenterInset
        topEdgeRightInset:      parentToolInsets.topEdgeRightInset
        bottomEdgeLeftInset:    parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset:  parentToolInsets.bottomEdgeCenterInset
        bottomEdgeRightInset:   Math.max(parentToolInsets.bottomEdgeRightInset, instrumentPanel.width + _toolsMargin * 2)
    }

    // ===== 姿态警告横幅 =====
    Rectangle {
        id: warningBanner
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: parentToolInsets.topEdgeCenterInset + ScreenTools.defaultFontPixelHeight
        width: warningLabel.width + ScreenTools.defaultFontPixelWidth * 4
        height: warningLabel.height + ScreenTools.defaultFontPixelHeight
        color: qgcPal.colorRed
        radius: ScreenTools.defaultFontPixelWidth * 0.5
        visible: isAttitudeCritical
        opacity: 0.95
        z: 1000

        SequentialAnimation on opacity {
            running: warningBanner.visible
            loops: Animation.Infinite
            NumberAnimation { to: 0.6; duration: 500 }
            NumberAnimation { to: 0.95; duration: 500 }
        }

        QGCLabel {
            id: warningLabel
            anchors.centerIn: parent
            text: qsTr("⚠ 姿态异常 - R:%1° P:%2°").arg(Number(roll).toFixed(1)).arg(Number(pitch).toFixed(1))
            color: "white"
            font.bold: true
        }
    }

    // ===== 右下角仪表盘 =====
    IntegratedCompassAttitude {
        id: instrumentPanel
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: _toolsMargin
        anchors.rightMargin: _toolsMargin
        vehicle: activeVehicle
    }

    // ===== 左上载荷面板 (通过 source 路径加载，绕过 QML 模块 import 上下文问题) =====
    Loader {
        id: payloadPanel
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.topMargin: parentToolInsets.topEdgeLeftInset + _toolsMargin
        anchors.leftMargin: parentToolInsets.leftEdgeTopInset + _toolsMargin
        width: ScreenTools.defaultFontPixelWidth * 26
        visible: status === Loader.Ready && item && item.height > 0
        z: 999
        active: activeVehicle ? true : false
        source: "qrc:/qml/USV/res/USVPayloadPanel.qml"

        onStatusChanged: {
            if (status === Loader.Ready && item) {
                item.vehicle = Qt.binding(function() { return _root.activeVehicle })
                console.log("USVPayloadPanel loaded OK, size:", item.width, "x", item.height)
            } else if (status === Loader.Error) {
                console.warn("USVPayloadPanel LOAD FAILED")
            }
        }
    }

    // ===== 底部状态胶囊 =====
    Rectangle {
        id: statusCapsule
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parentToolInsets.bottomEdgeCenterInset + ScreenTools.defaultFontPixelHeight
        width: capsuleLabel.width + ScreenTools.defaultFontPixelWidth * 3
        height: capsuleLabel.height + ScreenTools.defaultFontPixelHeight * 0.6
        radius: height / 2
        color: _capsuleColor
        opacity: 0.9
        visible: activeVehicle && _capsuleStatus !== 0
        z: 500

        property int _capsuleStatus: _root._capsuleStatusFact ? _root._capsuleStatusFact.value : 0

        property color _capsuleColor: {
            switch(_capsuleStatus) {
            case 1: return qgcPal.colorGreen
            case 2: return qgcPal.brandingBlue
            case 3: return qgcPal.colorRed
            case 4: return qgcPal.colorOrange
            default: return "transparent"
            }
        }

        property string _capsuleText: {
            switch(_capsuleStatus) {
            case 1: return "● 采样中"
            case 2: return "● 检测中"
            case 3: return "⚠ 故障"
            case 4: return "● 校准中"
            default: return ""
            }
        }

        QGCLabel {
            id: capsuleLabel
            anchors.centerIn: parent
            text: statusCapsule._capsuleText
            color: "white"
            font.bold: true
        }

        SequentialAnimation on opacity {
            running: statusCapsule._capsuleStatus === 1 || statusCapsule._capsuleStatus === 4
            loops: Animation.Infinite
            NumberAnimation { to: 0.6; duration: 800 }
            NumberAnimation { to: 0.9; duration: 800 }
        }
    }
}
