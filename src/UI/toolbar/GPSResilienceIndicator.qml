/****************************************************************************
 *
 * (c) 2009-2024 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls

//-------------------------------------------------------------------------
//-- GPS Resilience Indicator
Item {
    id:             control
    width:          height
    anchors.top:    parent.top
    anchors.bottom: parent.bottom
    visible:        showIndicator

    property var    _activeVehicle: QGroundControl.multiVehicleManager.activeVehicle
    property var    _gpsAggregate:  _activeVehicle ? _activeVehicle.gpsAggregate : null

    property var    qgcPal:         QGroundControl.globalPalette

    property bool   showIndicator: _activeVehicle && _gpsAggregate && (
                                    (_gpsAggregate.authenticationState.value > 0 && _gpsAggregate.authenticationState.value < 255) ||
                                    (_gpsAggregate.spoofingState.value > 0 && _gpsAggregate.spoofingState.value < 255) ||
                                    (_gpsAggregate.jammingState.value > 0 && _gpsAggregate.jammingState.value < 255)
                                   )

    // Authentication Icon (Outer/Bottom Layer)
    QGCColoredImage {
        id:                 authIcon
        width:              parent.height * 0.95
        height:             parent.height * 0.95
        anchors.centerIn:   parent
        source:             "/qmlimages/GpsAuthentication.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        color:              _authColor()
        visible:            _gpsAggregate && _gpsAggregate.authenticationState.value > 0 && _gpsAggregate.authenticationState.value < 255
    }

    // Interference Icon (Inner/Top Layer)
    QGCColoredImage {
        id:                 interfIcon
        width:              parent.height * 0.55
        height:             parent.height * 0.55
        anchors.centerIn:   parent
        source:             "/qmlimages/GpsInterference.svg"
        fillMode:           Image.PreserveAspectFit
        sourceSize.height:  height
        color:              _interfColor()
        visible:            _gpsAggregate && (Math.max(_gpsAggregate.spoofingState.value, _gpsAggregate.jammingState.value) > 0) && (Math.max(_gpsAggregate.spoofingState.value, _gpsAggregate.jammingState.value) < 255)
    }

    function _authColor() {
        if (!_gpsAggregate) return qgcPal.colorGrey;
        switch (_gpsAggregate.authenticationState.value) {
            case 1: return qgcPal.colorYellow; // Initializing
            case 2: return qgcPal.colorRed;    // Error
            case 3: return qgcPal.colorGreen;  // OK
            default: return qgcPal.colorGrey;  // Unknown or Disabled
        }
    }

    function _interfColor() {
        if (!_gpsAggregate) return qgcPal.colorGrey;
        let maxState = Math.max(_gpsAggregate.spoofingState.value, _gpsAggregate.jammingState.value);
        switch (maxState) {
            case 1: return qgcPal.colorGreen;  // Not spoofed/jammed
            case 2: return qgcPal.colorOrange; // Mitigated
            case 3: return qgcPal.colorRed;    // Detected
            default: return qgcPal.colorGrey;  // Unknown
        }
    }

    MouseArea {
        anchors.fill:   parent
        onClicked:      mainWindow.showIndicatorDrawer(resiliencePopup, control)
    }

    Component {
        id: resiliencePopup
        ToolIndicatorPage {
            showExpand: expandedComponent ? true : false
            contentComponent: resilienceContent
        }
    }

    Component {
        id: resilienceContent
        ColumnLayout {
            spacing: ScreenTools.defaultFontPixelHeight / 2

            // Unified GPS Resilience Status
            SettingsGroupLayout {
                heading: qsTr("GPS Resilience Status")
                showDividers: true

                LabelledLabel {
                    label: qsTr("GPS Jamming")
                    labelText: _gpsAggregate ? (_gpsAggregate.jammingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                    visible: _gpsAggregate && _gpsAggregate.jammingState.value > 0 && _gpsAggregate.jammingState.value < 255
                }

                LabelledLabel {
                    label: qsTr("GPS Spoofing")
                    labelText: _gpsAggregate ? (_gpsAggregate.spoofingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                    visible: _gpsAggregate && _gpsAggregate.spoofingState.value > 0 && _gpsAggregate.spoofingState.value < 255
                }

                LabelledLabel {
                    label: qsTr("GPS Authentication")
                    labelText: _gpsAggregate ? (_gpsAggregate.authenticationState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                    visible: _gpsAggregate && _gpsAggregate.authenticationState.value > 0 && _gpsAggregate.authenticationState.value < 255
                }
            }

            // GPS 1 Details
            SettingsGroupLayout {
                heading: qsTr("GPS 1 Details")
                showDividers: true
                visible: _activeVehicle && _activeVehicle.gps && _activeVehicle.gps.lock.value > 0

                LabelledLabel {
                    label: qsTr("Jamming")
                    labelText: (_activeVehicle && _activeVehicle.gps) ? (_activeVehicle.gps.jammingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
                LabelledLabel {
                    label: qsTr("Spoofing")
                    labelText: (_activeVehicle && _activeVehicle.gps) ? (_activeVehicle.gps.spoofingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
                LabelledLabel {
                    label: qsTr("Authentication")
                    labelText: (_activeVehicle && _activeVehicle.gps) ? (_activeVehicle.gps.authenticationState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
            }

            // GPS 2 Details
            SettingsGroupLayout {
                heading: qsTr("GPS 2 Details")
                showDividers: true
                visible: _activeVehicle && _activeVehicle.gps2 && _activeVehicle.gps2.lock.value > 0

                LabelledLabel {
                    label: qsTr("Jamming")
                    labelText: (_activeVehicle && _activeVehicle.gps2) ? (_activeVehicle.gps2.jammingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
                LabelledLabel {
                    label: qsTr("Spoofing")
                    labelText: (_activeVehicle && _activeVehicle.gps2) ? (_activeVehicle.gps2.spoofingState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
                LabelledLabel {
                    label: qsTr("Authentication")
                    labelText: (_activeVehicle && _activeVehicle.gps2) ? (_activeVehicle.gps2.authenticationState.enumStringValue || qsTr("n/a")) : qsTr("n/a")
                }
            }
        }
    }
}
