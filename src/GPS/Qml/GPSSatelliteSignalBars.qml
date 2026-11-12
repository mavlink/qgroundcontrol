pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

/// A signal bar for each satellite in view, as tall as its signal is strong, in rows that wrap. The bars of a
/// constellation follow its name and have its colour; the bar of a satellite the receiver uses is filled.
Flow {
    id: root

    property GPSSatelliteModel satellites

    /// The signal strength of a full bar, in dB-Hz.
    readonly property int _fullScale: 50
    readonly property real _barHeight: ScreenTools.defaultFontPixelHeight * 2.5
    /// Room for a three-digit number, as SBAS satellites have.
    readonly property real _barWidth: ScreenTools.smallFontPixelWidth * 4

    spacing: ScreenTools.defaultFontPixelWidth / 2

    GPSConstellationPalette { id: qgcPal }

    Repeater {
        model: root.satellites

        delegate: Row {
            id: satellite

            required property int constellation
            required property string constellationName
            required property bool startsConstellation
            required property int satelliteId
            required property int signalStrength
            required property bool used
            readonly property color tint: qgcPal.constellationColor(satellite.constellation)

            objectName: "gpsSatelliteBar"
            spacing: root.spacing

            QGCLabel {
                objectName: "gpsSatelliteBarConstellation"
                anchors.verticalCenter: parent.verticalCenter
                visible: satellite.startsConstellation
                text: satellite.constellationName
                textFormat: Text.PlainText
                color: satellite.tint
                font.pointSize: ScreenTools.smallFontPointSize
            }

            Column {
                Item {
                    width: root._barWidth
                    height: root._barHeight

                    Rectangle {
                        objectName: "gpsSatelliteBarLevel"
                        anchors.bottom: parent.bottom
                        width: parent.width
                        // A satellite without a signal keeps a line, so that its number has a bar.
                        height: Math.max(1, parent.height * Math.min(satellite.signalStrength, root._fullScale)
                                            / root._fullScale)
                        color: satellite.used ? satellite.tint : "transparent"
                        border.color: satellite.tint
                    }
                }

                QGCLabel {
                    width: root._barWidth
                    horizontalAlignment: Text.AlignHCenter
                    text: satellite.satelliteId
                    textFormat: Text.PlainText
                    color: qgcPal.textSecondary
                    font.pointSize: ScreenTools.smallFontPointSize
                }
            }
        }
    }
}
