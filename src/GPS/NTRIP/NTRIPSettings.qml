import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ColumnLayout {
    id: root
    spacing: ScreenTools.defaultFontPixelHeight / 2

    property var  _ntrip:          QGroundControl.settingsManager.ntripSettings
    property Fact _ntripEnabled:   _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr:       QGroundControl.ntripManager
    property bool _isConnected:    _ntripMgr.connectionStatus === NTRIPManager.Connected
    property bool _isConnecting:   _ntripMgr.connectionStatus === NTRIPManager.Connecting
    property bool _isReconnecting: _ntripMgr.connectionStatus === NTRIPManager.Reconnecting
    property bool _isError:        _ntripMgr.connectionStatus === NTRIPManager.Error
    property bool _isActive:       _ntripEnabled.rawValue
    property bool _hasHost:        _ntrip.ntripServerHostAddress.rawValue !== ""
    property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 30

    function formatBytes(bytes) {
        if (bytes < 1024) return bytes + " B"
        if (bytes < 1048576) return (bytes / 1024).toFixed(1) + " KB"
        return (bytes / 1048576).toFixed(1) + " MB"
    }

    function formatRate(bytesPerSec) {
        if (bytesPerSec < 1024) return bytesPerSec.toFixed(0) + " B/s"
        return (bytesPerSec / 1024).toFixed(1) + " KB/s"
    }

    QGCPalette { id: qgcPal }

    // -- Connection Status ---------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth:   true
        heading:            qsTr("NTRIP Connection")
        headingDescription: qsTr("Receive RTK corrections from an NTRIP caster")
        showDividers:       true
        visible:            _ntrip.visible

        ConnectionStatusRow {
            statusColor: {
                switch (_ntripMgr.connectionStatus) {
                case NTRIPManager.Connected:    return qgcPal.colorGreen
                case NTRIPManager.Connecting:
                case NTRIPManager.Reconnecting: return qgcPal.colorOrange
                case NTRIPManager.Error:        return qgcPal.colorRed
                default:                        return qgcPal.colorGrey
                }
            }
            statusText: _ntripMgr.statusMessage || qsTr("Disconnected")
            buttonText: {
                if (_isConnecting)   return qsTr("Connecting...")
                if (_isReconnecting) return qsTr("Reconnecting...")
                if (_isActive)       return qsTr("Disconnect")
                return qsTr("Connect")
            }
            buttonEnabled: _isActive ? !_isConnecting : _hasHost
            onButtonClicked: _ntripEnabled.rawValue = !_ntripEnabled.rawValue
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _isConnected && _ntripMgr.connectionStats.messagesReceived > 0
            text: {
                var parts = [qsTr("%1 messages").arg(_ntripMgr.connectionStats.messagesReceived),
                             formatBytes(_ntripMgr.connectionStats.bytesReceived)]
                if (_ntripMgr.connectionStats.dataRateBytesPerSec > 0)
                    parts.push(formatRate(_ntripMgr.connectionStats.dataRateBytesPerSec))
                if (_ntripMgr.ggaSource)
                    parts.push(qsTr("GGA: %1").arg(_ntripMgr.ggaSource))
                return parts.join(" \u00B7 ")
            }
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorGrey
        }

        // Connected summary: show server/mountpoint inline
        LabelledLabel {
            label:     qsTr("Server")
            labelText: {
                var host = _ntrip.ntripServerHostAddress.rawValue
                var port = _ntrip.ntripServerPort.rawValue
                var mp = _ntrip.ntripMountpoint.rawValue
                var s = host + ":" + port
                if (mp) s += "/" + mp
                return s
            }
            visible: _isActive
        }

        QGCLabel {
            Layout.fillWidth: true
            visible:  _isError && _ntripMgr.statusMessage !== ""
            text:     _ntripMgr.statusMessage
            wrapMode: Text.WordWrap
            color:    qgcPal.colorRed
            font.pointSize: ScreenTools.smallFontPointSize
        }
    }

    // -- Server Settings -----------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:          qsTr("NTRIP Server")
        showDividers:     true
        visible:          !_isActive
                          && (_ntrip.ntripServerHostAddress.visible || _ntrip.ntripServerPort.visible ||
                              _ntrip.ntripUsername.visible || _ntrip.ntripPassword.visible)

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            fact:    _ntrip.ntripServerHostAddress
            visible: fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            fact:    _ntrip.ntripServerPort
            visible: fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            label:   fact.shortDescription
            fact:    _ntrip.ntripUsername
            visible: fact.visible
        }

        RowLayout {
            Layout.fillWidth: true
            visible: _ntrip.ntripPassword.visible
            spacing: ScreenTools.defaultFontPixelWidth * 0.5

            LabelledFactTextField {
                id:                      passwordField
                Layout.fillWidth:        true
                textFieldPreferredWidth: _textFieldWidth
                label:              fact.shortDescription
                fact:               _ntrip.ntripPassword
                textField.echoMode: _showPassword ? TextInput.Normal : TextInput.Password

                property bool _showPassword: false
            }

            QGCButton {
                text:    passwordField._showPassword ? qsTr("Hide") : qsTr("Show")
                onClicked: passwordField._showPassword = !passwordField._showPassword
                Layout.alignment: Qt.AlignBottom
            }
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:    fact.shortDescription
            fact:    _ntrip.ntripUseTls
            visible: fact.visible
        }
    }

    // -- Mountpoint ----------------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:          qsTr("NTRIP Mountpoint")
        showDividers:     true
        visible:          !_isActive && _ntrip.ntripMountpoint.visible

        property bool _showBrowseList: false

        RowLayout {
            Layout.fillWidth: true
            spacing: ScreenTools.defaultFontPixelWidth

            LabelledFactTextField {
                Layout.fillWidth:        true
                textFieldPreferredWidth: _textFieldWidth
                label:   fact.shortDescription
                fact:    _ntrip.ntripMountpoint
            }

            QGCButton {
                text:    qsTr("Browse")
                enabled: _hasHost &&
                         _ntripMgr.sourceTableController.fetchStatus !== NTRIPSourceTableController.InProgress
                onClicked: {
                    parent.parent._showBrowseList = true
                    _ntripMgr.fetchMountpoints()
                }
            }
        }

        QGCLabel {
            Layout.fillWidth: true
            visible: _ntripMgr.sourceTableController.fetchStatus === NTRIPSourceTableController.InProgress
            text:    qsTr("Fetching mountpoints...")
            color:   qgcPal.colorOrange
        }

        QGCLabel {
            Layout.fillWidth: true
            visible:  _ntripMgr.sourceTableController.fetchStatus === NTRIPSourceTableController.Error
            text:     _ntripMgr.sourceTableController.fetchError
            color:    qgcPal.colorRed
            wrapMode: Text.WordWrap
        }

        QGCListView {
            id: mountpointList
            Layout.fillWidth:       true
            Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 20)
            visible: !!parent._showBrowseList
                     && !!_ntripMgr.sourceTableController.mountpointModel
                     && _ntripMgr.sourceTableController.mountpointModel.count > 0
            model:   _ntripMgr.sourceTableController.mountpointModel
            spacing: ScreenTools.defaultFontPixelHeight * 0.25

            delegate: Rectangle {
                required property int index
                required property var object

                width:  ListView.view.width
                height: mountRow.height + ScreenTools.defaultFontPixelHeight * 0.5
                radius: ScreenTools.defaultFontPixelHeight * 0.25
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
                                text:      object.mountpoint
                                font.bold: true
                                color:     object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                           ? qgcPal.buttonHighlightText : qgcPal.text
                            }
                            QGCLabel {
                                visible:        object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                text:           qsTr("(selected)")
                                font.pointSize: ScreenTools.smallFontPointSize
                                color:          qgcPal.buttonHighlightText
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
                                return parts.join(" \u00B7 ")
                            }
                            font.pointSize: ScreenTools.smallFontPointSize
                            color: object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                   ? qgcPal.buttonHighlightText : qgcPal.colorGrey
                        }
                    }

                    QGCButton {
                        text:    object.mountpoint === _ntrip.ntripMountpoint.rawValue
                                 ? qsTr("Selected") : qsTr("Select")
                        enabled: object.mountpoint !== _ntrip.ntripMountpoint.rawValue
                        onClicked: {
                            _ntripMgr.sourceTableController.selectMountpoint(object.mountpoint)
                            mountpointList.parent._showBrowseList = false
                        }
                    }
                }
            }
        }
    }

    // -- Options & Forwarding ------------------------------------------------

    SettingsGroupLayout {
        Layout.fillWidth: true
        heading:          qsTr("NTRIP Options")
        headingDescription: qsTr("Message filtering, GGA position source, and UDP forwarding")
        showDividers:     true

        FactComboBox {
            Layout.fillWidth: true
            fact:    _ntrip.ntripGgaPositionSource
            visible: fact.visible
            indexModel: false
        }

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            label:   fact.shortDescription
            fact:    _ntrip.ntripWhitelist
            visible: fact.visible
            textField.placeholderText: qsTr("e.g. 1005,1077,1087")
        }

        FactCheckBoxSlider {
            Layout.fillWidth: true
            text:    fact.shortDescription
            fact:    _ntrip.ntripUdpForwardEnabled
            visible: fact.visible
        }

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            label:   fact.shortDescription
            fact:    _ntrip.ntripUdpTargetAddress
            visible: fact.visible
            enabled: _ntrip.ntripUdpForwardEnabled.rawValue
        }

        LabelledFactTextField {
            Layout.fillWidth:        true
            textFieldPreferredWidth: _textFieldWidth
            label:   fact.shortDescription
            fact:    _ntrip.ntripUdpTargetPort
            visible: fact.visible
            enabled: _ntrip.ntripUdpForwardEnabled.rawValue
        }
    }
}
