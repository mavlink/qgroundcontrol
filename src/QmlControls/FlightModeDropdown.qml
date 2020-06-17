/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick 2.3

import QGroundControl.Controls      1.0
import QGroundControl.ScreenTools   1.0

Item {
    width:  flightModeLabel.visible ? flightModeLabel.width : flightModeCombo.width
    height: flightModeLabel.visible ? flightModeLabel.height : flightModeCombo.height

    property var activeVehicle  ///< Vehicle to show flight modes for

    property int _maxFMCharLength:  10   ///< Maximum number of chars in a flight mode
    property string flightMode:     activeVehicle ? activeVehicle.flightMode : qsTr("N/A", "No data to display")

    onActiveVehicleChanged: _activeVehicleChanged()

    onFlightModeChanged: {
        if (flightModeCombo.visible) {
            flightModeCombo.currentIndex = flightModeCombo.find(flightMode)
        }
    }

    Component.onCompleted: _activeVehicleChanged()

    function _activeVehicleChanged() {
        if (activeVehicle.flightModeSetAvailable) {
            var maxFMChars = 0
            for (var i=0; i<activeVehicle.flightModes.length; i++) {
                maxFMChars = Math.max(maxFMChars, activeVehicle.flightModes[i].length)
            }
            _maxFMCharLength = maxFMChars
        }
    }

    QGCLabel {
        id:         flightModeLabel
        text:       flightMode
        visible:    !activeVehicle.flightModeSetAvailable
        anchors.verticalCenter: parent.verticalCenter
    }

    QGCComboBox {
        id:         flightModeCombo
        width:      (_maxFMCharLength + 4) * ScreenTools.defaultFontPixelWidth
        model:      activeVehicle ? activeVehicle.flightModes : 0
        visible:    activeVehicle.flightModeSetAvailable

        onModelChanged: {
            if (activeVehicle && visible) {
                currentIndex = find(flightMode)
            }
        }

        onActivated: activeVehicle.flightMode = textAt(index)
    }
}
