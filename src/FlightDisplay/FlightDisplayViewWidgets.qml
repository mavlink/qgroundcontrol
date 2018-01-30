/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.3
import QtQuick.Layouts          1.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0
import QGroundControl.Airmap        1.0

Item {
    id: widgetRoot

    property var    qgcView
    property bool   useLightColors
    property var    missionController
    property bool   showValues:             _activeVehicle ? !_activeVehicle.airspaceController.airspaceVisible : true

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:           _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:    _isSatellite
    property bool   _enableAirMap:          QGroundControl.airmapSupported ? QGroundControl.settingsManager.airMapSettings.enableAirMap.rawValue : false

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight * 0.5

    QGCMapPalette { id: mapPal; lightColors: useLightColors }
    QGCPalette    { id: qgcPal }

    function getPreferredInstrumentWidth() {
        if(ScreenTools.isMobile) {
            return mainWindow.width * 0.25
        } else if(ScreenTools.isHugeScreen) {
            return mainWindow.width * 0.11
        }
        return ScreenTools.defaultFontPixelWidth * 30
    }

    function _setInstrumentWidget() {
        if(QGroundControl.corePlugin.options.instrumentWidget) {
            if(QGroundControl.corePlugin.options.instrumentWidget.source.toString().length) {
                instrumentsLoader.source = QGroundControl.corePlugin.options.instrumentWidget.source
            } else {
                // Note: We currently show alternate instruments all the time. This is a trial change for daily builds.
                // Leaving non-alternate code in for now in case the trial fails.
                var useAlternateInstruments = true//QGroundControl.settingsManager.appSettings.virtualJoystick.value || ScreenTools.isTinyScreen
                if(useAlternateInstruments) {
                    instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidgetAlternate.qml"
                } else {
                    instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidget.qml"
                }
            }
        } else {
            instrumentsLoader.source = ""
        }
    }

    Connections {
        target:         QGroundControl.settingsManager.appSettings.virtualJoystick
        onValueChanged: _setInstrumentWidget()
    }

    Connections {
        target:         QGroundControl.settingsManager.appSettings.showLargeCompass
        onValueChanged: _setInstrumentWidget()
    }

    Connections {
        target:         _activeVehicle ? _activeVehicle.airspaceController : null
        onAirspaceVisibleChanged: {
            if(_activeVehicle) {
                widgetRoot.showValues = !_activeVehicle.airspaceController.airspaceVisible
            }
        }
    }

    Component.onCompleted: {
        _setInstrumentWidget()
    }

    //-- Map warnings
    Column {
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.verticalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _activeVehicle && !_activeVehicle.coordinate.isValid && _mainIsMap
            z:                          QGroundControl.zOrderTopMost
            color:                      mapPal.text
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("No GPS Lock for Vehicle")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _activeVehicle && _activeVehicle.prearmError
            z:                          QGroundControl.zOrderTopMost
            color:                      mapPal.text
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       _activeVehicle ? _activeVehicle.prearmError : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    _activeVehicle && _activeVehicle.prearmError
            width:                      ScreenTools.defaultFontPixelWidth * 50
            horizontalAlignment:        Text.AlignHCenter
            wrapMode:                   Text.WordWrap
            z:                          QGroundControl.zOrderTopMost
            color:                      mapPal.text
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       "The vehicle has failed a pre-arm check. In order to arm the vehicle, resolve the failure or disable the arming check via the Safety tab on the Vehicle Setup page."
        }
    }
    Column {
        id:                     instrumentsColumn
        spacing:                ScreenTools.defaultFontPixelHeight * 0.25
        anchors.top:            parent.top
        anchors.topMargin:      QGroundControl.corePlugin.options.instrumentWidget.widgetTopMargin + (ScreenTools.defaultFontPixelHeight * 0.5)
        anchors.margins:        ScreenTools.defaultFontPixelHeight * 0.5
        anchors.right:          parent.right
        //-------------------------------------------------------
        // Airmap Airspace Control
        AirspaceControl {
            id:                 airspaceControl
            width:              getPreferredInstrumentWidth()
            visible:            _enableAirMap
            anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.5
        }
        //-------------------------------------------------------
        //-- Instrument Panel
        Loader {
            id:                         instrumentsLoader
            anchors.margins:            ScreenTools.defaultFontPixelHeight * 0.5
            property var  qgcView:      widgetRoot.qgcView
            property real maxHeight:    widgetRoot ? widgetRoot.height - instrumentsColumn.y - airspaceControl.height - (ScreenTools.defaultFontPixelHeight * 4) : 0
        }
    }
}
