import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QGroundControl
import QGroundControl.FactControls
import QGroundControl.Controls

SettingsPage {
    property var  _settingsManager: QGroundControl.settingsManager
    property var  _ntrip:           _settingsManager.ntripSettings
    property Fact _enabled:         _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr:        QGroundControl.ntripManager
    property bool _isConnected:     _ntripMgr.connectionStatus === NTRIPManager.Connected
    property bool _isConnecting:    _ntripMgr.connectionStatus === NTRIPManager.Connecting
    property bool _isReconnecting:  _ntripMgr.connectionStatus === NTRIPManager.Reconnecting
    property bool _isError:         _ntripMgr.connectionStatus === NTRIPManager.Error
    property bool _isActive:        _enabled.rawValue
    property bool _hasHost:         _ntrip.ntripServerHostAddress.rawValue !== ""
    property real _textFieldWidth:  ScreenTools.defaultFontPixelWidth * 30

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Connection")
        visible:            _ntrip.visible

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            Rectangle {
                width:          ScreenTools.defaultFontPixelHeight * 0.75
                height:         width
                radius:         width / 2
                color: {
                    switch (_ntripMgr.connectionStatus) {
                    case NTRIPManager.Connected:    return qgcPal.colorGreen
                    case NTRIPManager.Connecting:
                    case NTRIPManager.Reconnecting: return qgcPal.colorOrange
                    case NTRIPManager.Error:        return qgcPal.colorRed
                    default:                        return qgcPal.colorGrey
                    }
                }
                Layout.alignment: Qt.AlignVCenter
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               _ntripMgr.statusMessage || qsTr("Disconnected")
                wrapMode:           Text.WordWrap
            }

            QGCButton {
                text: {
                    if (_isConnecting)      return qsTr("Connecting…")
                    if (_isReconnecting)    return qsTr("Reconnecting…")
                    if (_isConnected)       return qsTr("Disconnect")
                    return qsTr("Connect")
                }
                enabled:    !_isConnecting && (_isActive || _hasHost)
                onClicked:  _enabled.rawValue = !_enabled.rawValue
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            visible:            _isConnected && _ntripMgr.messagesReceived > 0
            text: {
                var parts = [qsTr("%1 messages").arg(_ntripMgr.messagesReceived),
                             formatBytes(_ntripMgr.bytesReceived)]
                if (_ntripMgr.dataRateBytesPerSec > 0)
                    parts.push(formatRate(_ntripMgr.dataRateBytesPerSec))
                if (_ntripMgr.ggaSource)
                    parts.push(qsTr("GGA: %1").arg(_ntripMgr.ggaSource))
                return parts.join(" · ")
            }
            font.pointSize:     ScreenTools.smallFontPointSize
            color:              qgcPal.colorGrey

            function formatBytes(bytes) {
                if (bytes < 1024) return bytes + " B"
                if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
                return (bytes / 1048576).toFixed(1) + " MB"
            }

            function formatRate(bytesPerSec) {
                if (bytesPerSec < 1024) return bytesPerSec.toFixed(0) + " B/s"
                return (bytesPerSec / 1024).toFixed(1) + " KB/s"
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            visible:            _isError && _ntripMgr.statusMessage !== ""
            text:               _ntripMgr.statusMessage
            wrapMode:           Text.WordWrap
            color:              qgcPal.colorRed
            font.pointSize:     ScreenTools.smallFontPointSize
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Server")
        visible:            _ntrip.ntripServerHostAddress.visible || _ntrip.ntripServerPort.visible ||
                            _ntrip.ntripUsername.visible || _ntrip.ntripPassword.visible

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            fact:               _ntrip.ntripServerHostAddress
            visible:            fact.visible
            enabled:            !_isActive
        }

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            fact:               _ntrip.ntripServerPort
            visible:            fact.visible
            enabled:            !_isActive
        }

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            label:              fact.shortDescription
            fact:               _ntrip.ntripUsername
            visible:            fact.visible
            enabled:            !_isActive
        }

        RowLayout {
            Layout.fillWidth:   true
            visible:            _ntrip.ntripPassword.visible
            spacing:            ScreenTools.defaultFontPixelWidth * 0.5

            LabelledFactTextField {
                id:                 passwordField
                Layout.fillWidth:           true
                textFieldPreferredWidth:    _textFieldWidth
                label:              fact.shortDescription
                fact:               _ntrip.ntripPassword
                textField.echoMode: _showPassword ? TextInput.Normal : TextInput.Password
                enabled:            !_isActive

                property bool _showPassword: false
            }

            QGCButton {
                text:       passwordField._showPassword ? qsTr("Hide") : qsTr("Show")
                onClicked:  passwordField._showPassword = !passwordField._showPassword
                Layout.alignment: Qt.AlignBottom
            }
        }

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               fact.shortDescription
            fact:               _ntrip.ntripUseTls
            visible:            fact.visible
            enabled:            !_isActive
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Mountpoint")
        visible:            _ntrip.ntripMountpoint.visible

        RowLayout {
            Layout.fillWidth:   true
            spacing:            ScreenTools.defaultFontPixelWidth

            LabelledFactTextField {
                Layout.fillWidth:           true
                textFieldPreferredWidth:    _textFieldWidth
                label:              fact.shortDescription
                fact:               _ntrip.ntripMountpoint
                enabled:            !_isActive
            }

            QGCButton {
                text:       qsTr("Browse")
                enabled:    !_isActive && _hasHost &&
                            _ntripMgr.mountpointFetchStatus !== NTRIPManager.FetchInProgress
                onClicked:  _ntripMgr.fetchMountpoints()
            }
        }

        QGCLabel {
            Layout.fillWidth:   true
            visible:            _ntripMgr.mountpointFetchStatus === NTRIPManager.FetchInProgress
            text:               qsTr("Fetching mountpoints…")
            color:              qgcPal.colorOrange
        }

        QGCLabel {
            Layout.fillWidth:   true
            visible:            _ntripMgr.mountpointFetchStatus === NTRIPManager.FetchError
            text:               _ntripMgr.mountpointFetchError
            color:              qgcPal.colorRed
            wrapMode:           Text.WordWrap
        }

        QGCListView {
            Layout.fillWidth:       true
            Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 20)
            visible:                _ntripMgr.mountpointModel && _ntripMgr.mountpointModel.count > 0
            model:                  _ntripMgr.mountpointModel
            spacing:                ScreenTools.defaultFontPixelHeight * 0.25

            delegate: Rectangle {
                required property int index
                required property var object

                width:      ListView.view.width
                height:     mountRow.height + ScreenTools.defaultFontPixelHeight * 0.5
                radius:     ScreenTools.defaultFontPixelHeight * 0.25
                color: {
                    if (object.mountpoint === _ntrip.ntripMountpoint.rawValue)
                        return qgcPal.buttonHighlight
                    return index % 2 === 0 ? qgcPal.windowShade : qgcPal.window
                }

                RowLayout {
                    id:             mountRow
                    anchors.left:   parent.left
                    anchors.right:  parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.margins: ScreenTools.defaultFontPixelWidth

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 0

                        RowLayout {
                            spacing: ScreenTools.defaultFontPixelWidth

                            QGCLabel {
                                text:       object.mountpoint
                                font.bold:  true
                                color:      object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                            ? qgcPal.buttonHighlightText : qgcPal.text
                            }
                            QGCLabel {
                                visible:    object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                text:       qsTr("(selected)")
                                font.pointSize: ScreenTools.smallFontPointSize
                                color:      qgcPal.buttonHighlightText
                            }
                        }

                        QGCLabel {
                            text: {
                                var parts = []
                                if (object.format) parts.push(object.format)
                                if (object.navSystem) parts.push(object.navSystem)
                                if (object.country) parts.push(object.country)
                                if (object.bitrate > 0) parts.push(object.bitrate + " bps")
                                if (object.distanceKm >= 0) parts.push(object.distanceKm.toFixed(1) + " km")
                                return parts.join(" · ")
                            }
                            font.pointSize: ScreenTools.smallFontPointSize
                            color:  object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                    ? qgcPal.buttonHighlightText : qgcPal.colorGrey
                        }
                    }

                    QGCButton {
                        text:       object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                    ? qsTr("Selected") : qsTr("Select")
                        enabled:    object.mountpoint !== _ntrip.ntripMountpoint.rawValue
                        onClicked:  _ntripMgr.selectMountpoint(object.mountpoint)
                    }
                }
            }
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("Options")
        visible:            _ntrip.ntripWhitelist.visible

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            label:              fact.shortDescription
            fact:               _ntrip.ntripWhitelist
            visible:            fact.visible
            textField.placeholderText: qsTr("e.g. 1005,1077,1087")
        }
    }

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("UDP Forwarding")
        visible:            _ntrip.ntripUdpForwardEnabled.visible

        FactCheckBoxSlider {
            Layout.fillWidth:   true
            text:               fact.shortDescription
            fact:               _ntrip.ntripUdpForwardEnabled
            visible:            fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            label:              fact.shortDescription
            fact:               _ntrip.ntripUdpTargetAddress
            visible:            fact.visible
            enabled:            _ntrip.ntripUdpForwardEnabled.rawValue
        }

        LabelledFactTextField {
            Layout.fillWidth:           true
            textFieldPreferredWidth:    _textFieldWidth
            label:              fact.shortDescription
            fact:               _ntrip.ntripUdpTargetPort
            visible:            fact.visible
            enabled:            _ntrip.ntripUdpForwardEnabled.rawValue
        }
    }
}
