/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
import QGroundControl.Airspace      1.0
import QGroundControl.Airmap        1.0

Item {
    id: widgetRoot

    readonly property real  _rightPanelWidth: Math.min(parent.width / 3, ScreenTools.defaultFontPixelWidth * 30)

    property bool   useLightColors
    property var    missionController
    property bool   showValues:             !QGroundControl.airspaceManager.airspaceVisible

    property bool   _isSatellite:           mainIsMap ? (mainWindow.flightDisplayMap ? mainWindow.flightDisplayMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:    _isSatellite
    property bool   _airspaceEnabled:       QGroundControl.airmapSupported ? QGroundControl.settingsManager.airMapSettings.enableAirMap.rawValue : false

    readonly property real _margins:        ScreenTools.defaultFontPixelHeight * 0.5
    readonly property bool _useAlternateInstrumentPanel:        QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel.value

    QGCMapPalette { id: mapPal; lightColors: useLightColors }

    function getPreferredInstrumentWidth() {
        // Don't allow instrument panel to chew more than 1/4 of full window
        var defaultWidth = ScreenTools.defaultFontPixelWidth * 30
        var maxWidth = mainWindow.width * 0.25
        return Math.min(maxWidth, defaultWidth)
    }

    function _setInstrumentWidget() {
        if(QGroundControl.corePlugin.options.instrumentWidget) {
            if(QGroundControl.corePlugin.options.instrumentWidget.source.toString().length) {
                instrumentsLoader.source = QGroundControl.corePlugin.options.instrumentWidget.source
                switch(QGroundControl.corePlugin.options.instrumentWidget.widgetPosition) {
                case CustomInstrumentWidget.POS_TOP_LEFT:
                    instrumentsLoader.state  = "topLeftMode"
                    break;
                case CustomInstrumentWidget.POS_BOTTOM_LEFT:
                    instrumentsLoader.state  = "bottomLeftMode"
                    break;
                case CustomInstrumentWidget.POS_CENTER_LEFT:
                    instrumentsLoader.state  = "centerLeftMode"
                    break;
                case CustomInstrumentWidget.POS_TOP_RIGHT:
                    instrumentsLoader.state  = "topRightMode"
                    break;
                case CustomInstrumentWidget.POS_BOTTOM_RIGHT:
                    instrumentsLoader.state  = "bottomRightMode"
                    break;
                case CustomInstrumentWidget.POS_CENTER_RIGHT:
                default:
                    instrumentsLoader.state  = "centerRightMode"
                    break;
                }
            } else {
                if(_useAlternateInstrumentPanel){
                    instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidgetAlternate.qml"
                }
                else{
                    instrumentsLoader.source = "qrc:/qml/QGCInstrumentWidget.qml"
                }
            }
        } else {
            instrumentsLoader.source = ""
        }
    }
    Connections {
        target:          QGroundControl.settingsManager.flyViewSettings.alternateInstrumentPanel
        onValueChanged:  _setInstrumentWidget()
    }

    Connections {
        target:         QGroundControl.settingsManager.appSettings.virtualJoystick
        onValueChanged: _setInstrumentWidget()
    }
    Connections {
        target:         QGroundControl.settingsManager.appSettings.virtualJoystickCentralized
        onValueChanged: _setInstrumentWidget()
    }

    Connections {
        target:         QGroundControl.settingsManager.appSettings.showLargeCompass
        onValueChanged: _setInstrumentWidget()
    }

    Connections {
        target: QGroundControl.airspaceManager
        onAirspaceVisibleChanged: {
             widgetRoot.showValues = !QGroundControl.airspaceManager.airspaceVisible
        }
    }

    Component.onCompleted: {
        _setInstrumentWidget()
    }

    //-- Map warnings

    Rectangle {
        anchors.margins:    -ScreenTools.defaultFontPixelHeight
        anchors.fill:       warningsCol
        color:              "white"
        opacity:            0.5
        radius:             ScreenTools.defaultFontPixelWidth / 2
        visible:            warningsCol.noGPSLockVisible || warningsCol.prearmErrorVisible
    }

    Column {
        id:                         warningsCol
        anchors.horizontalCenter:   parent.horizontalCenter
        anchors.top:                parent.verticalCenter
        spacing:                    ScreenTools.defaultFontPixelHeight

        property bool noGPSLockVisible:     activeVehicle && !activeVehicle.coordinate.isValid && mainIsMap
        property bool prearmErrorVisible:   activeVehicle && !activeVehicle.armed && activeVehicle.prearmError

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    warningsCol.noGPSLockVisible
            z:                          QGroundControl.zOrderTopMost
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       qsTr("No GPS Lock for Vehicle")
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    warningsCol.prearmErrorVisible
            z:                          QGroundControl.zOrderTopMost
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       activeVehicle ? activeVehicle.prearmError : ""
        }

        QGCLabel {
            anchors.horizontalCenter:   parent.horizontalCenter
            visible:                    warningsCol.prearmErrorVisible
            width:                      ScreenTools.defaultFontPixelWidth * 50
            horizontalAlignment:        Text.AlignHCenter
            wrapMode:                   Text.WordWrap
            z:                          QGroundControl.zOrderTopMost
            color:                      "black"
            font.pointSize:             ScreenTools.largeFontPointSize
            text:                       "The vehicle has failed a pre-arm check. In order to arm the vehicle, resolve the failure."
        }
    }
    Column {
        id:                     instrumentsColumn
        spacing:                ScreenTools.defaultFontPixelHeight * 0.25
        anchors.top:            parent.top
        anchors.topMargin:      QGroundControl.corePlugin.options.instrumentWidget ? (QGroundControl.corePlugin.options.instrumentWidget.widgetTopMargin + _toolsMargin) : 0
        anchors.margins:        _toolsMargin
        anchors.right:          parent.right
        //-------------------------------------------------------
        // Airmap Airspace Control
        AirspaceControl {
            id:                 airspaceControl
            width:              _rightPanelWidth
            planView:           false
            visible:            _airspaceEnabled
        }
        //-------------------------------------------------------
        //-- Instrument Panel
        Loader {
            id:                         instrumentsLoader
            anchors.margins:            _toolsMargin
            property real maxHeight:    widgetRoot ? widgetRoot.height - instrumentsColumn.y - airspaceControl.height - (ScreenTools.defaultFontPixelHeight * 4) : 0
            states: [
                State {
                    name:   "topRightMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.verticalCenter: undefined
                        anchors.bottom:         undefined
                        anchors.top:            widgetRoot ? widgetRoot.top : undefined
                        anchors.right:          widgetRoot ? widgetRoot.right : undefined
                        anchors.left:           undefined
                    }
                },
                State {
                    name:   "centerRightMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.top:            undefined
                        anchors.bottom:         undefined
                        anchors.verticalCenter: widgetRoot ? widgetRoot.verticalCenter : undefined
                        anchors.right:          widgetRoot ? widgetRoot.right : undefined
                        anchors.left:           undefined
                    }
                },
                State {
                    name:   "bottomRightMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.top:            undefined
                        anchors.verticalCenter: undefined
                        anchors.bottom:         widgetRoot ? widgetRoot.bottom : undefined
                        anchors.right:          widgetRoot ? widgetRoot.right : undefined
                        anchors.left:           undefined
                    }
                },
                State {
                    name:   "topLeftMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.verticalCenter: undefined
                        anchors.bottom:         undefined
                        anchors.top:            widgetRoot ? widgetRoot.top : undefined
                        anchors.right:          undefined
                        anchors.left:           widgetRoot ? widgetRoot.left : undefined
                    }
                },
                State {
                    name:   "centerLeftMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.top:            undefined
                        anchors.bottom:         undefined
                        anchors.verticalCenter: widgetRoot ? widgetRoot.verticalCenter : undefined
                        anchors.right:          undefined
                        anchors.left:           widgetRoot ? widgetRoot.left : undefined
                    }
                },
                State {
                    name:   "bottomLeftMode"
                    AnchorChanges {
                        target:                 instrumentsLoader
                        anchors.top:            undefined
                        anchors.verticalCenter: undefined
                        anchors.bottom:         widgetRoot ? widgetRoot.bottom : undefined
                        anchors.right:          undefined
                        anchors.left:           widgetRoot ? widgetRoot.left : undefined
                    }
                }
            ]
        }
    }
}
