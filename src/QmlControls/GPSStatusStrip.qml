/****************************************************************************
 *
 * (c) 2025 IGCS
 *
 * Neon-styled GPS status strip shown along the bottom edge of Fly View.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Item {
    id: root
    visible: activeVehicle !== null
    implicitWidth: infoRow.implicitWidth + panelPadding * 2
    implicitHeight: infoRow.implicitHeight + panelPadding * 2

    property real panelPadding: ScreenTools.defaultFontPixelWidth * 1.5
    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property color _cyan: "#00F0FF"
    property color _panelBg: Qt.rgba(0.02, 0.10, 0.16, 0.9)
    property color _divider: Qt.rgba(0.0, 0.6, 0.8, 0.35)

    QGCPalette { id: qgcPal }

    Rectangle {
        id: background
        anchors.fill: parent
        color: _panelBg
        border.width: 1
        border.color: _cyan
        radius: 4
    }

    RowLayout {
        id: infoRow
        anchors.fill: parent
        anchors.margins: panelPadding
        spacing: ScreenTools.defaultFontPixelWidth * 2

        function valueOrNA(fact, fallback) {
            if (!fact) {
                return fallback
            }
            if (fact.enumStringValue !== undefined && fact.enumStringValue !== null) {
                if (fact.enumStringValue.length) {
                    return fact.enumStringValue
                }
            }
            if (fact.valueString !== undefined && fact.valueString !== null) {
                if (fact.valueString.length) {
                    return fact.valueString
                }
            }
            if (fact.value !== undefined && !isNaN(fact.value)) {
                return fact.value
            }
            return fallback
        }

        function factAvailable(fact) {
            return fact && fact.value !== undefined && !isNaN(fact.value)
        }

        Segment {
            title: qsTr("SPD")
            value: {
                if (!activeVehicle || !activeVehicle.groundSpeed) {
                    return "--"
                }
                const speed = activeVehicle.groundSpeed.rawValue
                return isNaN(speed) ? "--" : (speed * 3.6).toFixed(1) + qsTr(" km/h")
            }
        }

        Divider { }

        Segment {
            title: qsTr("SATS")
            value: activeVehicle ? infoRow.valueOrNA(activeVehicle.gps.count, "--") : "--"
        }

        Divider { }

        Segment {
            title: qsTr("GPS LOCK")
            value: activeVehicle ? infoRow.valueOrNA(activeVehicle.gps.lock, qsTr("None")) : qsTr("None")
        }

        Divider { }

        Segment {
            title: qsTr("HDOP")
            value: activeVehicle ? infoRow.valueOrNA(activeVehicle.gps.hdop, "--.--") : "--.--"
        }

        Divider { }

        Segment {
            title: qsTr("VDOP")
            value: activeVehicle ? infoRow.valueOrNA(activeVehicle.gps.vdop, "--.--") : "--.--"
        }

        Divider { }

        Segment {
            title: qsTr("COG")
            value: activeVehicle ? infoRow.valueOrNA(activeVehicle.gps.courseOverGround, "--.--") : "--.--"
        }

        Divider { }

        Segment {
            title: qsTr("MSL")
            value: {
                if (!activeVehicle || !activeVehicle.altitudeAMSL) {
                    return "--"
                }
                const alt = activeVehicle.altitudeAMSL.rawValue
                return isNaN(alt) ? "--" : alt.toFixed(1) + qsTr(" m")
            }
        }

        Divider { }

        Segment {
            title: qsTr("AGL")
            value: {
                if (!activeVehicle || !activeVehicle.altitudeRelative) {
                    return "--"
                }
                const alt = activeVehicle.altitudeRelative.rawValue
                return isNaN(alt) ? "--" : alt.toFixed(1) + qsTr(" m")
            }
        }

        Divider { }

        Segment {
            title: qsTr("LAT")
            value: {
                if (!activeVehicle || !activeVehicle.coordinate || !activeVehicle.coordinate.isValid) {
                    return "--"
                }
                const lat = activeVehicle.coordinate.latitude
                return isNaN(lat) ? "--" : lat.toFixed(6)
            }
        }

        Divider { }

        Segment {
            title: qsTr("LON")
            value: {
                if (!activeVehicle || !activeVehicle.coordinate || !activeVehicle.coordinate.isValid) {
                    return "--"
                }
                const lon = activeVehicle.coordinate.longitude
                return isNaN(lon) ? "--" : lon.toFixed(6)
            }
        }

        component Divider: Rectangle {
            implicitWidth: 1
            implicitHeight: infoRow.height * 0.55
            color: _divider
            anchors.verticalCenter: parent.verticalCenter
        }
    }

    component Segment: ColumnLayout {
        spacing: ScreenTools.defaultFontPixelHeight * 0.3
        Layout.alignment: Qt.AlignVCenter

        required property string title
        required property string value

        QGCLabel {
            text: title
            color: qgcPal.windowTransparentText
            font.pointSize: ScreenTools.smallFontPointSize
        }

        QGCLabel {
            text: value
            color: _cyan
            font.bold: true
            font.pointSize: ScreenTools.defaultFontPointSize
        }
    }
}

