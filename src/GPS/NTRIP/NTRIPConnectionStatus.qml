import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

ColumnLayout {
    id: root

    property var rtcmMavlink: null

    property var    _ntripMgr:      QGroundControl.ntripManager
    readonly property int  _status: root._ntripMgr ? root._ntripMgr.connectionStatus : NTRIPManager.Disconnected
    readonly property var  _stats:  root._ntripMgr ? root._ntripMgr.connectionStats : null
    readonly property bool _connected: root._status === NTRIPManager.Connected
    property bool   _dataStale:     root._stats ? root._stats.dataStale : false
    property string _valueNA:       qsTr("-.--")

    readonly property real _dataWarningLimitBytes: 50 * 1024 * 1024  // 50 MB

    function _formatDataSize(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / 1048576).toFixed(1) + " MB"
    }

    function _formatDataRate(bytesPerSec) {
        if (bytesPerSec < 1024) return bytesPerSec.toFixed(0) + " B/s"
        return (bytesPerSec / 1024).toFixed(1) + " KB/s"
    }

    spacing:      ScreenTools.defaultFontPixelHeight / 2
    visible:      root._status !== NTRIPManager.Disconnected

    QGCPalette { id: qgcPal }

    QGCLabel {
        text:             qsTr("Connected but no data received recently")
        color:            qgcPal.colorOrange
        wrapMode:         Text.WordWrap
        font.pointSize:   ScreenTools.smallFontPointSize
        visible:          root._dataStale && root._connected
        Layout.fillWidth: true
    }

    LabelledLabel {
        label:     qsTr("Mountpoint")
        labelText: QGroundControl.settingsManager.ntripSettings.ntripMountpoint.valueString
        visible:   QGroundControl.settingsManager.ntripSettings.ntripMountpoint.valueString !== ""
                   && root._connected
    }

    LabelledLabel {
        label:     qsTr("Messages")
        labelText: root._stats ? root._stats.messagesReceived : ""
        visible:   root._connected
                   && root._stats && root._stats.messagesReceived > 0
    }

    ColumnLayout {
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root._connected
                 && root._stats && root._stats.messageCountsById.length > 0

        QGCLabel {
            text: qsTr("Message Types")
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorGrey
        }

        Flow {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth / 2

            Repeater {
                model: root._stats ? root._stats.messageCountsById : []

                delegate: Rectangle {
                    id: chip
                    required property var modelData

                    readonly property string _idLabel: chip.modelData[0] === 0
                        ? qsTr("unknown")
                        : chip.modelData[0].toString()

                    implicitWidth:  chipLabel.implicitWidth + ScreenTools.defaultFontPixelWidth * 1.2
                    implicitHeight: chipLabel.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.3
                    radius:         implicitHeight / 2
                    color:          qgcPal.windowShade
                    border.color:   qgcPal.groupBorder
                    border.width:   1

                    QGCLabel {
                        id: chipLabel
                        anchors.centerIn: parent
                        text: qsTr("%1 × %2").arg(chip._idLabel).arg(chip.modelData[1])
                        font.pointSize: ScreenTools.smallFontPointSize
                    }
                }
            }
        }
    }

    LabelledLabel {
        label:     qsTr("Data Received")
        labelText: root._stats ? (root._formatDataSize(root._stats.bytesReceived) + " ("
                   + root._formatDataRate(root._stats.dataRateBytesPerSec) + ")") : root._valueNA
        visible:   root._connected
                   && root._stats && root._stats.bytesReceived > 0
    }

    QGCLabel {
        text:           qsTr("Warning: Data usage: %1 — consider connection costs").arg(root._stats ? root._formatDataSize(root._stats.bytesReceived) : "")
        wrapMode:       Text.WordWrap
        color:          qgcPal.warningText
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._connected
                        && root._stats && root._stats.bytesReceived > root._dataWarningLimitBytes
        Layout.fillWidth: true
    }

    LabelledLabel {
        label:     qsTr("To Vehicle")
        labelText: root.rtcmMavlink ? (root._formatDataSize(root.rtcmMavlink.totalBytesSent) + " ("
                   + root.rtcmMavlink.bandwidthKBps.toFixed(1) + " KB/s)") : root._valueNA
        visible:   root._connected
                   && root.rtcmMavlink && root.rtcmMavlink.totalBytesSent > 0
    }

    LabelledLabel {
        label:     qsTr("GGA Source")
        labelText: (root._ntripMgr ? root._ntripMgr.ggaSource : "")
        visible:   root._connected
                   && (root._ntripMgr ? root._ntripMgr.ggaSource : "")
    }

    QGCLabel {
        text:           root._ntripMgr ? root._ntripMgr.securityWarning : ""
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorOrange
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._connected
                        && root._ntripMgr
                        && root._ntripMgr.securityWarning !== ""
        Layout.fillWidth: true
    }

    QGCLabel {
        text:           (root._ntripMgr ? root._ntripMgr.statusMessage : "")
        wrapMode:       Text.WordWrap
        color:          qgcPal.colorRed
        font.pointSize: ScreenTools.smallFontPointSize
        visible:        root._status === NTRIPManager.Error
                        && (root._ntripMgr ? root._ntripMgr.statusMessage : "") !== ""
        Layout.fillWidth: true
    }
}
