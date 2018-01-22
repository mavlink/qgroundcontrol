import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Airmap        1.0

Item {
    height: _activeVehicle && _activeVehicle.airspaceController.hasWeather ? weatherRow.height : 0
    width:  _activeVehicle && _activeVehicle.airspaceController.hasWeather ? weatherRow.width  : 0
    property var    iconHeight:         ScreenTools.defaultFontPixelWidth * 4
    property color  _colorWhite:        "#ffffff"
    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    Row {
        id:                     weatherRow
        spacing:                ScreenTools.defaultFontPixelHeight * 0.5
        QGCColoredImage {
            width:                  height
            height:                 iconHeight
            sourceSize.height:      height
            source:                 _activeVehicle ? _activeVehicle.airspaceController.weatherIcon : ""
            color:                  _colorWhite
            visible:                _activeVehicle && _activeVehicle.airspaceController.hasWeather
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            text:                   _activeVehicle ? _activeVehicle.airspaceController.weatherTemp + "<sup>º</sup>C" : ""
            color:                  _colorWhite
            visible:                _activeVehicle && _activeVehicle.airspaceController.hasWeather
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
