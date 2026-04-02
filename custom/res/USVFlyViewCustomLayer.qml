/****************************************************************************
 *
 * USV Fly View Custom Layer - 无人船飞行视图自定义层
 *
 * 包含：
 * - 右下角：USV 综合仪表盘（罗盘 + 航行状态 + 姿态监测）
 * - 左侧：载荷控制面板
 * - 顶部：姿态危险警告横幅
 * - 底部居中：采样状态胶囊
 *
 ****************************************************************************/
import USV 1.0
import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightMap

/// @brief 无人船飞行视图自定义层
Item {
    id: _root

    // ========== 必需的属性 - 与 QGC 原生接口保持一致 ==========
    property var parentToolInsets
    property var totalToolInsets:   _toolInsets
    property var mapControl

    // 更新边距以适应左下角的仪表盘
    QGCToolInsets {
        id:                     _toolInsets
        leftEdgeTopInset:       parentToolInsets.leftEdgeTopInset
        leftEdgeCenterInset:    parentToolInsets.leftEdgeCenterInset
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

    // ========== USV 自定义属性 ==========
    property var  activeVehicle:    QGroundControl.multiVehicleManager.activeVehicle
    property real roll:             activeVehicle && activeVehicle.roll ? activeVehicle.roll.rawValue : 0
    property real pitch:            activeVehicle && activeVehicle.pitch ? activeVehicle.pitch.rawValue : 0
    property real _toolsMargin:     ScreenTools.defaultFontPixelWidth * 0.75

    // 姿态警告阈值
    property real rollCriticalThreshold:  25.0
    property real pitchCriticalThreshold: 20.0
    property bool isAttitudeCritical:     Math.abs(roll) > rollCriticalThreshold ||
                                          Math.abs(pitch) > pitchCriticalThreshold

    QGCPalette { id: qgcPal; colorGroupEnabled: true }

    // ========== 姿态危险警告横幅 (顶部居中) ==========
    Rectangle {
        id:                         warningBanner
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.top
        anchors.topMargin:          parentToolInsets.topEdgeCenterInset + ScreenTools.defaultFontPixelHeight
        width:                      warningLabel.width + ScreenTools.defaultFontPixelWidth * 4
        height:                     warningLabel.height + ScreenTools.defaultFontPixelHeight
        color:                      qgcPal.colorRed
        radius:                     ScreenTools.defaultFontPixelWidth / 2
        visible:                    isAttitudeCritical
        opacity:                    0.95
        z:                          1000

        SequentialAnimation on opacity {
            running:    warningBanner.visible
            loops:      Animation.Infinite
            NumberAnimation { to: 0.6; duration: 500 }
            NumberAnimation { to: 0.95; duration: 500 }
        }

        QGCLabel {
            id:                 warningLabel
            anchors.centerIn:   parent
            text:               qsTr("⚠️ 船体姿态异常 - 横滚: %1° 俯仰: %2° - 请检查水况或减速！")
                                    .arg(roll.toFixed(1)).arg(pitch.toFixed(1))
            color:              "white"
            font.bold:          true
        }
    }

    // ========== 右下角：USV 综合仪表盘 ==========
    IntegratedCompassAttitude {
        id:                     instrumentPanel
        anchors.bottom:         parent.bottom
        anchors.right:          parent.right
        anchors.bottomMargin:   parentToolInsets.bottomEdgeRightInset + _toolsMargin
        anchors.rightMargin:    _toolsMargin
        vehicle:                activeVehicle
    }

    // ========== 左侧：载荷控制面板 ==========
    USVPayloadPanel {
        id:                     payloadPanel
        anchors.left:           parent.left
        anchors.verticalCenter: parent.verticalCenter
        anchors.leftMargin:     parentToolInsets.leftEdgeCenterInset + _toolsMargin
        vehicle:                activeVehicle
        visible:                activeVehicle  // USV 专用构建，连接即显示
    }

    // ========== 地图顶部：采样状态胶囊 ==========
    Rectangle {
        id:                         statusCapsule
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.bottom:             parent.bottom
        anchors.bottomMargin:       parentToolInsets.bottomEdgeCenterInset + ScreenTools.defaultFontPixelHeight
        width:                      statusCapsuleRow.width + ScreenTools.defaultFontPixelWidth * 3
        height:                     statusCapsuleRow.height + ScreenTools.defaultFontPixelHeight * 0.6
        radius:                     height / 2
        color:                      _capsuleColor
        opacity:                    0.9
        visible:                    activeVehicle && _capsuleStatus !== 0
        z:                          500

        property int _capsuleStatus: {
            if (!activeVehicle) return 0
            try { return activeVehicle.getFact("usvPayload.status").value } catch(e) { return 0 }
        }

        property color _capsuleColor: {
            switch(_capsuleStatus) {
            case 1: return qgcPal.colorGreen   // sampling
            case 2: return qgcPal.brandingBlue  // detecting
            case 3: return qgcPal.colorRed      // fault
            case 4: return qgcPal.colorOrange   // calibrating
            default: return "transparent"
            }
        }

        property string _capsuleText: {
            switch(_capsuleStatus) {
            case 1: return qsTr("● 正在采样")
            case 2: return qsTr("● 正在检测")
            case 3: return qsTr("⚠ 载荷故障")
            case 4: return qsTr("● 校准中")
            default: return ""
            }
        }

        Row {
            id: statusCapsuleRow
            anchors.centerIn: parent
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            QGCLabel {
                text:           statusCapsule._capsuleText
                color:          "white"
                font.bold:      true
                anchors.verticalCenter: parent.verticalCenter
            }
        }

        SequentialAnimation on opacity {
            running: statusCapsule._capsuleStatus === 1 || statusCapsule._capsuleStatus === 4
            loops:   Animation.Infinite
            NumberAnimation { to: 0.6; duration: 800 }
            NumberAnimation { to: 0.9; duration: 800 }
        }
    }
}
