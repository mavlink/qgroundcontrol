import QtQml

import QGroundControl
import QGroundControl.GPS

QtObject {
    id: root

    required property NTRIPConnectionStats stats
    required property NTRIPSourceTableController controller
    required property GPSCorrectionManager corrections

    readonly property QtObject forwarder: root.corrections.rtcmMavlink
    readonly property real totalBytesSubmitted: root.corrections.rtcmMavlink.totalBytesSubmitted
    readonly property bool canSaveBasePosition: QGroundControl.gpsManager.gpsRtk.facts.canSaveCurrentBasePosition
    readonly property int satellitesInView: QGroundControl.gpsManager.gpsRtk.facts.numSatellites.rawValue
    readonly property int satellitesUsed: QGroundControl.gpsManager.gpsRtk.facts.numSatellitesUsed.rawValue

    readonly property real bytesReceived: root.stats.bytesReceived
    readonly property real messagesReceived: root.stats.messagesReceived
    readonly property real dataRateBytesPerSec: root.stats.dataRateBytesPerSec
    readonly property real correctionAgeSec: root.stats.correctionAgeSec
    readonly property bool dataStale: root.stats.dataStale
    readonly property list<rtcmMessageCount> messageCountsById: root.stats.messageCountsById

    readonly property int fetchStatus: root.controller.fetchStatus
    readonly property string fetchError: root.controller.fetchError
    readonly property QtObject mountpointModel: root.controller.mountpointModel
    readonly property bool idle: root.controller.fetchStatus === NTRIPSourceTableController.Idle
    readonly property bool inProgress: root.controller.fetchStatus === NTRIPSourceTableController.InProgress
    readonly property bool success: root.controller.fetchStatus === NTRIPSourceTableController.Success
    readonly property bool error: root.controller.fetchStatus === NTRIPSourceTableController.Error
}
