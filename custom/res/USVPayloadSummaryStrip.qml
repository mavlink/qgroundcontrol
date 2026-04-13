import QtQuick
import QtQuick.Layouts
import QtQuick.Controls

import QGroundControl
import QGroundControl.Controls

import "USVFlyViewLayout.js" as USVLayout

Rectangle {
    id: root

    property var vehicle: null
    property bool isExpanded: false
    signal clicked()

    width: layout.implicitWidth + _m * 3
    height: ScreenTools.defaultFontPixelHeight * 2.8
    radius: height / 2
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, USVLayout.Tokens.opacity.panel)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)

    property real _m: ScreenTools.defaultFontPixelWidth

    // ========== Fact 缓存 ==========
    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _voltageFact:     _hasPayloadGroup ? vehicle.getFact("usvPayload.voltage")    : null
    property var _absorbanceFact:  _hasPayloadGroup ? vehicle.getFact("usvPayload.absorbance") : null
    property var _statusFact:      _hasPayloadGroup ? vehicle.getFact("usvPayload.status")     : null
    property var _linkActiveFact:  _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null

    property int payloadStatus: _statusFact ? _statusFact.value : USVLayout.StatusIdle
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : !_hasPayloadGroup
    property bool _isWorking: payloadStatus === USVLayout.StatusSampling || payloadStatus === USVLayout.StatusDetecting || payloadStatus === USVLayout.StatusCalibrating

    function statusText(st) {
        if (st === USVLayout.StatusFault) return qsTr("故障")
        if (!_linkOk) return qsTr("离线")
        switch(st) {
            case USVLayout.StatusIdle:        return qsTr("空闲")
            case USVLayout.StatusSampling:    return qsTr("采样中")
            case USVLayout.StatusDetecting:   return qsTr("检测中")
            case USVLayout.StatusFault:       return qsTr("故障")
            case USVLayout.StatusCalibrating: return qsTr("校准中")
            default:                          return qsTr("未知")
        }
    }

    function statusColor(st) {
        if (st === USVLayout.StatusFault) return qgcPal.colorRed
        if (!_linkOk) return qgcPal.colorOrange
        switch(st) {
            case USVLayout.StatusIdle:        return qgcPal.text
            case USVLayout.StatusSampling:    return qgcPal.colorGreen
            case USVLayout.StatusDetecting:   return qgcPal.brandingBlue
            case USVLayout.StatusFault:       return qgcPal.colorRed
            case USVLayout.StatusCalibrating: return qgcPal.colorOrange
            default:                          return qgcPal.text
        }
    }

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    RowLayout {
        id: layout
        anchors.centerIn: parent
        spacing: _m * USVLayout.Tokens.spacing.lg

        // 状态区
        RowLayout {
            spacing: _m * USVLayout.Tokens.spacing.sm

            Rectangle {
                width: _m * 1.2
                height: width
                radius: width / 2
                color: statusColor(payloadStatus)

                SequentialAnimation on opacity {
                    running: _isWorking
                    loops: Animation.Infinite
                    NumberAnimation { to: 0.3; duration: 1000; easing.type: Easing.InOutSine }
                    NumberAnimation { to: 1.0; duration: 1000; easing.type: Easing.InOutSine }
                }
            }

            QGCLabel {
                text: qsTr("水质载荷")
                font.bold: true
            }

            QGCLabel {
                text: statusText(payloadStatus)
                color: statusColor(payloadStatus)
                font.pointSize: ScreenTools.smallFontPointSize
                font.bold: true
            }
        }

        // 分隔线
        Rectangle {
            width: 1
            height: ScreenTools.defaultFontPixelHeight * 1.2
            color: qgcPal.text
            opacity: 0.2
        }

        // 数据区
        RowLayout {
            spacing: _m * USVLayout.Tokens.spacing.lg
            opacity: _linkOk ? 1.0 : USVLayout.Tokens.opacity.subtle

            RowLayout {
                spacing: _m * USVLayout.Tokens.spacing.xs
                QGCLabel { 
                    text: _absorbanceFact ? _absorbanceFact.valueString : "--"
                    color: qgcPal.brandingBlue
                    font.bold: true
                    font.pointSize: ScreenTools.mediumFontPointSize
                }
                QGCLabel { 
                    text: "Abs"
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: USVLayout.Tokens.opacity.subtle 
                }
            }

            RowLayout {
                spacing: _m * USVLayout.Tokens.spacing.xs
                QGCLabel { 
                    text: _voltageFact ? _voltageFact.valueString : "--"
                    font.bold: true
                    font.pointSize: ScreenTools.defaultFontPointSize
                }
                QGCLabel { 
                    text: "V"
                    font.pointSize: ScreenTools.smallFontPointSize
                    opacity: USVLayout.Tokens.opacity.subtle 
                }
            }
        }

        // 展开箭头
        QGCLabel {
            text: root.isExpanded ? "▼" : "▶"
            font.pointSize: ScreenTools.smallFontPointSize
            opacity: USVLayout.Tokens.opacity.subtle
        }
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true
        cursorShape: Qt.PointingHandCursor
        onClicked: {
            root.clicked()
        }
        
        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: qgcPal.text
            opacity: parent.containsMouse ? 0.05 : 0
        }
    }
}
