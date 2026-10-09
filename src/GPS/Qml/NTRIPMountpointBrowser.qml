pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

SettingsGroupLayout {
    id: root

    readonly property NTRIPSettings _ntrip: QGroundControl.settingsManager.ntripSettings
    readonly property NTRIPManager _ntripMgr: QGroundControl.gpsManager.ntrip
    readonly property NTRIPSourceTableController _sourceTable: root._ntripMgr.sourceTableController
    readonly property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property bool _isActive: root._ntrip.ntripServerConnectEnabled.rawValue
    readonly property bool _hasHost: root._ntrip.ntripServerHostAddress.rawValue !== ""
    readonly property bool _fetching: root._sourceTable.fetchStatus === NTRIPSourceTableController.InProgress

    Layout.fillWidth:   true
    heading:            qsTr("NTRIP Mountpoint")
    visible:            root._ntrip.userVisible && (root._ntrip.ntripMountpoint as SettingsFact).userVisible

    RowLayout {
        Layout.fillWidth:   true
        spacing:            ScreenTools.defaultFontPixelWidth

        LabelledFactTextField {
            objectName:                 "ntripMountpointField"
            Layout.fillWidth:           true
            textFieldPreferredWidth:    ScreenTools.defaultFontPixelWidth * 30
            fact:                       root._ntrip.ntripMountpoint
            enabled:                    !root._isActive
        }

        QGCButton {
            objectName: "ntripBrowseButton"
            text:       qsTr("Browse")
            enabled:    !root._isActive && root._hasHost && !root._fetching
            onClicked:  root._activeVehicle ? root._ntripMgr.fetchMountpoints(root._activeVehicle.coordinate)
                                            : root._ntripMgr.fetchMountpoints()
        }
    }

    GPSNoteLabel {
        visible:            root._fetching
        text:               qsTr("Fetching mountpoints…")
    }

    GPSNoteLabel {
        objectName:         "ntripSourceTableSecurityWarning"
        visible:            text !== ""
        text:               root._sourceTable.securityWarning
        warning:            true
    }

    GPSNoteLabel {
        visible:            root._sourceTable.fetchStatus === NTRIPSourceTableController.Error
        text:               root._sourceTable.fetchError
        warning:            true
    }

    NTRIPMountpointList {
        objectName:             "ntripMountpointList"
        Layout.fillWidth:       true
        enabled:                !root._isActive
        visible:                count > 0
        model:                  root._sourceTable.mountpointModel
        selectedMountpoint:     root._ntrip.ntripMountpoint.rawValue
        onMountpointSelected:   (mountpoint) => { root._ntrip.ntripMountpoint.rawValue = mountpoint }
    }
}
