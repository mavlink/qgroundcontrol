import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQml                    2.2

import QGroundControl                   1.0
import QGroundControl.Airmap            1.0
import QGroundControl.Airspace          1.0
import QGroundControl.Controls          1.0
import QGroundControl.Palette           1.0
import QGroundControl.ScreenTools       1.0
import QGroundControl.SettingsManager   1.0

Item {
    height: _valid ? weatherRow.height : 0
    width:  _valid ? weatherRow.width  : 0
    property color  contentColor:       "#ffffff"
    property real   iconHeight:         ScreenTools.defaultFontPixelHeight * 2
    property bool   _valid:             QGroundControl.airspaceManager.weatherInfo.valid
    property bool   _celcius:           QGroundControl.settingsManager.unitsSettings.temperatureUnits.rawValue === UnitsSettings.TemperatureUnitsCelsius
    property int    _tempC:             _valid ? QGroundControl.airspaceManager.weatherInfo.temperature : 0
    property string _tempS:             (_celcius ? _tempC : _tempC * 1.8 + 32).toFixed(0) + (_celcius ? "°C" : "°F")
    Row {
        id:                         weatherRow
        spacing:                    ScreenTools.defaultFontPixelHeight * 0.5
        QGCColoredImage {
            width:                  height
            height:                 iconHeight
            sourceSize.height:      height
            source:                 _valid ? QGroundControl.airspaceManager.weatherInfo.icon : ""
            color:                  contentColor
            visible:                _valid
            fillMode:               Image.PreserveAspectFit
            anchors.verticalCenter: parent.verticalCenter
        }
        QGCLabel {
            text:                   _tempS
            color:                  contentColor
            visible:                _valid
            anchors.verticalCenter: parent.verticalCenter
        }
    }
}
