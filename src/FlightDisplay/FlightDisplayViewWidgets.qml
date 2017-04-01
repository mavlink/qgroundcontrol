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

    property alias  guidedModeBar:          _guidedModeBar
    property bool   gotoEnabled:            _activeVehicle && _activeVehicle.guidedMode && _activeVehicle.flying
    property var    qgcView
    property bool   useLightColors
    property var    missionController

    property var    _activeVehicle:         QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:           _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:    _isSatellite

    // Guided bar properties
    property bool _missionAvailable:    missionController.containsItems
    property bool _missionActive:       _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.missionFlightMode : false
    property bool _vehiclePaused:       _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.pauseFlightMode : false
    property bool _vehicleInRTLMode:    _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.rtlFlightMode : false
    property bool _vehicleInLandMode:   _activeVehicle ? _activeVehicle.flightMode === _activeVehicle.landFlightMode : false
    property int  _resumeMissionItem:   missionController.resumeMissionItem
    property bool _showEmergenyStop:    QGroundControl.corePlugin.options.guidedBarShowEmergencyStop
    property bool _showOrbit:           QGroundControl.corePlugin.options.guidedBarShowOrbit

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
            visible:                    _activeVehicle && !_activeVehicle.coordinateValid && _mainIsMap
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
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
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

    //-- Guided mode buttons
    Rectangle {
        id:                         _guidedModeBar
        anchors.margins:            _barMargin
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      (guidedModeButtons.visible ?  guidedModeButtons.width : guidedModeConfirm.width) + (_margins * 2)
        height:                     (guidedModeButtons.visible ?  guidedModeButtons.height : guidedModeConfirm.height) + (_margins * 2)
        radius:                     ScreenTools.defaultFontPixelHeight * 0.25
        color:                      _backgroundColor
        visible:                    _activeVehicle
        z:                          QGroundControl.zOrderWidgets

        property real   _fontPointSize:     ScreenTools.isMobile ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize
        property color  _backgroundColor:   _isSatellite ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark
        property color  _textColor:         _isSatellite ? qgcPal.mapWidgetBorderDark : qgcPal.mapWidgetBorderLight

        property string _emergencyStopTitle:    qsTr("Emergency Stop")
        property string _armTitle:              qsTr("Arm")
        property string _disarmTitle:           qsTr("Disarm")
        property string _rtlTitle:              qsTr("RTL")
        property string _takeoffTitle:          qsTr("Takeoff")
        property string _landTitle:             qsTr("Land")
        property string _startMissionTitle:     qsTr("Start Mission")
        property string _resumeMissionTitle:    qsTr("Resume Mission")
        property string _pauseTitle:            _missionActive ? qsTr("Pause Mission") : qsTr("Pause")
        property string _changeAltTitle:        qsTr("Change Altitude")
        property string _orbitTitle:            qsTr("Orbit")
        property string _abortTitle:            qsTr("Abort")


        readonly property int confirmHome:                  1
        readonly property int confirmLand:                  2
        readonly property int confirmTakeoff:               3
        readonly property int confirmArm:                   4
        readonly property int confirmDisarm:                5
        readonly property int confirmEmergencyStop:         6
        readonly property int confirmChangeAlt:             7
        readonly property int confirmGoTo:                  8
        readonly property int confirmRetask:                9
        readonly property int confirmOrbit:                 10
        readonly property int confirmAbort:                 11
        readonly property int confirmStartMission:          12
        readonly property int confirmResumeMission:         13
        readonly property int confirmResumeMissionReady:    14

        property int    confirmActionCode
        property real   _showMargin:    _margins
        property real   _hideMargin:    _margins - _guidedModeBar.height
        property real   _barMargin:     _showMargin

        function actionConfirmed() {
            switch (confirmActionCode) {
            case confirmHome:
                _activeVehicle.guidedModeRTL()
                break;
            case confirmLand:
                _activeVehicle.guidedModeLand()
                break;
            case confirmTakeoff:
                _activeVehicle.guidedModeTakeoff()
                break;
            case confirmResumeMission:
                missionController.resumeMission(missionController.resumeMissionItem)
                break;
            case confirmResumeMissionReady:
                _activeVehicle.startMission()
                break;
            case confirmStartMission:
                _activeVehicle.startMission()
                break;
            case confirmArm:
                _activeVehicle.armed = true
                break;
            case confirmDisarm:
                _activeVehicle.armed = false
                break;
            case confirmEmergencyStop:
                _activeVehicle.emergencyStop()
                break;
            case confirmChangeAlt:
                _activeVehicle.guidedModeChangeAltitude(altitudeSlider.getValue())
                break;
            case confirmGoTo:
                _activeVehicle.guidedModeGotoLocation(_flightMap._gotoHereCoordinate)
                break;
            case confirmRetask:
                _activeVehicle.setCurrentMissionSequence(_flightMap._retaskSequence)
                break;
            case confirmOrbit:
                //-- All parameters controlled by RC
                _activeVehicle.guidedModeOrbit()
                //-- Center on current flight map position and orbit with a 50m radius (velocity/direction controlled by the RC)
                //_activeVehicle.guidedModeOrbit(QGroundControl.flightMapPosition, 50.0)
                break;
            case confirmAbort:
                _activeVehicle.abortLanding(50)     // hardcoded value for climbOutAltitude that is currently ignored
                break;
            default:
                console.warn(qsTr("Internal error: unknown confirmActionCode"), confirmActionCode)
            }
        }

        function rejectGuidedModeConfirm() {
            guidedModeButtons.visible = true
            guidedModeConfirm.visible = false
            altitudeSlider.visible = false
            _flightMap._gotoHereCoordinate = QtPositioning.coordinate()
        }

        function confirmAction(actionCode) {
            confirmActionCode = actionCode
            switch (confirmActionCode) {
            case confirmArm:
                guidedModeConfirm.title = _armTitle
                guidedModeConfirm.message = qsTr("arm")
                break;
            case confirmDisarm:
                guidedModeConfirm.title = _disarmTitle
                guidedModeConfirm.message = qsTr("disarm")
                break;
            case confirmEmergencyStop:
                guidedModeConfirm.title = _emergencyStopTitle
                guidedModeConfirm.message = qsTr("WARNING: This still stop all motors. If vehicle is currently in air it will crash.")
                break;
            case confirmTakeoff:
                guidedModeConfirm.title = _takeoffTitle
                guidedModeConfirm.message = qsTr("Takeoff from ground and hold position.")
                break;
            case confirmStartMission:
                guidedModeConfirm.title = _startMissionTitle
                guidedModeConfirm.message = qsTr("Start the mission which is currently displayed above. If the vehicle is on the ground it will takeoff.")
                break;
            case confirmResumeMission:
                guidedModeConfirm.title = _resumeMissionTitle
                guidedModeConfirm.message = qsTr("Resume the mission which is displayed above. This will re-generate the mission from waypoint %1, takeoff and continue the mission.").arg(_resumeMissionItem)
                break;
            case confirmResumeMissionReady:
                guidedModeConfirm.title = _resumeMissionTitle
                guidedModeConfirm.message = qsTr("Review the modified mission above. Confirm if you want to takeoff and begin mission.")
                break;
            case confirmLand:
                guidedModeConfirm.title = _landTitle
                guidedModeConfirm.message = qsTr("Land the vehicle at the current position.")
                break;
            case confirmHome:
                guidedModeConfirm.title = _rtlTitle
                guidedModeConfirm.message = qsTr("Return to the home position of the vehicle.")
                break;
            case confirmChangeAlt:
                altitudeSlider.visible = true
                altitudeSlider.setValue(0)
                guidedModeConfirm.title = _changeAltTitle
                guidedModeConfirm.message = qsTr("Adjust the slider to the left up or down to change the altitude of the vehicle.")
                break;
            case confirmGoTo:
                guidedModeConfirm.title = qsTr("Go To Location")
                guidedModeConfirm.message = qsTr("Confirm to move to the new location.")
                break;
            case confirmRetask:
                guidedModeConfirm.title = qsTr("Waypoint Change")
                guidedModeConfirm.message = qsTr("Confirm to change current mission item to %1.").arg(_flightMap._retaskSequence)
                break;
            case confirmOrbit:
                guidedModeConfirm.title = _orbitTitle
                guidedModeConfirm.message = qsTr("Confirm to enter Orbit mode.")
                break;
            case confirmAbort:
                guidedModeConfirm.title = _abortTitle
                guidedModeConfirm.message = qsTr("Confirm to abort the current landing.")
                break;
            }
            guidedModeButtons.visible = false
            guidedModeConfirm.visible = true
        }

        ColumnLayout {
            id:                 guidedModeButtons
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                color:                      _guidedModeBar._textColor
                text:                       qsTr("Click in map to move vehicle")
                visible:                    gotoEnabled
            }

            Row {
                spacing:    _margins * 2

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._emergencyStopTitle
                    visible:    _showEmergenyStop && _activeVehicle && _activeVehicle.armed && _activeVehicle.flying
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmEmergencyStop)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._disarmTitle
                    visible:    _activeVehicle && _activeVehicle.armed && !_activeVehicle.flying
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmDisarm)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._rtlTitle
                    visible:    _activeVehicle && _activeVehicle.armed && _activeVehicle.guidedModeSupported && _activeVehicle.flying && !_vehicleInRTLMode
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmHome)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._takeoffTitle
                    visible:    _activeVehicle && _activeVehicle.guidedModeSupported && !_activeVehicle.flying  && !_activeVehicle.fixedWing
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmTakeoff)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._landTitle
                    visible:    _activeVehicle && _activeVehicle.guidedModeSupported && _activeVehicle.armed && !_activeVehicle.fixedWing && !_vehicleInLandMode
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmLand)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._startMissionTitle
                    visible:    _activeVehicle && _missionAvailable && !_missionActive
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmStartMission)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._resumeMissionTitle
                    visible:    _activeVehicle && !_activeVehicle.flying && _missionAvailable && _resumeMissionItem !== -1
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmResumeMission)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._pauseTitle
                    visible:    _activeVehicle && _activeVehicle.armed && _activeVehicle.pauseVehicleSupported && _activeVehicle.flying && !_vehiclePaused
                    onClicked:  _activeVehicle.pauseVehicle()
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._changeAltTitle
                    visible:    (_activeVehicle && _activeVehicle.flying) && _activeVehicle.guidedModeSupported && _activeVehicle.armed && !_missionActive
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmChangeAlt)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._orbitTitle
                    visible:    _showOrbit && _activeVehicle && _activeVehicle.flying && _activeVehicle.orbitModeSupported && _activeVehicle.armed && !_missionActive
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmOrbit)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       _guidedModeBar._abortTitle
                    visible:    _activeVehicle && _activeVehicle.flying && _activeVehicle.fixedWing
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmAbort)
                }
            }
        }

        Column {
            id:                 guidedModeConfirm
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins
            visible:            false

            property alias title:   titleText.text
            property alias message: messageText.text

            QGCLabel {
                id:                 titleText
                anchors.left:           slider.left
                anchors.right:          slider.right
                color:                  _guidedModeBar._textColor
                horizontalAlignment:    Text.AlignHCenter
            }

            QGCLabel {
                id:                     messageText
                anchors.left:           slider.left
                anchors.right:          slider.right
                color:                  _guidedModeBar._textColor
                horizontalAlignment:    Text.AlignHCenter
                wrapMode:               Text.WordWrap
            }

            // Action confirmation control
            SliderSwitch {
                id:             slider
                fontPointSize:  _guidedModeBar._fontPointSize
                confirmText:    qsTr("Slide to confirm")
                width:          Math.max(implicitWidth, ScreenTools.defaultFontPixelWidth * 30)

                onAccept: {
                    guidedModeButtons.visible = true
                    guidedModeConfirm.visible = false
                    altitudeSlider.visible = false
                    _guidedModeBar.actionConfirmed()
                }

                onReject: _guidedModeBar.rejectGuidedModeConfirm()
            }
        }

        QGCColoredImage {
            anchors.margins:    _margins
            anchors.top:        _guidedModeBar.top
            anchors.right:      _guidedModeBar.right
            width:              ScreenTools.defaultFontPixelHeight
            height:             width
            sourceSize.height:  width
            source:             "/res/XDelete.svg"
            fillMode:           Image.PreserveAspectFit
            color:              qgcPal.alertText
            visible:            guidedModeConfirm.visible

            QGCMouseArea {
                fillItem:   parent
                onClicked:  _guidedModeBar.rejectGuidedModeConfirm()
            }
        }

    } // Rectangle - Guided mode buttons

    //-- Altitude slider
    Rectangle {
        id:                 altitudeSlider
        anchors.margins:    _margins
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        radius:             ScreenTools.defaultFontPixelWidth / 2
        width:              ScreenTools.defaultFontPixelWidth * 10
        color:              qgcPal.window
        visible:            false

        function setValue(value) {
            altSlider.value = value
        }

        function getValue() {
            return altSlider.value
        }

        Column {
            id:                 headerColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            anchors.right:      parent.right

            QGCLabel {
                anchors.horizontalCenter:   parent.horizontalCenter
                text:                       altSlider.value >=0 ? qsTr("Up") : qsTr("Down")
            }

            QGCLabel {
                id:                         altField
                anchors.horizontalCenter:   parent.horizontalCenter
                text:                       Math.abs(altSlider.value.toFixed(1)) + " " + QGroundControl.appSettingsDistanceUnitsString
            }
        }

        QGCSlider {
            id:                 altSlider
            anchors.margins:    _margins
            anchors.top:        headerColumn.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            orientation:        Qt.Vertical
            //minimumValue:       QGroundControl.metersToAppSettingsDistanceUnits(0)
            //maximumValue:       QGroundControl.metersToAppSettingsDistanceUnits((_activeVehicle && _activeVehicle.flying) ? Math.round((_activeVehicle.altitudeRelative.value + 100) / 100) * 100 : 10)
            minimumValue:       QGroundControl.metersToAppSettingsDistanceUnits(-10)
            maximumValue:       QGroundControl.metersToAppSettingsDistanceUnits(10)
            indicatorCentered:  true
            rotation:           180

            // We want slide up to be positive values
            transform: Rotation {
                origin.x:   altSlider.width / 2
                origin.y:   altSlider.height / 2
                angle:      180
            }
        }
    }
}
