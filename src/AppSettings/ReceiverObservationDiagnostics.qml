import QGroundControl
import QGroundControl.Controls
import QtQuick
import QtQuick.Layouts

SettingsGroupLayout {
    id: root

    readonly property var _satellites: sourceSelector.currentIndex === 1 ? root.nmeaSatelliteModel : root.satelliteModel
    property var integrity: QGroundControl.gpsReceiver.integrity
    property var nmeaSatelliteModel: QGroundControl.gpsManager.nmeaSatelliteModel
    property var relativePosition: QGroundControl.gpsManager.relativePositionModel
    property var satelliteModel: QGroundControl.gpsManager.satelliteModel

    function formatValue(value, digits) {
        return typeof value === "number" && Number.isFinite(value) ? value.toFixed(digits) : qsTr("Unknown");
    }

    heading: qsTr("Receiver Observations")

    LabelledComboBox {
        id: sourceSelector

        Layout.fillWidth: true
        currentIndex: 0
        label: qsTr("Satellite source")
        model: [qsTr("Local receiver"), qsTr("NMEA")]
        objectName: "gpsSatelliteSource"
    }

    QGCLabel {
        Layout.fillWidth: true
        Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
        objectName: "gpsSatelliteSummary"
        text: root._satellites && root._satellites.fresh ? qsTr("%1 satellites — %2").arg(root._satellites.count).arg(root._satellites.sourceId) : qsTr("No fresh satellite report")
        wrapMode: Text.WordWrap
    }

    QGCCheckBox {
        id: showDetails

        objectName: "gpsObservationDetailsToggle"
        text: qsTr("Show satellite, relative-position, and integrity details")
    }

    Loader {
        Layout.fillWidth: true
        active: showDetails.checked
        visible: active

        sourceComponent: ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            ListView {
                id: satelliteList

                Layout.fillWidth: true
                Layout.preferredHeight: Math.min(contentHeight, ScreenTools.defaultFontPixelHeight * 12)
                clip: true
                model: root._satellites
                objectName: "gpsSatelliteDetails"
                reuseItems: true
                spacing: ScreenTools.defaultFontPixelHeight / 2

                delegate: ColumnLayout {
                    id: satellite

                    required property var azimuth
                    required property string constellation
                    required property var elevation
                    required property int satelliteId
                    required property var signalStrength
                    required property var used

                    spacing: 0
                    width: satelliteList.width

                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("%1 %2 — signal %3, used: %4").arg(satellite.constellation).arg(satellite.satelliteId).arg(root.formatValue(satellite.signalStrength, 0)).arg(typeof satellite.used !== "boolean" ? qsTr("Unknown") : satellite.used ? qsTr("Yes") : qsTr("No"))
                        wrapMode: Text.WordWrap
                    }

                    QGCLabel {
                        Layout.fillWidth: true
                        text: qsTr("Elevation: %1°   Azimuth: %2°").arg(root.formatValue(satellite.elevation, 0)).arg(root.formatValue(satellite.azimuth, 0))
                        wrapMode: Text.WordWrap
                    }
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                objectName: "gpsIntegritySummary"
                text: root.integrity && root.integrity.available ? qsTr("Local receiver integrity") : qsTr("Local receiver integrity: no fresh report")
                wrapMode: Text.WordWrap
            }

            Repeater {
                model: ["jammingState", "spoofingState", "authenticationState", "correctionsProtocol", "correctionsUsed"]

                QGCLabel {
                    readonly property var fact: root.integrity ? root.integrity[modelData] : null
                    required property string modelData

                    Layout.fillWidth: true
                    Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                    objectName: "gpsIntegrity_" + modelData
                    text: fact ? qsTr("%1: %2").arg(fact.shortDescription).arg(root.integrity.available ? fact.enumStringValue : qsTr("Unknown")) : ""
                    visible: root.integrity ? root.integrity.available : false
                    wrapMode: Text.WordWrap
                }
            }

            QGCLabel {
                Layout.fillWidth: true
                objectName: "gpsRelativeSummary"
                text: root.relativePosition && root.relativePosition.fresh ? qsTr("Local receiver relative position — %1").arg(root.relativePosition.sourceId) : qsTr("Local receiver relative position: no fresh report")
                wrapMode: Text.WordWrap
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                objectName: "gpsRelativeDetails"
                text: !root.relativePosition || !root.relativePosition.fresh ? "" : qsTr("North: %1 m   East: %2 m   Down: %3 m\nAccuracy N/E/D: %4 / %5 / %6 m\nLength: %7 m ± %8 m\nHeading: %9° ± %10°\nReference station: %11").arg(root.formatValue(root.relativePosition.north, 3)).arg(root.formatValue(root.relativePosition.east, 3)).arg(root.formatValue(root.relativePosition.down, 3)).arg(root.formatValue(root.relativePosition.northAccuracy, 3)).arg(root.formatValue(root.relativePosition.eastAccuracy, 3)).arg(root.formatValue(root.relativePosition.downAccuracy, 3)).arg(root.formatValue(root.relativePosition.length, 3)).arg(root.formatValue(root.relativePosition.lengthAccuracy, 3)).arg(root.formatValue(root.relativePosition.heading, 2)).arg(root.formatValue(root.relativePosition.headingAccuracy, 2)).arg(root.relativePosition.referenceStationId >= 0 ? root.relativePosition.referenceStationId : qsTr("Unknown"))
                visible: text.length > 0
                wrapMode: Text.WordWrap
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                text: !root.relativePosition || !root.relativePosition.fresh ? "" : qsTr("GNSS fix: %1   Differential: %2\nMoving base: %3   Normalized vector: %4").arg(root.relativePosition.fixValid ? qsTr("Yes") : qsTr("No")).arg(root.relativePosition.differential ? qsTr("Yes") : qsTr("No")).arg(root.relativePosition.movingBase ? qsTr("Yes") : qsTr("No")).arg(root.relativePosition.normalized ? qsTr("Yes") : qsTr("No"))
                visible: text.length > 0
                wrapMode: Text.WordWrap
            }

            QGCLabel {
                Layout.fillWidth: true
                Layout.preferredWidth: ScreenTools.defaultFontPixelWidth * 50
                text: !root.relativePosition || !root.relativePosition.fresh ? "" : root.relativePosition.referencePositionMissing || root.relativePosition.referenceObservationsMissing ? qsTr("Reference position or observations are missing.") : !root.relativePosition.positionValid ? qsTr("Relative position is not valid.") : root.relativePosition.carrierFixed ? qsTr("Carrier solution: fixed") : root.relativePosition.carrierFloat ? qsTr("Carrier solution: float") : qsTr("Carrier solution: unknown")
                visible: text.length > 0
                wrapMode: Text.WordWrap
            }
        }
    }
}
