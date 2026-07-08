import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.ScreenTools
import QGroundControl.Controls
import QGroundControl.FactControls
import QGroundControl.Palette
import QGroundControl.UTMSP

// Toolbar for Plan View
Item {
    width: missionStats.implicitWidth + _margins

    property var    planMasterController

    property var    _planMasterController:      planMasterController
    property var    _currentMissionItem:        _planMasterController.missionController.currentPlanViewItem ///< Mission item to display status for

    property var    missionItems:               _controllerValid ? _planMasterController.missionController.visualItems : undefined
    property real   missionPlannedDistance:     _controllerValid ? _planMasterController.missionController.missionPlannedDistance : NaN
    property real   missionTime:                _controllerValid ? _planMasterController.missionController.missionTime : 0
    property real   missionMaxTelemetry:        _controllerValid ? _planMasterController.missionController.missionMaxTelemetry : NaN
    property bool   missionDirty:               _controllerValid ? _planMasterController.missionController.dirty : false

    property bool   _controllerValid:           _planMasterController !== undefined && _planMasterController !== null
    property bool   _controllerOffline:         _controllerValid ? _planMasterController.offline : true
    property var    _controllerDirty:           _controllerValid ? _planMasterController.dirty : false
    property var    _controllerSyncInProgress:  _controllerValid ? _planMasterController.syncInProgress : false

    property bool   _currentMissionItemValid:   _currentMissionItem && _currentMissionItem !== undefined && _currentMissionItem !== null
    property bool   _curreItemIsFlyThrough:     _currentMissionItemValid && _currentMissionItem.specifiesCoordinate && !_currentMissionItem.isStandaloneCoordinate
    property bool   _currentItemIsVTOLTakeoff:  _currentMissionItemValid && _currentMissionItem.command == 84
    property bool   _missionValid:              missionItems !== undefined

    property real   _labelToValueSpacing:       ScreenTools.defaultFontPixelWidth * 0.70
    property real   _segmentPadding:            ScreenTools.defaultFontPixelWidth * 0.80
    property real   _distance:                  _currentMissionItemValid ? _currentMissionItem.distance : NaN
    property real   _altDifference:             _currentMissionItemValid ? _currentMissionItem.altDifference : NaN
    property real   _azimuth:                   _currentMissionItemValid ? _currentMissionItem.azimuth : NaN
    property real   _heading:                   _currentMissionItemValid ? _currentMissionItem.missionVehicleYaw : NaN
    property real   _missionPlannedDistance:    _missionValid ? missionPlannedDistance : NaN
    property real   _missionMaxTelemetry:       _missionValid ? missionMaxTelemetry : NaN
    property real   _missionTime:               _missionValid ? missionTime : 0
    property int    _batteryChangePoint:        _controllerValid ? _planMasterController.missionController.batteryChangePoint : -1
    property int    _batteriesRequired:         _controllerValid ? _planMasterController.missionController.batteriesRequired : -1
    property bool   _batteryInfoAvailable:      _batteryChangePoint >= 0 || _batteriesRequired >= 0
    property real   _gradient:                  _currentMissionItemValid && _currentMissionItem.distance > 0 ?
                                                    (_currentItemIsVTOLTakeoff ?
                                                         0 :
                                                         (Math.atan(_currentMissionItem.altDifference / _currentMissionItem.distance) * (180.0/Math.PI)))
                                                  : NaN

    property string _distanceText:                  isNaN(_distance) ?                  "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_distance).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _altDifferenceText:             isNaN(_altDifference) ?             "-.-" : QGroundControl.unitsConversion.metersToAppSettingsVerticalDistanceUnits(_altDifference).toFixed(1) + " " + QGroundControl.unitsConversion.appSettingsVerticalDistanceUnitsString
    property string _gradientText:                  isNaN(_gradient) ?                  "-.-" : _gradient.toFixed(0) + qsTr(" deg")
    property string _azimuthText:                   isNaN(_azimuth) ?                   "-.-" : Math.round(_azimuth) % 360
    property string _headingText:                   isNaN(_heading) ?                   "-.-" : Math.round(_heading) % 360
    property string _missionPlannedDistanceText:    isNaN(_missionPlannedDistance) ?    "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionPlannedDistance).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _missionMaxTelemetryText:       isNaN(_missionMaxTelemetry) ?       "-.-" : QGroundControl.unitsConversion.metersToAppSettingsHorizontalDistanceUnits(_missionMaxTelemetry).toFixed(0) + " " + QGroundControl.unitsConversion.appSettingsHorizontalDistanceUnitsString
    property string _batteryChangePointText:        _batteryChangePoint < 0 ?           qsTr("N/A") : _batteryChangePoint
    property string _batteriesRequiredText:         _batteriesRequired < 0 ?            qsTr("N/A") : _batteriesRequired
    property string _selectedWaypointText:          _currentMissionItemValid && _currentMissionItem.sequenceNumber !== undefined ? _currentMissionItem.sequenceNumber : "--"

    readonly property real _margins: ScreenTools.defaultFontPixelWidth

    // Properties of UTM adapter
    property bool   _utmspEnabled:                       QGroundControl.utmspSupported

    QGCPalette { id: qgcPal }

    function getMissionTime() {
        if (!_missionTime) {
            return "00:00:00"
        }
        var t = new Date(2021, 0, 0, 0, 0, Number(_missionTime))
        var days = Qt.formatDateTime(t, 'dd')
        var complete

        if (days == 31) {
            days = '0'
            complete = Qt.formatTime(t, 'hh:mm:ss')
        } else {
            complete = days + " days " + Qt.formatTime(t, 'hh:mm:ss')
        }
        return complete
    }

    component BarDivider: Rectangle {
        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  1
        Layout.preferredHeight: ScreenTools.defaultFontPixelHeight * 2.1
        color:                  Qt.rgba(0.82, 0.90, 0.95, 0.09)
    }

    component MetricSegment: Item {
        id: metricSegment

        property string label: ""
        property string value: ""
        property real   minimumWidth: ScreenTools.defaultFontPixelWidth * 7.8

        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  Math.max(minimumWidth, valueColumn.implicitWidth + _segmentPadding)
        Layout.fillHeight:      true

        function displayLabel() {
            var text = metricSegment.label
            while (text.length > 0 && text.charAt(text.length - 1) === " ") {
                text = text.substring(0, text.length - 1)
            }
            if (text.length > 0 && (text.charAt(text.length - 1) === ":" || text.charAt(text.length - 1) === "：")) {
                return text.substring(0, text.length - 1)
            }
            return text
        }

        ColumnLayout {
            id:                     valueColumn
            anchors.left:           parent.left
            anchors.right:          parent.right
            anchors.verticalCenter: parent.verticalCenter
            spacing:                -ScreenTools.defaultFontPixelHeight * 0.08

            QGCLabel {
                Layout.fillWidth:   true
                text:               metricSegment.displayLabel()
                color:              qgcPal.buttonText
                font.pointSize:     ScreenTools.labelFontPointSize
                elide:              Text.ElideRight
            }

            QGCLabel {
                Layout.fillWidth:   true
                text:               metricSegment.value
                color:              qgcPal.text
                font.bold:          true
                font.pointSize:     ScreenTools.controlFontPointSize
                elide:              Text.ElideRight
            }
        }
    }

    RowLayout {
        id:                     missionStats
        anchors.top:            parent.top
        anchors.bottom:         parent.bottom
        anchors.leftMargin:     _margins
        anchors.left:           parent.left
        spacing:                ScreenTools.defaultFontPixelWidth * 0.72

        Button {
            id:          uploadButton
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredWidth:  Math.max(uploadText.implicitWidth + ScreenTools.defaultFontPixelWidth * 3.0,
                                             ScreenTools.defaultFontPixelWidth * 6.4)
            Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 1.60,
                                             missionStats.height * 0.54)
            padding:      0
            topInset:     0
            bottomInset:  0
            leftInset:    0
            rightInset:   0
            hoverEnabled: !ScreenTools.isMobile
            text:        _controllerDirty ? qsTr("Upload Required") : qsTr("Upload")
            enabled:     _utmspEnabled ? !_controllerSyncInProgress && UTMSPStateStorage.enableMissionUploadButton : !_controllerSyncInProgress
            visible:     !_controllerOffline && !_controllerSyncInProgress
            onClicked: {
                if (_utmspEnabled) {
                    QGroundControl.utmspManager.utmspVehicle.triggerActivationStatusBar(true);
                    UTMSPStateStorage.removeFlightPlanState = true
                    UTMSPStateStorage.indicatorDisplayStatus = true
                }
                _planMasterController.upload();
            }

            background: Rectangle {
                radius:         Math.round(ScreenTools.defaultFontPixelWidth * 0.42)
                color:          _controllerDirty ? qgcPal.primaryButton :
                                    (uploadButton.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.48) :
                                     uploadButton.hovered ? Qt.rgba(1, 1, 1, 0.055) : Qt.rgba(1, 1, 1, 0.020))
                border.color:   _controllerDirty ? qgcPal.primaryButton : Qt.rgba(0.82, 0.90, 0.95, 0.22)
                border.width:   1
                opacity:        uploadButton.enabled ? 1.0 : 0.45
            }

            contentItem: Item {
                QGCLabel {
                    id:                     uploadText
                    anchors.centerIn:       parent
                    text:                   uploadButton.text
                    color:                  _controllerDirty ? qgcPal.primaryButtonText : qgcPal.text
                    font.pointSize:         ScreenTools.controlFontPointSize
                    horizontalAlignment:    Text.AlignHCenter
                    verticalAlignment:      Text.AlignVCenter
                }
            }

            PropertyAnimation on opacity {
                easing.type:    Easing.OutQuart
                from:           0.5
                to:             1
                loops:          Animation.Infinite
                running:        _controllerDirty && !_controllerSyncInProgress
                alwaysRunToEnd: true
                duration:       2000
            }
        }

        BarDivider { }

        MetricSegment { label: qsTr("Selected Waypoint"); value: _selectedWaypointText; minimumWidth: ScreenTools.defaultFontPixelWidth * 8.6 }
        BarDivider { }
        MetricSegment { label: qsTr("Alt diff:");         value: _altDifferenceText;    minimumWidth: ScreenTools.defaultFontPixelWidth * 7.4 }
        BarDivider { }
        MetricSegment { label: qsTr("Azimuth:");          value: _azimuthText;          minimumWidth: ScreenTools.defaultFontPixelWidth * 6.3 }
        BarDivider { }
        MetricSegment { label: qsTr("Dist prev WP:");     value: _distanceText;         minimumWidth: ScreenTools.defaultFontPixelWidth * 8.8 }
        BarDivider { }
        MetricSegment { label: qsTr("Gradient:");         value: _gradientText;         minimumWidth: ScreenTools.defaultFontPixelWidth * 6.9 }
        BarDivider { }
        MetricSegment { label: qsTr("Heading:");          value: _headingText;          minimumWidth: ScreenTools.defaultFontPixelWidth * 6.3 }
        BarDivider { }
        MetricSegment { label: qsTr("Distance:");         value: _missionPlannedDistanceText; minimumWidth: ScreenTools.defaultFontPixelWidth * 7.6 }
        BarDivider { }
        MetricSegment { label: qsTr("Max telem dist:");   value: _missionMaxTelemetryText;    minimumWidth: ScreenTools.defaultFontPixelWidth * 10.0 }
        BarDivider { }
        MetricSegment { label: qsTr("Time:");             value: getMissionTime();            minimumWidth: ScreenTools.defaultFontPixelWidth * 8.0 }
        BarDivider { visible: _batteryInfoAvailable }

        MetricSegment {
            label:        qsTr("Batteries required:")
            value:        _batteriesRequiredText
            minimumWidth: ScreenTools.defaultFontPixelWidth * 9.8
            visible:      _batteryInfoAvailable
        }
    }
}

