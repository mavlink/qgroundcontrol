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

import USV

Item {
    id: _root

    property var parentToolInsets
    property var totalToolInsets: _toolInsets
    property var mapControl

    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property real _toolsMargin: ScreenTools.defaultFontPixelWidth * 0.75

    property real roll: (activeVehicle && activeVehicle.roll && activeVehicle.roll.rawValue !== undefined) ? activeVehicle.roll.rawValue : 0
    property real pitch: (activeVehicle && activeVehicle.pitch && activeVehicle.pitch.rawValue !== undefined) ? activeVehicle.pitch.rawValue : 0
    property bool isAttitudeCritical: Math.abs(roll) > 25 || Math.abs(pitch) > 20

    property bool _hasCapsulePayload: activeVehicle && activeVehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _capsuleStatusFact: _hasCapsulePayload ? activeVehicle.getFact("usvPayload.status") : null
    property var _capsuleLinkFact: _hasCapsulePayload ? activeVehicle.getFact("usvPayload.linkActive") : null
    property int _capsuleStatus: _capsuleStatusFact ? _capsuleStatusFact.value : 0
    property bool _linkOk: _capsuleLinkFact ? _capsuleLinkFact.value === 1 : true

    readonly property int _cmdStop: 31011
    readonly property int _cmdPause: 31012
    readonly property int _cmdResume: 31013
    readonly property int _cmdCalibrate: 31014
    readonly property int _payloadCompId: 191

    property var _taskActions: {
        const actions = []
        if (!activeVehicle) {
            return actions
        }

        switch (_capsuleStatus) {
        case 1:
            actions.push({ text: qsTr("暂停"), cmd: _cmdPause, warn: false })
            actions.push({ text: qsTr("停止"), cmd: _cmdStop, warn: true })
            break
        case 2:
        case 4:
            actions.push({ text: qsTr("停止"), cmd: _cmdStop, warn: true })
            break
        case 3:
            actions.push({ text: qsTr("重新校准"), cmd: _cmdCalibrate, warn: false })
            break
        default:
            break
        }

        return actions
    }

    function _sendPayloadCommand(cmdId) {
        if (activeVehicle) {
            activeVehicle.sendCommand(_payloadCompId, cmdId, false)
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    QGCToolInsets {
        id: _toolInsets
        leftEdgeTopInset: parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset: parentToolInsets.leftEdgeCenterInset
        leftEdgeBottomInset: parentToolInsets.leftEdgeBottomInset
        rightEdgeTopInset: Math.max(parentToolInsets.rightEdgeTopInset, summaryStrip.visible ? summaryStrip.y + summaryStrip.height + _toolsMargin : parentToolInsets.rightEdgeTopInset)
        rightEdgeCenterInset: parentToolInsets.rightEdgeCenterInset
        rightEdgeBottomInset: Math.max(parentToolInsets.rightEdgeBottomInset, instrumentPanel.height + _toolsMargin * 1.5)
        topEdgeLeftInset: parentToolInsets.topEdgeLeftInset
        topEdgeCenterInset: parentToolInsets.topEdgeCenterInset
        topEdgeRightInset: Math.max(parentToolInsets.topEdgeRightInset, summaryStrip.visible ? summaryStrip.x + summaryStrip.width + _toolsMargin : parentToolInsets.topEdgeRightInset)
        bottomEdgeLeftInset: parentToolInsets.bottomEdgeLeftInset
        bottomEdgeCenterInset: Math.max(parentToolInsets.bottomEdgeCenterInset, actionBar.visible ? actionBar.height + _toolsMargin * 1.5 : parentToolInsets.bottomEdgeCenterInset)
        bottomEdgeRightInset: Math.max(parentToolInsets.bottomEdgeRightInset, instrumentPanel.width + _toolsMargin * 1.5)
    }

    // 1. 顶部警告横幅 (仅致命级)
    Rectangle {
        id: warningBanner
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: parentToolInsets.topEdgeCenterInset + ScreenTools.defaultFontPixelHeight * 0.6
        width: warningLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 4
        height: warningLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.75
        color: qgcPal.colorRed
        radius: ScreenTools.defaultFontPixelWidth * 0.5
        visible: isAttitudeCritical || _capsuleStatus === 3
        opacity: 0.95
        z: 1000

        QGCLabel {
            id: warningLabel
            anchors.centerIn: parent
            text: _capsuleStatus === 3
                  ? qsTr("载荷故障，请检查采样模块")
                  : qsTr("姿态异常  R:%1°  P:%2°").arg(Number(roll).toFixed(1)).arg(Number(pitch).toFixed(1))
            color: "white"
            font.bold: true
        }
    }

    // 2. 右上角：载荷摘要条
    USVPayloadSummaryStrip {
        id: summaryStrip
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.topMargin: 0
        anchors.rightMargin: 0
        vehicle: activeVehicle
        visible: !!activeVehicle
        z: 999
        onClicked: {
            detailDrawer.isExpanded = !detailDrawer.isExpanded
            summaryStrip.isExpanded = detailDrawer.isExpanded
        }
    }

    // 3. 右侧抽屉：载荷详情
    USVPayloadDetailPanel {
        id: detailDrawer
        anchors.top: summaryStrip.bottom
        anchors.right: parent.right
        anchors.topMargin: _toolsMargin * 0.5
        anchors.rightMargin: 0
        vehicle: activeVehicle
        maxHeight: Math.max(0, instrumentPanel.y > 0 ? (instrumentPanel.y - summaryStrip.y - summaryStrip.height - _toolsMargin * 1.5) : (parent.height - instrumentPanel.height - summaryStrip.y - summaryStrip.height - _toolsMargin * 3))
        z: 998
    }

    // 4. 右下角：航行仪表 (解除隐式拦截，显式重构)
    USVInstrumentPanel {
        id: instrumentPanel
        anchors.bottom: parent.bottom
        anchors.right: parent.right
        anchors.bottomMargin: 0
        anchors.rightMargin: 0
        vehicle: activeVehicle
    }

    // 5. 底部居中：操作栏
    USVActionBar {
        id: actionBar
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parentToolInsets.bottomEdgeCenterInset + _toolsMargin
        vehicle: activeVehicle
        payloadStatus: _capsuleStatus
        z: 500
    }
}
