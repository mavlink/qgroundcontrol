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
    property real maxHeight: 9999

    property real _m: ScreenTools.defaultFontPixelWidth

    width: _m * 24
    height: isExpanded ? Math.min(layout.implicitHeight + _m * 3, maxHeight) : 0
    radius: _m * USVLayout.Tokens.radius.md
    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, USVLayout.Tokens.opacity.panel)
    border.width: 1
    border.color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.45)
    clip: true
    visible: height > 0

    Behavior on height { NumberAnimation { duration: 200; easing.type: Easing.InOutQuad } }

    // ========== Fact 缓存 ==========
    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _pumpXFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpX")      : null
    property var _pumpYFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpY")      : null
    property var _pumpZFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpZ")      : null
    property var _pumpAFact:       _hasPayloadGroup ? vehicle.getFact("usvPayload.pumpA")      : null
    property var _statusFact:      _hasPayloadGroup ? vehicle.getFact("usvPayload.status")     : null
    property var _linkActiveFact:  _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null
    
    property int payloadStatus: _statusFact ? _statusFact.value : USVLayout.StatusIdle
    property bool _linkOk: _linkActiveFact ? _linkActiveFact.value === 1 : !_hasPayloadGroup

    QGCPalette { id: qgcPal; colorGroupEnabled: enabled }

    ColumnLayout {
        id: layout
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.margins: _m * 1.5
        spacing: _m * USVLayout.Tokens.spacing.lg

        // 链路警告
        Rectangle {
            Layout.fillWidth: true
            height: warnLabel.implicitHeight + _m * 1.5
            color: Qt.rgba(qgcPal.colorRed.r, qgcPal.colorRed.g, qgcPal.colorRed.b, 0.12)
            radius: _m * USVLayout.Tokens.radius.sm
            visible: _hasPayloadGroup && !_linkOk

            QGCLabel {
                id: warnLabel
                anchors.centerIn: parent
                text: qsTr("遥测超时")
                color: qgcPal.colorRed
                font.pointSize: ScreenTools.smallFontPointSize
                font.bold: true
            }
        }

        // 泵组角度
        Rectangle {
            Layout.fillWidth: true
            height: pumpRow.implicitHeight + _m * 1.5
            color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.3)
            radius: _m * USVLayout.Tokens.radius.sm
            opacity: _linkOk ? 1.0 : USVLayout.Tokens.opacity.subtle

            RowLayout {
                id: pumpRow
                anchors.centerIn: parent
                width: parent.width - _m * 2
                spacing: 0

                Repeater {
                    model: [
                        { label: "X", fact: root._pumpXFact },
                        { label: "Y", fact: root._pumpYFact },
                        { label: "Z", fact: root._pumpZFact },
                        { label: "A", fact: root._pumpAFact }
                    ]
                    delegate: RowLayout {
                        Layout.fillWidth: true
                        spacing: _m * USVLayout.Tokens.spacing.xs
                        
                        QGCLabel {
                            text: modelData.label
                            font.pointSize: ScreenTools.smallFontPointSize
                            opacity: USVLayout.Tokens.opacity.subtle
                        }
                        QGCLabel {
                            text: (modelData.fact && modelData.fact.value !== undefined) ? Number(modelData.fact.value).toFixed(1) + "°" : "--"
                            font.bold: true
                            font.pointSize: ScreenTools.smallFontPointSize
                        }
                        Rectangle {
                            width: 1; height: ScreenTools.defaultFontPixelHeight * 0.7
                            color: qgcPal.text; opacity: 0.15
                            visible: index < 3
                            Layout.leftMargin: _m * USVLayout.Tokens.spacing.sm
                        }
                    }
                }
            }
        }

        // 通信诊断折叠面板
        Rectangle {
            id: diagRect
            Layout.fillWidth: true
            height: diagCol.implicitHeight
            color: Qt.rgba(0, 0, 0, 0.05)
            radius: _m * USVLayout.Tokens.radius.sm
            visible: _hasPayloadGroup

            property bool diagExpanded: false
            property var payloadGroup: _hasPayloadGroup ? vehicle.getFactGroup("usvPayload") : null

            ColumnLayout {
                id: diagCol
                width: parent.width
                spacing: _m * USVLayout.Tokens.spacing.xs

                // 标题栏
                Item {
                    Layout.fillWidth: true
                    height: diagTitleRow.implicitHeight + _m * 1.5

                    RowLayout {
                        id: diagTitleRow
                        anchors.fill: parent
                        anchors.margins: _m * USVLayout.Tokens.spacing.sm

                        QGCLabel {
                            text: diagRect.diagExpanded ? "▼ 通信诊断" : "▶ 通信诊断"
                            font.pointSize: ScreenTools.smallFontPointSize
                            font.bold: true
                            opacity: 0.7
                        }
                        Item { Layout.fillWidth: true }
                        Rectangle {
                            width: _m * 0.8; height: _m * 0.8; radius: width / 2
                            color: _linkOk ? qgcPal.colorGreen : qgcPal.colorRed
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: diagRect.diagExpanded = !diagRect.diagExpanded
                    }
                }

                // 展开内容
                GridLayout {
                    columns: 2
                    columnSpacing: _m * 0.8
                    rowSpacing: _m * 0.2
                    Layout.leftMargin: _m
                    Layout.rightMargin: _m
                    Layout.bottomMargin: _m * 0.5
                    visible: diagRect.diagExpanded

                    QGCLabel { text: "RX总计"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                    QGCLabel { text: diagRect.payloadGroup ? diagRect.payloadGroup.rxMsgTotal : "0"; font.pointSize: ScreenTools.smallFontPointSize; font.bold: true }
                    QGCLabel { text: "Named值"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                    QGCLabel { text: diagRect.payloadGroup ? diagRect.payloadGroup.rxNamedValue : "0"; font.pointSize: ScreenTools.smallFontPointSize; font.bold: true }
                    QGCLabel { text: "心跳"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                    QGCLabel { text: diagRect.payloadGroup ? diagRect.payloadGroup.rxHeartbeat : "0"; font.pointSize: ScreenTools.smallFontPointSize; font.bold: true }
                    QGCLabel { text: "心跳间隔"; font.pointSize: ScreenTools.smallFontPointSize; opacity: 0.5 }
                    QGCLabel { text: diagRect.payloadGroup ? Number(diagRect.payloadGroup.latencyMs).toFixed(0) + " ms" : "--"; font.pointSize: ScreenTools.smallFontPointSize; font.bold: true }
                }
            }
        }
    }
}
