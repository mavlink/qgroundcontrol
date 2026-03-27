import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS
import QGroundControl.GPS.NTRIP

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("NTRIP Server")
    visible:            _ntrip.ntripServerHostAddress.userVisible || _ntrip.ntripServerPort.userVisible ||
                        _ntrip.ntripUsername.userVisible || _ntrip.ntripPassword.userVisible

    QGCPalette { id: qgcPal }

    property var  _ntrip:       QGroundControl.settingsManager.ntripSettings
    property Fact _enabled:     _ntrip.ntripServerConnectEnabled
    property var  _ntripMgr:    QGroundControl.ntripManager
    property var  _stats:       _ntripMgr.connectionStats
    property var  _stc:         _ntripMgr.sourceTableController
    property bool _isActive:    _enabled.rawValue
    property bool _hasHost:     _ntrip.ntripServerHostAddress.rawValue !== ""
    property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 30

    NTRIPConnectionStatusRow {
        ntripManager:     _ntripMgr
        canConnect:       _isActive || _hasHost
        onButtonClicked:  _enabled.rawValue = !_enabled.rawValue
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _ntripMgr.connectionStatus === NTRIPManager.Connected && _stats.messagesReceived > 0
        text: {
            var parts = [qsTr("%1 messages").arg(_stats.messagesReceived),
                         GPSFormatter.formatDataSize(_stats.bytesReceived)]
            if (_stats.dataRateBytesPerSec > 0)
                parts.push(GPSFormatter.formatDataRate(_stats.dataRateBytesPerSec))
            if (_ntripMgr.ggaSource)
                parts.push(qsTr("GGA: %1").arg(_ntripMgr.ggaSource))
            return parts.join(" · ")
        }
        font.pointSize:     ScreenTools.smallFontPointSize
        color:              qgcPal.colorGrey
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _ntripMgr.connectionStatus === NTRIPManager.Error && _ntripMgr.statusMessage !== ""
        text:               _ntripMgr.statusMessage
        wrapMode:           Text.WordWrap
        color:              qgcPal.colorRed
        font.pointSize:     ScreenTools.smallFontPointSize
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        fact:               _ntrip.ntripServerHostAddress
        visible:            fact.userVisible
        enabled:            !_isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        fact:               _ntrip.ntripServerPort
        visible:            fact.userVisible
        enabled:            !_isActive
    }

    LabelledFactTextField {
        Layout.fillWidth:           true
        textFieldPreferredWidth:    _textFieldWidth
        label:              fact.shortDescription
        fact:               _ntrip.ntripUsername
        visible:            fact.userVisible
        enabled:            !_isActive
    }

    RowLayout {
        Layout.fillWidth:   true
        visible:            _ntrip.ntripPassword.userVisible
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
        visible:            fact.userVisible
        enabled:            !_isActive
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            !_ntrip.ntripUseTls.rawValue
                            && (_ntrip.ntripUsername.rawValue !== "" || _ntrip.ntripPassword.rawValue !== "")
        text:               qsTr("⚠ Credentials will be sent unencrypted. Enable TLS for secure connections.")
        wrapMode:           Text.WordWrap
        color:              qgcPal.colorOrange
        font.pointSize:     ScreenTools.smallFontPointSize
    }

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth
        visible:            _ntrip.ntripMountpoint.userVisible

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
                        _stc.fetchStatus !== NTRIPSourceTableController.InProgress
            onClicked:  _ntripMgr.fetchMountpoints()
        }
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _stc.fetchStatus === NTRIPSourceTableController.InProgress
        text:               qsTr("Fetching mountpoints…")
        color:              qgcPal.colorOrange
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            _stc.fetchStatus === NTRIPSourceTableController.Error
        text:               _stc.fetchError
        color:              qgcPal.colorRed
        wrapMode:           Text.WordWrap
    }

    QGCListView {
        Layout.fillWidth:       true
        Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 20)
        visible:                _stc.mountpointModel && _stc.mountpointModel.count > 0
        model:                  _stc.mountpointModel
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
                    onClicked:  _stc.selectMountpoint(object.mountpoint)
                }
            }
        }
    }
}
