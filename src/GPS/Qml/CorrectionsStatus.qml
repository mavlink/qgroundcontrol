pragma ComponentBehavior: Bound

import QtQuick
import QtQuick.Layouts
import QtPositioning

import QGroundControl
import QGroundControl.Controls
import QGroundControl.GPS

/// The correction stream vehicles receive, shared by the GPS indicator and the RTK Corrections settings page.
QGCGroupBox {
    id: root

    property GPSCorrectionManager corrections: QGroundControl.gpsManager.corrections
    /// Keep the group visible while no correction source is configured.
    property bool showWhenInactive: true
    /// Show the base station's coordinates; its distance from the ground station shows either way.
    property bool showCoordinates: true
    /// Where the distance to the base station is measured from.
    property geoCoordinate gcsPosition: QGroundControl.positionManager.gcsPosition

    readonly property gpsCorrectionStream _selected: root.corrections.selectedStream
    readonly property bool _active: root.corrections.state !== GPSCorrectionManager.Inactive
    readonly property bool _fresh: root.corrections.state === GPSCorrectionManager.Fresh
    readonly property geoCoordinate _basePosition: root.corrections.basePosition
    readonly property bool _baseKnown: root._fresh && root._basePosition.isValid
    readonly property string _udpInputError: root.corrections.udpInputError
    readonly property string _udpOutputError: root.corrections.udpOutputError

    heading: qsTr("Corrections Status")
    visible: root.showWhenInactive || root._active || root._udpInputError !== "" || root._udpOutputError !== ""

    QGCPalette { id: qgcPal }

    LabelledLabel {
        objectName: "correctionsSelectedSource"
        label: qsTr("Source")
        labelText: root._fresh ? root.corrections.sourceName(root._selected.source)
                   //: No fresh correction stream is available yet
                   : root._active ? qsTr("Waiting") : qsTr("None")
        labelTextFormat: Text.PlainText
    }

    // Stream endpoints can be long URLs, so they wrap anywhere.
    GPSNoteLabel {
        objectName: "correctionsSelectedStream"
        visible: root._fresh && root._selected.instanceId.length > 0
        text: root._selected.instanceId
        wrapMode: Text.WrapAnywhere
    }

    LabelledLabel {
        objectName: "correctionsDataRate"
        visible: root._fresh
        label: qsTr("Data rate")
        //: %1 is a data size, such as 2.1KB; the label is a data rate per second
        labelText: qsTr("%1/s").arg(QGroundControl.bigSizeToString(root.corrections.selectedBytesPerSecond))
        labelTextFormat: Text.PlainText
    }

    ColumnLayout {
        objectName: "correctionsMessageTypes"
        Layout.fillWidth: true
        spacing: ScreenTools.defaultFontPixelHeight / 4
        visible: root._fresh && messageChips.count > 0

        QGCLabel {
            text: qsTr("Message Types")
            font.pointSize: ScreenTools.smallFontPointSize
            color: qgcPal.colorGrey
        }

        RTCMMessageChips {
            id: messageChips

            Layout.fillWidth: true
            Layout.minimumWidth: 0
            Layout.preferredWidth: 0
            messageCounts: root.corrections.messageCounts
        }
    }

    LabelledLabel {
        objectName: "correctionsBasePosition"
        visible: root.showCoordinates && root._baseKnown
        label: qsTr("Base position")
        //: %1 is a latitude and %2 a longitude, in degrees
        labelText: qsTr("%1, %2").arg(root._basePosition.latitude.toFixed(7))
                                 .arg(root._basePosition.longitude.toFixed(7))
        labelTextFormat: Text.PlainText
    }

    LabelledLabel {
        objectName: "correctionsBaseDistance"
        visible: root._baseKnown && root.gcsPosition.isValid
        label: qsTr("Base distance from GCS")
        labelText: QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnitsString(
                       root._basePosition.distanceTo(root.gcsPosition), 0)
        labelTextFormat: Text.PlainText
    }

    // Why the UDP RTCM input cannot listen, such as a port another program uses; the input keeps retrying.
    GPSNoteLabel {
        objectName: "correctionsUdpInputError"
        visible: root._udpInputError !== ""
        text: root._udpInputError
        warning: true
    }

    // Why the enabled UDP output forwards nothing, such as an invalid address or a loop into the UDP input.
    GPSNoteLabel {
        objectName: "correctionsUdpOutputError"
        visible: root._udpOutputError !== ""
        text: root._udpOutputError
        warning: true
    }
}
