/****************************************************************************
*
* (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
*
* QGroundControl is licensed according to the terms in the file
* COPYING.md in the root of the source code directory.
*
****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

Rectangle {
    id: root

    implicitHeight: Math.max(ScreenTools.defaultFontPixelHeight * 2.6,
                              statusRow.implicitHeight + padding * 2)
    implicitWidth: statusRow.implicitWidth + padding * 2
    radius: 0
    color: Qt.rgba(0.02, 0.1, 0.16, 0.88)
    border.color: Qt.rgba(0.0, 0.62, 0.82, 0.7)
    border.width: 1
    visible: activeVehicle !== null

    property real padding: ScreenTools.defaultFontPixelWidth * 1.4
    property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property string timeText: Qt.formatTime(new Date(), "hh:mm:ss")
    property int rssiValue: activeVehicle && activeVehicle.rcRSSI > 0 ? Math.round(activeVehicle.rcRSSI) : -1
    property var primaryBattery: activeVehicle && activeVehicle.batteries && activeVehicle.batteries.count > 0 ? activeVehicle.batteries.get(0) : null
    property bool communicationLost: activeVehicle ? activeVehicle.vehicleLinkManager.communicationLost : false
    property bool healthChecksSupported: activeVehicle ? activeVehicle.healthAndArmingCheckReport.supported : false
    property bool vehicleFlies: activeVehicle ? activeVehicle.airShip || activeVehicle.fixedWing || activeVehicle.vtol || activeVehicle.multiRotor : false
    property bool readyToFlyAvailable: activeVehicle ? activeVehicle.readyToFlyAvailable : false
    property var  flightModeIndicator: null
    property var  statusIndicator: null

    Timer {
        interval: 1000
        repeat: true
        running: true
        onTriggered: root.timeText = Qt.formatTime(new Date(), "hh:mm:ss")
    }

    QGCPalette { id: qgcPal }

    function batteryPercentValue() {
        if (!primaryBattery || !primaryBattery.percentRemaining) {
            return NaN
        }
        return primaryBattery.percentRemaining.rawValue
    }

    function batteryText() {
        if (!primaryBattery) {
            return qsTr("--")
        }
        if (primaryBattery.percentRemaining && !isNaN(primaryBattery.percentRemaining.rawValue)) {
            return primaryBattery.percentRemaining.valueString + primaryBattery.percentRemaining.units
        }
        if (primaryBattery.voltage && !isNaN(primaryBattery.voltage.rawValue)) {
            return primaryBattery.voltage.valueString + primaryBattery.voltage.units
        }
        if (primaryBattery.chargeState && primaryBattery.chargeState.enumStringValue) {
            return primaryBattery.chargeState.enumStringValue
        }
        return qsTr("--")
    }

    function batteryIconSource() {
        var percent = batteryPercentValue()
        if (!isNaN(percent)) {
            if (percent >= 80) {
                return "/qmlimages/BatteryGreen.svg"
            }
            if (percent >= 50) {
                return "/qmlimages/BatteryYellowGreen.svg"
            }
            if (percent >= 25) {
                return "/qmlimages/BatteryYellow.svg"
            }
            return "/qmlimages/BatteryOrange.svg"
        }

        if (primaryBattery && primaryBattery.chargeState && primaryBattery.chargeState.rawValue >= 0) {
            return "/qmlimages/BatteryCritical.svg"
        }

        return "/qmlimages/Battery.svg"
    }

    function batteryIconColor() {
        var percent = batteryPercentValue()
        if (!isNaN(percent)) {
            if (percent >= 80) {
                return qgcPal.colorGreen
            }
            if (percent >= 50) {
                return qgcPal.colorYellowGreen
            }
            if (percent >= 25) {
                return qgcPal.colorYellow
            }
            return qgcPal.colorOrange
        }

        if (primaryBattery && primaryBattery.chargeState && primaryBattery.chargeState.rawValue >= 0) {
            return qgcPal.colorRed
        }

        return qgcPal.windowTransparentText
    }

    function armedLabel() {
        if (!activeVehicle) {
            return qsTr("ARM: --")
        }
        return activeVehicle.armed ? qsTr("ARM: ARMED") : qsTr("ARM: SAFE")
    }

    function armedColor() {
        if (!activeVehicle) {
            return qgcPal.windowTransparentText
        }
        return activeVehicle.armed ? qgcPal.colorGreen : qgcPal.colorOrange
    }

    function readyLabel() {
        return vehicleFlies ? qsTr("Ready To Fly") : qsTr("Ready")
    }

    function flightModeText() {
        return activeVehicle && activeVehicle.flightMode ? activeVehicle.flightMode : qsTr("N/A")
    }

    function statusText() {
        if (!activeVehicle) {
            return qsTr("Disconnected")
        }
        if (communicationLost) {
            return qsTr("Comms Lost")
        }
        if (activeVehicle.armed) {
            if (activeVehicle.flying) {
                return qsTr("Flying")
            } else if (activeVehicle.landing) {
                return qsTr("Landing")
            }
            return qsTr("Armed")
        }

        if (healthChecksSupported) {
            if (activeVehicle.healthAndArmingCheckReport.canArm) {
                return readyLabel()
            }
            return qsTr("Not Ready")
        } else if (readyToFlyAvailable) {
            return activeVehicle.readyToFly ? readyLabel() : qsTr("Not Ready")
        } else {
            if (activeVehicle.allSensorsHealthy && activeVehicle.autopilotPlugin.setupComplete) {
                return readyLabel()
            }
            return qsTr("Not Ready")
        }
    }

    function statusColor() {
        if (!activeVehicle) {
            return qgcPal.windowTransparentText
        }
        if (communicationLost) {
            return qgcPal.colorRed
        }
        if (activeVehicle.armed) {
            if (healthChecksSupported) {
                if (!activeVehicle.healthAndArmingCheckReport.canArm) {
                    return qgcPal.colorRed
                }
                if (activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors) {
                    return qgcPal.colorYellow
                }
            }
            return qgcPal.colorGreen
        }

        if (healthChecksSupported) {
            if (activeVehicle.healthAndArmingCheckReport.canArm) {
                return activeVehicle.healthAndArmingCheckReport.hasWarningsOrErrors ? "#FFC400" : qgcPal.colorGreen
            }
            return qgcPal.colorRed
        } else if (readyToFlyAvailable) {
            return activeVehicle.readyToFly ? qgcPal.colorGreen : "#FFC400"
        } else {
            return (activeVehicle.allSensorsHealthy && activeVehicle.autopilotPlugin.setupComplete) ? qgcPal.colorGreen : "#FFC400"
        }
    }

    function statusDisplayText() {
        return statusText().toUpperCase()
    }

    RowLayout {
        id: statusRow
        anchors.centerIn: parent
        spacing: ScreenTools.defaultFontPixelWidth * 1.2

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 0.6
            Layout.alignment: Qt.AlignVCenter

            Rectangle {
                width: ScreenTools.defaultFontPixelWidth * 0.6
                height: width
                radius: width / 2
                color: Qt.rgba(0.0, 0.75, 0.95, 1.0)
            }

            QGCLabel {
                text: root.timeText
                color: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                font.pointSize: ScreenTools.defaultFontPointSize
                font.bold: true
            }
        }

        Rectangle {
            width: 1
            height: statusRow.height * 0.6
            color: Qt.rgba(0.0, 0.62, 0.82, 0.5)
            Layout.alignment: Qt.AlignVCenter
        }

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 0.4
            Layout.alignment: Qt.AlignVCenter

            SignalStrength {
                size: ScreenTools.defaultFontPixelHeight
                percent: root.rssiValue >= 0 ? root.rssiValue : 0
                iconColor: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
            }

            QGCLabel {
                text: (root.rssiValue >= 0 ? root.rssiValue + "%" : qsTr("--")).toUpperCase()
                color: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                font.pointSize: ScreenTools.defaultFontPointSize
                font.bold: true
            }
        }

        Rectangle {
            width: 1
            height: statusRow.height * 0.6
            color: Qt.rgba(0.0, 0.62, 0.82, 0.5)
            Layout.alignment: Qt.AlignVCenter
        }

        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth * 0.4
            Layout.alignment: Qt.AlignVCenter

            QGCColoredImage {
                source: batteryIconSource()
                height: ScreenTools.defaultFontPixelHeight
                width: height
                fillMode: Image.PreserveAspectFit
                color: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                visible: source !== ""
            }

            QGCLabel {
                text: batteryText().toUpperCase()
                color: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                font.pointSize: ScreenTools.defaultFontPointSize
                font.bold: true
            }
        }

        Rectangle {
            width: 1
            height: statusRow.height * 0.6
            color: Qt.rgba(0.0, 0.62, 0.82, 0.5)
            Layout.alignment: Qt.AlignVCenter
        }

        QGCLabel {
            text: armedLabel()
            color: armedColor()
            font.bold: true
            font.pointSize: ScreenTools.defaultFontPointSize
            Layout.alignment: Qt.AlignVCenter
        }

        Rectangle {
            width: 1
            height: statusRow.height * 0.6
            color: Qt.rgba(0.0, 0.62, 0.82, 0.5)
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth:  flightModeTextLabel.implicitWidth
            implicitHeight: flightModeTextLabel.implicitHeight

            QGCLabel {
                id: flightModeTextLabel
                anchors.centerIn: parent
                text: flightModeText().toUpperCase()
                color: qgcPal.globalTheme === QGCPalette.Light ? qgcPal.text : qgcPal.brandingBlue
                font.pointSize: ScreenTools.defaultFontPointSize
                font.bold: true
            }

            QGCMouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (flightModeIndicator && flightModeIndicator.openIndicator) {
                        flightModeIndicator.openIndicator()
                    }
                }
            }
        }

        Rectangle {
            width: 1
            height: statusRow.height * 0.6
            color: Qt.rgba(0.0, 0.62, 0.82, 0.5)
            Layout.alignment: Qt.AlignVCenter
        }

        Item {
            Layout.alignment: Qt.AlignVCenter
            implicitWidth: statusRowItem.implicitWidth
            implicitHeight: statusRowItem.implicitHeight

            RowLayout {
                id: statusRowItem
                spacing: ScreenTools.defaultFontPixelWidth

                Rectangle {
                    width: ScreenTools.defaultFontPixelWidth * 0.6
                    height: width
                    radius: width / 2
                    color: statusColor()
                }

                QGCLabel {
                    text: statusDisplayText()
                    color: statusColor()
                    font.pointSize: ScreenTools.defaultFontPointSize
                    font.bold: true
                }
            }

            QGCMouseArea {
                anchors.fill: parent
                cursorShape: Qt.PointingHandCursor
                onClicked: {
                    if (statusIndicator && statusIndicator.dropMainStatusIndicator) {
                        statusIndicator.dropMainStatusIndicator()
                    }
                }
            }
        }
    }
}

