import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

SettingsGroupLayout {
    Layout.fillWidth:   true
    heading:            qsTr("Mountpoint")
    visible:            _ntrip.ntripMountpoint.userVisible

    QGCPalette { id: qgcPal }

    property var  _ntrip:       QGroundControl.settingsManager.ntripSettings
    property var  _ntripMgr:    QGroundControl.ntripManager
    property bool _isActive:    _ntrip.ntripServerConnectEnabled.rawValue
    property bool _hasHost:     _ntrip.ntripServerHostAddress.rawValue !== ""
    property real _textFieldWidth: ScreenTools.defaultFontPixelWidth * 30

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
