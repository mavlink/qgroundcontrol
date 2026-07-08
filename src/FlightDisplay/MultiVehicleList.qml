/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FlightDisplay
import QGroundControl.Palette
import QGroundControl.ScreenTools
import QGroundControl.Vehicle

Rectangle {
    id:             vehiclePanel
    width:          Math.max(ScreenTools.defaultFontPixelWidth * 22.0, ScreenTools.minTouchPixels * 3.55)
    height:         Math.min(maxHeight > 0 ? maxHeight : _contentHeight, _contentHeight)
    implicitHeight: height
    color:          "transparent"
    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.78)
    border.color:   "transparent"
    border.width:   0
    clip:           true

    property var  _activeVehicle:      QGroundControl.multiVehicleManager.activeVehicle
    property var  _vehicles:           QGroundControl.multiVehicleManager.vehicles
    property var  _guidedController:   globals.guidedControllerFlyView
    property var  _corePluginOptions:  QGroundControl.corePlugin.options
    property var  backdropSourceItem:  null
    property real maxHeight:           0
    property bool panelExpanded:       false
    property var  panelVehicle:        null

    property int  _vehicleCount:       _vehicles ? _vehicles.count : 0
    property real _panelMargin:        ScreenTools.defaultFontPixelWidth * 0.38
    property real _rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.36
    property real _tileHeight:         Math.max(ScreenTools.minTouchPixels * 1.02, ScreenTools.defaultFontPixelHeight * 3.36)
    property real _contentHeight:      Math.max(1, (_panelMargin * 2) + (_vehicleCount * _tileHeight) + (Math.max(0, _vehicleCount - 1) * _rowSpacing))
    property bool _useChecklist:       QGroundControl.settingsManager.appSettings.useChecklist.rawValue && QGroundControl.corePlugin.options.preFlightChecklistUrl.toString().length
    property bool _enforceChecklist:   _useChecklist && QGroundControl.settingsManager.appSettings.enforceChecklist.rawValue

    signal vehicleClicked(var vehicle)

    QGCPalette { id: qgcPal }
    FlightModeDisplay { id: flightModeDisplay }

    GlassBackdrop {
        anchors.fill:       parent
        sourceItem:         vehiclePanel.backdropSourceItem
        backdropBlurEnabled:true
        targetItem:         vehiclePanel
        cornerRadius:       vehiclePanel.radius
        sourceScale:        0.46
        blurAmount:         0.94
        blurMax:            42
        sourceBrightness:   -0.01
        sourceSaturation:   0.62
        tintColor:          Qt.rgba(0.045, 0.048, 0.052, 0.68)
        sheenColor:         "transparent"
    }

    function batteryText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
            if (battery && !isNaN(battery.voltage.rawValue)) {
                return battery.voltage.valueString + " " + battery.voltage.units
            }
        }
        return qsTr("N/A")
    }

    function vehicleKindText(vehicle) {
        if (!vehicle) {
            return qsTr("Unknown")
        }
        if (vehicle.vehicleTypeString && vehicle.vehicleTypeString !== "" && vehicle.vehicleTypeString !== "MAV_TYPE_UNKNOWN") {
            return vehicle.vehicleTypeString
        }
        return qsTr("Unknown")
    }

    function vehicleDisplayName(vehicle) {
        return vehicle ? vehicleKindText(vehicle) + " #" + vehicle.id : qsTr("Unknown")
    }

    function activateVehicleFromCard(vehicle) {
        if (vehicle && _activeVehicle !== vehicle) {
            QGroundControl.multiVehicleManager.activeVehicle = vehicle
        }
        vehicleClicked(vehicle)
    }

    function guidedActionsEnabled(vehicle) {
        if (!vehicle) {
            return false
        }
        if (!ScreenTools.isDebug && _corePluginOptions.guidedActionsRequireRCRSSI) {
            return vehicle.rcRSSI > 0 && vehicle.rcRSSI <= 100
        }
        return true
    }

    function checklistPassed(vehicle) {
        if (!vehicle || !_useChecklist) {
            return true
        }
        return _enforceChecklist ? vehicle.checkListState === Vehicle.CheckListPassed : true
    }

    function canArmVehicle(vehicle) {
        if (!vehicle || !checklistPassed(vehicle)) {
            return false
        }
        return !vehicle.healthAndArmingCheckReport.supported || vehicle.healthAndArmingCheckReport.canArm
    }

    function showArmForVehicle(vehicle) {
        return guidedActionsEnabled(vehicle) && vehicle && !vehicle.armed && canArmVehicle(vehicle)
    }

    function showDisarmForVehicle(vehicle) {
        return guidedActionsEnabled(vehicle) && vehicle && vehicle.armed && !vehicle.flying
    }

    function lockActionCode(vehicle) {
        if (showArmForVehicle(vehicle)) {
            return _guidedController.actionArm
        }
        if (showDisarmForVehicle(vehicle)) {
            return _guidedController.actionDisarm
        }
        return -1
    }

    function lockActionText(vehicle) {
        if (showArmForVehicle(vehicle)) {
            return qsTr("解锁")
        }
        if (showDisarmForVehicle(vehicle)) {
            return qsTr("上锁")
        }
        return vehicle && vehicle.armed ? qsTr("已解锁") : qsTr("解锁")
    }

    function lockActionIcon(vehicle) {
        return vehicle && vehicle.armed ? "/res/LockOpen.svg" : "/res/LockClosed.svg"
    }

    function confirmVehicleAction(vehicle, actionCode) {
        if (!vehicle || actionCode < 0) {
            return
        }
        QGroundControl.multiVehicleManager.activeVehicle = vehicle
        Qt.callLater(function() {
            if (QGroundControl.multiVehicleManager.activeVehicle === vehicle) {
                _guidedController.confirmAction(actionCode)
            }
        })
    }

    component VehicleActionIconButton: Rectangle {
        id: actionButton

        property string iconSource: ""
        property string tooltipText: ""
        property bool available: true
        signal clicked()

        width:                  Math.max(22, ScreenTools.defaultFontPixelHeight * 0.92)
        height:                 width
        radius:                 Math.round(width * 0.18)
        color:                  available ? (buttonMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.44) :
                                             (buttonMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.075) : Qt.rgba(1, 1, 1, 0.040)))
                                             : Qt.rgba(1, 1, 1, 0.022)
        border.color:           available && (buttonMouse.pressed || buttonMouse.containsMouse) ? Qt.rgba(0.82, 0.90, 0.95, 0.16) : Qt.rgba(0.82, 0.90, 0.95, 0.08)
        border.width:           1
        opacity:                available ? 1.0 : 0.50

        QGCColoredImage {
            anchors.centerIn:   parent
            source:             actionButton.iconSource
            color:              actionButton.available ? qgcPal.text : qgcPal.buttonText
            width:              parent.width * 0.56
            height:             width
            sourceSize.width:   width
            sourceSize.height:  height
            fillMode:           Image.PreserveAspectFit
        }

        QGCMouseArea {
            id:             buttonMouse
            anchors.fill:   parent
            enabled:        actionButton.available
            hoverEnabled:   !ScreenTools.isMobile
            onClicked:      actionButton.clicked()
        }

        ToolTip.visible:    buttonMouse.containsMouse && actionButton.tooltipText !== ""
        ToolTip.text:       actionButton.tooltipText
        ToolTip.delay:      450
    }

    QGCFlickable {
        id:                     vehicleFlick
        anchors.fill:           parent
        anchors.margins:        _panelMargin
        contentWidth:           width
        contentHeight:          vehicleColumn.implicitHeight
        flickableDirection:     Flickable.VerticalFlick
        boundsBehavior:         Flickable.StopAtBounds
        interactive:            contentHeight > height
        clip:                   true
        indicatorColor:         qgcPal.buttonText

        Column {
            id:                 vehicleColumn
            width:              vehicleFlick.width
            spacing:            _rowSpacing

            Repeater {
                model: _vehicles

                Rectangle {
                    id:             vehicleTile
                    width:          vehicleColumn.width
                    height:         _tileHeight
                    radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.42)
                    color:          tileMouse.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.40) :
                                        (tileMouse.containsMouse ? Qt.rgba(1, 1, 1, 0.055) : "transparent")
                    border.color:   _activeVehicleTile ? qgcPal.primaryButton : Qt.rgba(0.82, 0.90, 0.95, 0.12)
                    border.width:   _activeVehicleTile ? 1 : 0

                    property var  _vehicle: object
                    property bool _activeVehicleTile: _vehicle && _activeVehicle === _vehicle
                    property bool _expandedVehicle: panelExpanded && _vehicle === panelVehicle
                    property int  _lockActionCode: lockActionCode(_vehicle)
                    property string _modeDisplayText: _vehicle ? flightModeDisplay.shortModeText(_vehicle, _vehicle.flightMode, qsTr("--")) : qsTr("--")

                    Rectangle {
                        anchors.left:       parent.left
                        anchors.top:        parent.top
                        anchors.bottom:     parent.bottom
                        width:              Math.max(2, ScreenTools.defaultFontPixelWidth * 0.20)
                        color:              qgcPal.primaryButton
                        visible:            vehicleTile._activeVehicleTile
                    }

                    QGCMouseArea {
                        id:             tileMouse
                        anchors.fill:   parent
                        hoverEnabled:   !ScreenTools.isMobile
                        onClicked:      vehiclePanel.activateVehicleFromCard(vehicleTile._vehicle)
                    }

                    RowLayout {
                        anchors.fill:       parent
                        anchors.leftMargin: ScreenTools.defaultFontPixelWidth * 0.74
                        anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 2.25
                        anchors.topMargin:  ScreenTools.defaultFontPixelHeight * 0.28
                        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.28
                        spacing:            ScreenTools.defaultFontPixelWidth * 0.58

                        Rectangle {
                            Layout.alignment:   Qt.AlignVCenter
                            width:              ScreenTools.defaultFontPixelHeight * 0.42
                            height:             width
                            radius:             width / 2
                            color:              vehicleTile._vehicle && !vehicleTile._vehicle.vehicleLinkManager.communicationLost ? qgcPal.colorGreen : qgcPal.buttonBorder
                        }

                        ColumnLayout {
                            Layout.fillWidth:   true
                            Layout.alignment:   Qt.AlignVCenter
                            spacing:            0

                            QGCLabel {
                                Layout.fillWidth:   true
                                text:               vehicleDisplayName(vehicleTile._vehicle)
                                color:              vehicleTile._activeVehicleTile ? qgcPal.text : qgcPal.buttonText
                                font.bold:          vehicleTile._activeVehicleTile
                                font.pointSize:     ScreenTools.controlFontPointSize
                                elide:              Text.ElideRight
                            }

                            RowLayout {
                                Layout.fillWidth:   true
                                spacing:            ScreenTools.defaultFontPixelWidth * 0.42

                                Item {
                                    Layout.fillWidth:   true
                                    Layout.preferredHeight: Math.max(modeLabel.implicitHeight,
                                                                     modeTypeBadge.visible ? modeTypeBadge.height : 0)

                                    RowLayout {
                                        id:                 modeInfoRow
                                        anchors.left:       parent.left
                                        anchors.verticalCenter: parent.verticalCenter
                                        spacing:            modeTypeBadge.visible ? ScreenTools.defaultFontPixelWidth * 0.28 : 0

                                        QGCLabel {
                                            id:                 modeLabel
                                            Layout.preferredWidth: Math.max(0, Math.min(implicitWidth,
                                                                                       parent.parent.width -
                                                                                       (modeTypeBadge.visible ? modeTypeBadge.width + modeInfoRow.spacing : 0)))
                                            Layout.maximumWidth: Layout.preferredWidth
                                            text:               flightModeDisplay.labelText(vehicleTile._modeDisplayText)
                                            color:              qgcPal.buttonText
                                            font.pointSize:     ScreenTools.labelFontPointSize
                                            verticalAlignment:  Text.AlignVCenter
                                            elide:              Text.ElideRight
                                            maximumLineCount:   1
                                        }

                                        Rectangle {
                                            id:                 modeTypeBadge
                                            Layout.alignment:   Qt.AlignVCenter
                                            Layout.preferredWidth: modeTypeBadgeText.implicitWidth + ScreenTools.defaultFontPixelWidth * 0.46
                                            Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 0.70,
                                                                             modeTypeBadgeText.implicitHeight + ScreenTools.defaultFontPixelHeight * 0.06)
                                            radius:             Math.round(height * 0.28)
                                            color:              Qt.rgba(0.82, 0.88, 0.94, 0.10)
                                            border.color:       Qt.rgba(0.82, 0.88, 0.94, 0.18)
                                            border.width:       1
                                            visible:            modeTypeBadgeText.text !== ""

                                            QGCLabel {
                                                id:                     modeTypeBadgeText
                                                anchors.centerIn:       parent
                                                text:                   flightModeDisplay.badgeText(vehicleTile._modeDisplayText)
                                                color:                  qgcPal.buttonText
                                                font.bold:              true
                                                font.pointSize:         Math.max(7, ScreenTools.captionFontPointSize - 1)
                                                horizontalAlignment:    Text.AlignHCenter
                                                verticalAlignment:      Text.AlignVCenter
                                                maximumLineCount:       1
                                            }
                                        }
                                    }
                                }

                                QGCLabel {
                                    text:               batteryText(vehicleTile._vehicle)
                                    color:              vehicleTile._activeVehicleTile ? qgcPal.text : qgcPal.buttonText
                                    font.bold:          vehicleTile._activeVehicleTile
                                    font.pointSize:     ScreenTools.labelFontPointSize
                                    elide:              Text.ElideRight
                                }
                            }
                        }
                    }

                    Row {
                        z:                  3
                        anchors.top:        parent.top
                        anchors.right:      parent.right
                        anchors.topMargin:  ScreenTools.defaultFontPixelWidth * 0.30
                        anchors.rightMargin:ScreenTools.defaultFontPixelWidth * 0.30
                        spacing:            ScreenTools.defaultFontPixelWidth * 0.22

                        VehicleActionIconButton {
                            iconSource:  lockActionIcon(vehicleTile._vehicle)
                            tooltipText: lockActionText(vehicleTile._vehicle)
                            available:   vehicleTile._lockActionCode >= 0
                            onClicked:   vehiclePanel.confirmVehicleAction(vehicleTile._vehicle, vehicleTile._lockActionCode)
                        }

                    }
                }
            }
        }
    }
}
