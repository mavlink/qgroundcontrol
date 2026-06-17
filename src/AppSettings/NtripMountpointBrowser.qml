import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS.NTRIP

SettingsGroupLayout {
    id: root

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
            objectName:         "ntripMountpointField"
            Layout.fillWidth:           true
            textFieldPreferredWidth:    root._textFieldWidth
            label:              fact.shortDescription
            fact:               root._ntrip.ntripMountpoint
            enabled:            !root._isActive
        }

        QGCButton {
            objectName: "ntripBrowseButton"
            text:       qsTr("Browse")
            enabled:    !root._isActive && root._hasHost &&
                        root._ntripMgr.sourceTableController.fetchStatus !== NTRIPSourceTableController.InProgress
            onClicked:  root._ntripMgr.fetchMountpoints()
        }
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            root._ntripMgr.sourceTableController.fetchStatus === NTRIPSourceTableController.InProgress
        text:               qsTr("Fetching mountpoints…")
        color:              qgcPal.colorOrange
    }

    QGCLabel {
        Layout.fillWidth:   true
        visible:            root._ntripMgr.sourceTableController.fetchStatus === NTRIPSourceTableController.Error
        text:               root._ntripMgr.sourceTableController.fetchError
        color:              qgcPal.colorRed
        wrapMode:           Text.WordWrap
    }

    NTRIPMountpointList {
        Layout.fillWidth:       true
        visible:                root._ntripMgr.sourceTableController.mountpointModel && root._ntripMgr.sourceTableController.mountpointModel.count > 0
        model:                  root._ntripMgr.sourceTableController.mountpointModel
        selectedMountpoint:     root._ntrip.ntripMountpoint.rawValue
        onMountpointSelected:   (mountpoint) => root._ntripMgr.selectMountpoint(mountpoint)
    }
}
