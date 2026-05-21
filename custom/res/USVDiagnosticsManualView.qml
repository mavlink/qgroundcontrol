import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.LocalStorage

import QGroundControl
import QGroundControl.Controls

Item {
    id: root

    property var vehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _hasPayloadGroup: vehicle && vehicle.factGroupNames.indexOf("usvPayload") >= 0
    property var _payloadGroup: _hasPayloadGroup ? vehicle.getFactGroup("usvPayload") : null
    property var _linkActiveFact: _hasPayloadGroup ? vehicle.getFact("usvPayload.linkActive") : null
    property bool _linkOk: _linkActiveFact ? Number(_linkActiveFact.value) >= 1 : false

    property string webBaseUrl: "http://10.42.0.1:5000"
    property string webState: qsTr("未测试")
    property string mavrosState: "--"
    property string routerState: "--"
    property string bridgeState: "--"
    property string manualState: "--"
    property string lastCommandState: "--"
    property bool requestPending: false
    property string selectedAxis: "X"
    property string selectedDirection: "F"
    property bool continuousMode: false
    property real speedRpm: 5
    property real angleDeg: 10

    readonly property real _m: ScreenTools.defaultFontPixelWidth

    function _db() {
        return LocalStorage.openDatabaseSync("USVDiagnosticsManual", "1.0", "USV diagnostics manual settings", 1024)
    }

    function _loadSettings() {
        var db = _db()
        db.transaction(function(tx) {
            tx.executeSql("CREATE TABLE IF NOT EXISTS settings(key TEXT UNIQUE, value TEXT)")
            var rs = tx.executeSql("SELECT value FROM settings WHERE key='webBaseUrl'")
            if (rs.rows.length > 0 && rs.rows.item(0).value.length > 0) {
                webBaseUrl = rs.rows.item(0).value
            }
        })
    }

    function _saveSettings() {
        var db = _db()
        db.transaction(function(tx) {
            tx.executeSql("CREATE TABLE IF NOT EXISTS settings(key TEXT UNIQUE, value TEXT)")
            tx.executeSql("INSERT OR REPLACE INTO settings VALUES(?, ?)", ["webBaseUrl", webBaseUrl])
        })
    }

    function _request(method, path, payload, callback) {
        var xhr = new XMLHttpRequest()
        xhr.open(method, webBaseUrl.replace(/\/$/, "") + path)
        xhr.setRequestHeader("Content-Type", "application/json")
        xhr.onreadystatechange = function() {
            if (xhr.readyState !== XMLHttpRequest.DONE) {
                return
            }
            var body = {}
            try {
                body = JSON.parse(xhr.responseText || "{}")
            } catch (e) {
                body = { success: false, message: xhr.responseText }
            }
            callback(xhr.status, body)
        }
        xhr.onerror = function() {
            callback(0, { success: false, message: qsTr("Jetson Web 不可达") })
        }
        xhr.send(payload ? JSON.stringify(payload) : "")
    }

    function refreshDiagnostics() {
        _request("GET", "/api/diagnostics/link", null, function(status, body) {
            webState = status === 200 ? qsTr("在线") : qsTr("不可达")
            if (body && body.data) {
                var data = body.data
                mavrosState = data.mavros && data.mavros.connected ? qsTr("在线") : qsTr("离线")
                routerState = data.nodes && data.nodes.mavlink_routerd ? qsTr("运行") : qsTr("离线")
                bridgeState = data.bridge && data.bridge.router_url ? data.bridge.router_url : "--"
            }
        })
        _request("GET", "/api/manual/status", null, function(status, body) {
            if (status === 200 && body && body.data) {
                manualState = body.data.enabled ? qsTr("手动模式") : qsTr("未启用")
            }
            if (body && body.events && body.events.length > 0) {
                var event = body.events[body.events.length - 1]
                lastCommandState = event.action + " / " + event.state + " / " + (event.message || "")
            }
        })
    }

    function setManualMode(enabled) {
        requestPending = true
        _request("POST", "/api/manual/mode", { enabled: enabled }, function(status, body) {
            requestPending = false
            manualState = body && body.success ? (enabled ? qsTr("手动模式") : qsTr("未启用")) : (body.message || qsTr("切换失败"))
            refreshDiagnostics()
        })
    }

    function sendManualStep() {
        requestPending = true
        _request("POST", "/api/manual/pump-step", {
            axis: selectedAxis,
            direction: selectedDirection,
            speed_rpm: speedRpm,
            angle_deg: angleDeg,
            continuous: continuousMode
        }, function(status, body) {
            requestPending = false
            lastCommandState = body && body.message ? body.message : qsTr("手动命令已返回")
            refreshDiagnostics()
        })
    }

    function stopAll() {
        requestPending = true
        _request("POST", "/api/manual/stop-all", {}, function(status, body) {
            requestPending = false
            lastCommandState = body && body.message ? body.message : qsTr("停止命令已返回")
            refreshDiagnostics()
        })
    }

    Component.onCompleted: {
        _loadSettings()
        refreshDiagnostics()
    }

    Timer {
        interval: 5000
        running: true
        repeat: true
        onTriggered: refreshDiagnostics()
    }

    QGCPalette {
        id: qgcPal
        colorGroupEnabled: enabled
    }

    Flickable {
        anchors.fill: parent
        contentWidth: width
        contentHeight: mainColumn.implicitHeight + _m * 4
        clip: true

        ColumnLayout {
            id: mainColumn
            width: Math.min(parent.width - _m * 4, ScreenTools.defaultFontPixelWidth * 110)
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: _m * 2
            spacing: _m * 1.2

            QGCLabel {
                text: qsTr("链路诊断 / 手动控制")
                font.pointSize: ScreenTools.largeFontPointSize
                font.bold: true
            }

            Rectangle {
                Layout.fillWidth: true
                height: urlRow.implicitHeight + _m * 1.4
                radius: _m * 0.5
                color: Qt.rgba(qgcPal.windowShade.r, qgcPal.windowShade.g, qgcPal.windowShade.b, 0.18)

                RowLayout {
                    id: urlRow
                    anchors.fill: parent
                    anchors.margins: _m * 0.7
                    spacing: _m

                    QGCLabel { text: qsTr("Jetson Web") }
                    QGCTextField {
                        id: urlField
                        Layout.fillWidth: true
                        text: webBaseUrl
                        onEditingFinished: {
                            webBaseUrl = text
                            _saveSettings()
                        }
                    }
                    QGCButton {
                        text: qsTr("测试")
                        onClicked: {
                            webBaseUrl = urlField.text
                            _saveSettings()
                            refreshDiagnostics()
                        }
                    }
                }
            }

            GridLayout {
                Layout.fillWidth: true
                columns: 2
                columnSpacing: _m
                rowSpacing: _m

                Rectangle {
                    Layout.fillWidth: true
                    height: diagGrid.implicitHeight + _m * 1.4
                    radius: _m * 0.5
                    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.72)
                    border.width: 1
                    border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)

                    GridLayout {
                        id: diagGrid
                        anchors.fill: parent
                        anchors.margins: _m * 0.7
                        columns: 2
                        rowSpacing: _m * 0.45
                        columnSpacing: _m

                        QGCLabel { text: qsTr("QGC MAVLink"); opacity: 0.6 }
                        QGCLabel { text: _linkOk ? qsTr("载荷在线") : qsTr("载荷离线"); color: _linkOk ? qgcPal.colorGreen : qgcPal.colorOrange; font.bold: true }
                        QGCLabel { text: qsTr("Web API"); opacity: 0.6 }
                        QGCLabel { text: webState; font.bold: true }
                        QGCLabel { text: qsTr("MAVROS"); opacity: 0.6 }
                        QGCLabel { text: mavrosState; font.bold: true }
                        QGCLabel { text: qsTr("Router"); opacity: 0.6 }
                        QGCLabel { text: routerState; font.bold: true }
                        QGCLabel { text: qsTr("Bridge"); opacity: 0.6 }
                        QGCLabel { text: bridgeState; elide: Text.ElideRight }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    height: manualGrid.implicitHeight + _m * 1.4
                    radius: _m * 0.5
                    color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.72)
                    border.width: 1
                    border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)

                    GridLayout {
                        id: manualGrid
                        anchors.fill: parent
                        anchors.margins: _m * 0.7
                        columns: 2
                        rowSpacing: _m * 0.45
                        columnSpacing: _m

                        QGCLabel { text: qsTr("手动状态"); opacity: 0.6 }
                        QGCLabel { text: manualState; font.bold: true }
                        QGCLabel { text: qsTr("最近命令"); opacity: 0.6 }
                        QGCLabel { text: lastCommandState; Layout.fillWidth: true; wrapMode: Text.WordWrap }
                        QGCButton {
                            text: qsTr("进入手动")
                            enabled: !requestPending
                            onClicked: setManualMode(true)
                        }
                        QGCButton {
                            text: qsTr("退出手动")
                            enabled: !requestPending
                            onClicked: setManualMode(false)
                        }
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                height: controlLayout.implicitHeight + _m * 1.4
                radius: _m * 0.5
                color: Qt.rgba(qgcPal.window.r, qgcPal.window.g, qgcPal.window.b, 0.72)
                border.width: 1
                border.color: Qt.rgba(qgcPal.text.r, qgcPal.text.g, qgcPal.text.b, 0.12)

                GridLayout {
                    id: controlLayout
                    anchors.fill: parent
                    anchors.margins: _m * 0.7
                    columns: 4
                    columnSpacing: _m
                    rowSpacing: _m

                    QGCLabel { text: qsTr("轴") }
                    QGCComboBox {
                        model: ["X", "Y", "Z", "A"]
                        onCurrentTextChanged: selectedAxis = currentText
                    }
                    QGCLabel { text: qsTr("方向") }
                    QGCComboBox {
                        model: [qsTr("正转"), qsTr("反转")]
                        onCurrentIndexChanged: selectedDirection = currentIndex === 0 ? "F" : "B"
                    }
                    QGCLabel { text: qsTr("速度 RPM") }
                    SpinBox {
                        from: 0
                        to: 100
                        value: speedRpm
                        onValueModified: speedRpm = value
                    }
                    QGCLabel { text: qsTr("角度 deg") }
                    SpinBox {
                        from: 0
                        to: 99999
                        value: angleDeg
                        enabled: !continuousMode
                        onValueModified: angleDeg = value
                    }
                    QGCCheckBox {
                        text: qsTr("连续")
                        checked: continuousMode
                        onClicked: continuousMode = checked
                    }
                    Item { Layout.fillWidth: true }
                    QGCButton {
                        text: requestPending ? qsTr("发送中") : qsTr("发送")
                        enabled: !requestPending
                        onClicked: sendManualStep()
                    }
                    QGCButton {
                        text: qsTr("停止全部")
                        enabled: !requestPending
                        onClicked: stopAll()
                    }
                }
            }
        }
    }
}
