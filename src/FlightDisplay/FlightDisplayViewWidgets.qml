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

import QGroundControl                           1.0
import QGroundControl.ScreenTools               1.0
import QGroundControl.Controls                  1.0
import QGroundControl.Palette                   1.0
import QGroundControl.Vehicle                   1.0
import QGroundControl.FlightMap                 1.0

Item {
    id: _root

    property var    qgcView
    property bool   useLightColors
    property var    missionController

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:           _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:    _isSatellite

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight * 0.5

    QGCMapPalette { id: mapPal; lightColors: useLightColors }
    QGCPalette    { id: qgcPal }

    function getPreferredInstrumentWidth() {
        if(ScreenTools.isMobile) {
            return ScreenTools.isTinyScreen ? mainWindow.width * 0.2 : mainWindow.width * 0.15
        }
        var w = mainWindow.width * 0.15
        return Math.min(w, 200)
    }

    function _setInstrumentWidget() {
        if(QGroundControl.corePlugin.options.instrumentWidget.source.toString().length) {
            instrumentsLoader.source = QGroundControl.corePlugin.options.instrumentWidget.source
            switch(QGroundControl.corePlugin.options.instrumentWidget.widgetPosition) {
            case CustomInstrumentWidget.POS_TOP_RIGHT:
                instrumentsLoader.state  = "topMode"
                break;
            case CustomInstrumentWidget.POS_BOTTOM_RIGHT:
                instrumentsLoader.state  = "bottomMode"
                break;
            case CustomInstrumentWidget.POS_CENTER_RIGHT:
            default:
                instrumentsLoader.state  = "centerMode"
                break;
            }
        } else {
            var useAlternateInstruments = QGroundControl.settingsManager.appSettings.virtualJoystick.value || ScreenTools.isTinyScreen
            if(useAlternateInstruments) {
                instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidgetAlternate.qml"
                instrumentsLoader.state  = "topMode"
            } else {
                instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidget.qml"
                instrumentsLoader.state  = QGroundControl.settingsManager.appSettings.showLargeCompass.value == 1 ? "centerMode" : "topMode"
            }
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
        target:                 missionController
        onResumeMissionReady:   _guidedModeBar.confirmAction(_guidedModeBar.confirmResumeMissionReady)
    }

    Component.onCompleted: {
        _setInstrumentWidget()
    }

    //-- Map warnings
    Column {
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.verticalCenter:     parent.verticalCenter
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
    }

    //-- Instrument Panel
    Loader {
        id:                     instrumentsLoader
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.right:          parent.right
        z:                      QGroundControl.zOrderWidgets
        property var  qgcView:  _root.qgcView
        property real maxHeight:parent.height - (anchors.margins * 2)
        states: [
            State {
                name:   "topMode"
                AnchorChanges {
                    target:                 instrumentsLoader
                    anchors.verticalCenter: undefined
                    anchors.bottom:         undefined
                    anchors.top:            _root ? _root.top : undefined
                }
            },
            State {
                name:   "centerMode"
                AnchorChanges {
                    target:                 instrumentsLoader
                    anchors.top:            undefined
                    anchors.bottom:         undefined
                    anchors.verticalCenter: _root ? _root.verticalCenter : undefined
                }
            },
            State {
                name:   "bottomMode"
                AnchorChanges {
                    target:                 instrumentsLoader
                    anchors.top:            undefined
                    anchors.verticalCenter: undefined
                    anchors.bottom:         _root ? _root.bottom : undefined
                }
            }
        ]
    }
}
