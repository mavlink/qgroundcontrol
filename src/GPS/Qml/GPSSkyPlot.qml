pragma ComponentBehavior: Bound

import QtQuick

import QGroundControl
import QGroundControl.Controls

/// The satellites in view on a circle seen from above, north up: the rim is the horizon and the centre is overhead,
/// with rings at 30 and 60 degrees of elevation. A satellite has its constellation's colour and is filled when the
/// receiver uses it. One whose direction the receiver does not report is left out.
Item {
    id: root

    property GPSSatelliteModel satellites

    readonly property real _markerSize: ScreenTools.smallFontPixelHeight * 1.5
    /// The horizon's radius, which leaves room for a marker on the rim and for the north label above it.
    readonly property real _radius: Math.max(0, Math.min(root.width, root.height) / 2 - root._markerSize / 2
                                                - north.height)

    implicitWidth: ScreenTools.defaultFontPixelWidth * 34
    implicitHeight: implicitWidth

    GPSConstellationPalette { id: qgcPal }

    Repeater {
        model: [1, 2 / 3, 1 / 3]

        Rectangle {
            required property real modelData

            anchors.centerIn: parent
            width: root._radius * 2 * modelData
            height: width
            radius: width / 2
            color: "transparent"
            border.color: qgcPal.colorGrey
        }
    }

    Rectangle {
        anchors.centerIn: parent
        width: 1
        height: root._radius * 2
        color: qgcPal.colorGrey
    }

    Rectangle {
        anchors.centerIn: parent
        width: root._radius * 2
        height: 1
        color: qgcPal.colorGrey
    }

    QGCLabel {
        id: north

        anchors.horizontalCenter: parent.horizontalCenter
        y: root.height / 2 - root._radius - root._markerSize / 2 - height
        //: The direction north, at the top of a plot of the satellites in the sky
        text: qsTr("N")
        textFormat: Text.PlainText
        color: qgcPal.textSecondary
        font.pointSize: ScreenTools.smallFontPointSize
    }

    Repeater {
        model: root.satellites

        delegate: Rectangle {
            id: marker

            required property int constellation
            required property int satelliteId
            required property int elevation
            required property int azimuth
            required property bool directionKnown
            required property bool used
            readonly property color tint: qgcPal.constellationColor(marker.constellation)
            /// From the centre: a satellite below the horizon is drawn on it.
            readonly property real _distance: root._radius * (90 - Math.max(0, Math.min(90, marker.elevation))) / 90
            readonly property real _bearing: marker.azimuth * Math.PI / 180

            objectName: "gpsSkyPlotSatellite"
            x: root.width / 2 + marker._distance * Math.sin(marker._bearing) - width / 2
            y: root.height / 2 - marker._distance * Math.cos(marker._bearing) - height / 2
            width: root._markerSize
            height: width
            radius: width / 2
            visible: marker.directionKnown
            // Where satellites are close together, those in use are on top.
            z: marker.used ? 1 : 0
            color: marker.used ? marker.tint : "transparent"
            border.color: marker.tint

            QGCLabel {
                anchors.centerIn: parent
                width: parent.width - ScreenTools.smallFontPixelWidth / 2
                horizontalAlignment: Text.AlignHCenter
                text: marker.satelliteId
                textFormat: Text.PlainText
                color: marker.used ? qgcPal.window : qgcPal.textPrimary
                font.pointSize: ScreenTools.smallFontPointSize
                // A three-digit number, as SBAS satellites have, shrinks to fit the marker.
                fontSizeMode: Text.HorizontalFit
                minimumPointSize: ScreenTools.smallFontPointSize / 2
            }
        }
    }
}
