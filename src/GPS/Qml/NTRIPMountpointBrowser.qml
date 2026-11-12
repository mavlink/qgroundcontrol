pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.GPS

QGCGroupBox {
    id: root

    /// The NTRIP connection setting; the mountpoint is locked while it is on.
    property Fact enabledFact: root._ntrip.ntripServerConnectEnabled
    /// The connection whose caster is asked for its mountpoints.
    property NTRIPManager ntripManager: QGroundControl.gpsManager.ntrip
    /// Where the distances of the mountpoints are measured from while no vehicle reports its position.
    property geoCoordinate gcsPosition: QGroundControl.positionManager.gcsPosition

    readonly property NTRIPSettings _ntrip: QGroundControl.settingsManager.ntripSettings
    readonly property NTRIPSourceTableController _sourceTable: root.ntripManager.sourceTableController
    readonly property Vehicle _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    readonly property bool _isActive: root.enabledFact.rawValue
    readonly property bool _fetching: root._sourceTable.fetchStatus === NTRIPSourceTableController.InProgress

    /// The position the mountpoints are listed nearest first from: the active vehicle's or, while there is none
    /// that knows where it is, the ground station's. Without either the list keeps the caster's order.
    function _referencePosition() {
        const vehicle = root._activeVehicle
        return vehicle && vehicle.coordinate.isValid ? vehicle.coordinate : root.gcsPosition
    }

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
            enabled:    !root._isActive && root.ntripManager.canConnect && !root._fetching
            onClicked:  root.ntripManager.fetchMountpoints(root._referencePosition())
        }
    }

    QGCNoteLabel {
        visible:            root._fetching
        text:               qsTr("Fetching mountpoints…")
    }

    QGCNoteLabel {
        objectName:         "ntripSourceTableSecurityWarning"
        visible:            text !== ""
        text:               root._sourceTable.securityWarning
        warning:            true
    }

    QGCNoteLabel {
        visible:            root._sourceTable.fetchStatus === NTRIPSourceTableController.Error
        text:               root._sourceTable.fetchError
        warning:            true
    }

    NTRIPMountpointFilter {
        sourceTable:            root._sourceTable
        listedCount:            mountpointList.count
    }

    NTRIPMountpointList {
        id:                     mountpointList
        objectName:             "ntripMountpointList"
        Layout.fillWidth:       true
        enabled:                !root._isActive
        visible:                count > 0
        model:                  root._sourceTable.mountpointModel
        selectedMountpoint:     root._ntrip.ntripMountpoint.rawValue
        onMountpointSelected:   (mountpoint) => { root._ntrip.ntripMountpoint.rawValue = mountpoint }
    }
}
