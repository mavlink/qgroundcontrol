/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtQuick                  2.4
import QtQuick.Controls         1.3
import QtQuick.Controls.Styles  1.2
import QtQuick.Dialogs          1.2
import QtLocation               5.3
import QtPositioning            5.2

import QGroundControl               1.0
import QGroundControl.ScreenTools   1.0
import QGroundControl.Controls      1.0
import QGroundControl.Palette       1.0
import QGroundControl.Vehicle       1.0
import QGroundControl.FlightMap     1.0

Item {
    id: _root

    property alias guidedModeBar: _guidedModeBar

    property var    _activeVehicle:             QGroundControl.multiVehicleManager.activeVehicle
    property bool   _isSatellite:               _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _lightWidgetBorders:        _mainIsMap ? (_flightMap ? _flightMap.isSatelliteMap : true) : true
    property bool   _useAlternateInstruments:   QGroundControl.virtualTabletJoystick || ScreenTools.isTinyScreen


    readonly property real _margins: ScreenTools.defaultFontPixelHeight / 2

    QGCMapPalette { id: mapPal; lightColors: !isBackgroundDark }
    QGCPalette { id: qgcPal }

    function getGadgetWidth() {
        if(ScreenTools.isMobile) {
            return ScreenTools.isTinyScreen ? mainWindow.width * 0.2 : mainWindow.width * 0.15
        }
        var w = mainWindow.width * 0.15
        return Math.min(w, 200)
    }

    ExclusiveGroup {
        id: _dropButtonsExclusiveGroup
    }

    ExclusiveGroup {
        id: _mapTypeButtonsExclusiveGroup
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

    //-- Dismiss Drop Down (if any)
    MouseArea {
        anchors.fill:   parent
        enabled:        _dropButtonsExclusiveGroup.current != null
        onClicked: {
            if(_dropButtonsExclusiveGroup.current)
                _dropButtonsExclusiveGroup.current.checked = false
            _dropButtonsExclusiveGroup.current = null
        }
    }

    //-- Instrument Panel
    QGCInstrumentWidget {
        id:                     instrumentGadget
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
        anchors.verticalCenter: parent.verticalCenter
        visible:                !_useAlternateInstruments
        size:                   getGadgetWidth()
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        groundSpeedFact:        _groundSpeedFact
        airSpeedFact:           _airSpeedFact
        lightBorders:           _lightWidgetBorders
        z:                      QGroundControl.zOrderWidgets
        qgcView:                parent.parent.qgcView
        maxHeight:              parent.height - (anchors.margins * 2)
    }

    QGCInstrumentWidgetAlternate {
        id:                     instrumentGadgetAlternate
        anchors.margins:        ScreenTools.defaultFontPixelHeight / 2
        anchors.top:            parent.top
        anchors.right:          altitudeSlider.visible ? altitudeSlider.left : parent.right
        visible:                _useAlternateInstruments
        width:                  ScreenTools.isTinyScreen ? getGadgetWidth() * 1.5 : getGadgetWidth()
        active:                 _activeVehicle != null
        heading:                _heading
        rollAngle:              _roll
        pitchAngle:             _pitch
        groundSpeedFact:        _groundSpeedFact
        airSpeedFact:           _airSpeedFact
        isSatellite:            _isSatellite
        z:                      QGroundControl.zOrderWidgets
    }

    ValuesWidget {
        anchors.topMargin:          ScreenTools.defaultFontPixelHeight
        anchors.top:                instrumentGadgetAlternate.bottom
        anchors.horizontalCenter:   instrumentGadgetAlternate.horizontalCenter
        width:                      getGadgetWidth()
        qgcView:                    parent.parent.qgcView
        textColor:                  _isSatellite ? "white" : "black"
        visible:                    _useAlternateInstruments
        maxHeight:                  virtualJoystickMultiTouch.visible ? virtualJoystickMultiTouch.y - y : parent.height - anchors.margins - y
    }

    //-- Vertical Tool Buttons
    Column {
        id:                         toolColumn
        visible:                    _mainIsMap
        anchors.margins:            ScreenTools.defaultFontPixelHeight
        anchors.left:               parent.left
        anchors.top:                parent.top
        spacing:                    ScreenTools.defaultFontPixelHeight

        //-- Map Center Control
        DropButton {
            id:                 centerMapDropButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/MapCenter.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders

            dropDownComponent: Component {
                Row {
                    spacing: ScreenTools.defaultFontPixelWidth

                    QGCCheckBox {
                        id:         followVehicleCheckBox
                        text:       qsTr("Follow Vehicle")
                        checked:    _flightMap ? _flightMap._followVehicle : false
                        anchors.verticalCenter: parent.verticalCenter
                        //anchors.baseline:   centerMapButton.baseline - This doesn't work correctly on mobile for some strange reason, so we center instead

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap._followVehicle = !_flightMap._followVehicle
                        }
                    }

                    QGCButton {
                        id:         centerMapButton
                        text:       qsTr("Center map on Vehicle")
                        enabled:    _activeVehicle && !followVehicleCheckBox.checked

                        property var activeVehicle: QGroundControl.multiVehicleManager.activeVehicle

                        onClicked: {
                            _dropButtonsExclusiveGroup.current = null
                            _flightMap.center = activeVehicle.coordinate
                        }
                    }
                }
            }
        }

        //-- Map Type Control
        DropButton {
            id:                 mapTypeButton
            dropDirection:      dropRight
            buttonImage:        "/qmlimages/MapType.svg"
            viewportMargins:    ScreenTools.defaultFontPixelWidth / 2
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders

            dropDownComponent: Component {
                Column {
                    spacing: ScreenTools.defaultFontPixelWidth

                    Row {
                        spacing: ScreenTools.defaultFontPixelWidth

                        Repeater {
                            model: QGroundControl.flightMapSettings.mapTypes

                            QGCButton {
                                checkable:      true
                                checked:        QGroundControl.flightMapSettings.mapType === text
                                text:           modelData
                                width:          clearButton.width
                                exclusiveGroup: _mapTypeButtonsExclusiveGroup

                                onClicked: {
                                    QGroundControl.flightMapSettings.mapType = text
                                    checked = true
                                    _dropButtonsExclusiveGroup.current = null
                                }
                            }
                        }
                    }

                    QGCButton {
                        id:         clearButton
                        text:       qsTr("Clear Flight Trails")
                        enabled:    QGroundControl.multiVehicleManager.activeVehicle
                        onClicked: {
                            QGroundControl.multiVehicleManager.activeVehicle.clearTrajectoryPoints()
                            _dropButtonsExclusiveGroup.current = null
                        }
                    }
                }
            }
        }

        //-- Zoom Map In
        RoundButton {
            id:                 mapZoomPlus
            visible:            !ScreenTools.isTinyScreen && _mainIsMap
            buttonImage:        "/qmlimages/ZoomPlus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel += 0.5
                checked = false
            }
        }

        //-- Zoom Map Out
        RoundButton {
            id:                 mapZoomMinus
            visible:            !ScreenTools.isTinyScreen && _mainIsMap
            buttonImage:        "/qmlimages/ZoomMinus.svg"
            exclusiveGroup:     _dropButtonsExclusiveGroup
            z:                  QGroundControl.zOrderWidgets
            lightBorders:       _lightWidgetBorders
            onClicked: {
                if(_flightMap)
                    _flightMap.zoomLevel -= 0.5
                checked = false
            }
        }
    }

    //-- Guided mode buttons
    Rectangle {
        id:                         _guidedModeBar
        anchors.margins:            _barMargin
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        width:                      guidedModeColumn.width  + (_margins * 2)
        height:                     guidedModeColumn.height + (_margins * 2)
        radius:                     _margins
        color:                      _lightWidgetBorders ? qgcPal.mapWidgetBorderLight : qgcPal.mapWidgetBorderDark
        visible:                    _activeVehicle
        opacity:                    0.9
        z:                          QGroundControl.zOrderWidgets
        state:                      "Shown"

        property real _fontPointSize: ScreenTools.isMobile ? ScreenTools.largeFontPointSize : ScreenTools.defaultFontPointSize

        states: [
            State {
                name: "Shown"
                PropertyChanges { target: showAnimation; running: true  }
                PropertyChanges { target: guidedModeHideTimer; running: true }
            },
            State {
                name: "Hidden"
                PropertyChanges { target: hideAnimation; running: true  }
            }
        ]

        PropertyAnimation {
            id:             hideAnimation
            target:         _guidedModeBar
            property:       "_barMargin"
            duration:       1000
            easing.type:    Easing.InOutQuad
            from:           _guidedModeBar._showMargin
            to:             _guidedModeBar._hideMargin
        }

        PropertyAnimation {
            id:             showAnimation
            target:         _guidedModeBar
            property:       "_barMargin"
            duration:       250
            easing.type:    Easing.InOutQuad
            from:           _guidedModeBar._hideMargin
            to:             _guidedModeBar._showMargin
        }

        Timer {
            id:             guidedModeHideTimer
            interval:       7000
            running:        true
            onTriggered: {
                if (ScreenTools.isShortScreen) {
                    _guidedModeBar.state = "Hidden"
                }
            }
        }

        readonly property int confirmHome:          1
        readonly property int confirmLand:          2
        readonly property int confirmTakeoff:       3
        readonly property int confirmArm:           4
        readonly property int confirmDisarm:        5
        readonly property int confirmEmergencyStop: 6
        readonly property int confirmChangeAlt:     7
        readonly property int confirmGoTo:          8
        readonly property int confirmRetask:        9

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
                var altitude1 = altitudeSlider.getValue()
                if (!isNaN(altitude1)) {
                    _activeVehicle.guidedModeTakeoff(altitude1)
                }
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
                var altitude2 = altitudeSlider.getValue()
                if (!isNaN(altitude2)) {
                    _activeVehicle.guidedModeChangeAltitude(altitude2)
                }
                break;
            case confirmGoTo:
                _activeVehicle.guidedModeGotoLocation(_flightMap._gotoHereCoordinate)
                break;
            case confirmRetask:
                _activeVehicle.setCurrentMissionSequence(_flightMap._retaskSequence)
                break;
            default:
                console.warn(qsTr("Internal error: unknown confirmActionCode"), confirmActionCode)
            }
        }

        function rejectGuidedModeConfirm() {
            guidedModeConfirm.visible = false
            _guidedModeBar.visible = true
            altitudeSlider.visible = false
            _flightMap._gotoHereCoordinate = QtPositioning.coordinate()
            guidedModeHideTimer.restart()
        }

        function confirmAction(actionCode) {
            guidedModeHideTimer.stop()
            confirmActionCode = actionCode
            switch (confirmActionCode) {
            case confirmArm:
                guidedModeConfirm.confirmText = qsTr("arm")
                break;
            case confirmDisarm:
                guidedModeConfirm.confirmText = qsTr("disarm")
                break;
            case confirmEmergencyStop:
                guidedModeConfirm.confirmText = qsTr("STOP ALL MOTORS!")
                break;
            case confirmTakeoff:
                altitudeSlider.visible = true
                altitudeSlider.setInitialValueMeters(2)
                guidedModeConfirm.confirmText = qsTr("takeoff")
                break;
            case confirmLand:
                guidedModeConfirm.confirmText = qsTr("land")
                break;
            case confirmHome:
                guidedModeConfirm.confirmText = qsTr("return to land")
                break;
            case confirmChangeAlt:
                altitudeSlider.visible = true
                altitudeSlider.setInitialValueAppSettingsDistanceUnits(_activeVehicle.altitudeAMSL.value)
                guidedModeConfirm.confirmText = qsTr("change altitude")
                break;
            case confirmGoTo:
                guidedModeConfirm.confirmText = qsTr("move vehicle")
                break;
            case confirmRetask:
                guidedModeConfirm.confirmText = qsTr("active waypoint change")
                break;
            }
            _guidedModeBar.visible = false
            guidedModeConfirm.visible = true
        }

        Column {
            id:                 guidedModeColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            spacing:            _margins

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                color:      _lightWidgetBorders ? qgcPal.mapWidgetBorderDark : qgcPal.mapWidgetBorderLight
                text:       "Click in map to move vehicle"
                visible:    _activeVehicle && _activeVehicle.guidedMode && _activeVehicle.flying
            }

            Row {
                spacing: _margins * 2

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       (_activeVehicle && _activeVehicle.armed) ? (_activeVehicle.flying ? qsTr("Emergency Stop") : qsTr("Disarm")) :  qsTr("Arm")
                    visible:    _activeVehicle
                    onClicked:  _guidedModeBar.confirmAction(_activeVehicle.armed ? (_activeVehicle.flying ? _guidedModeBar.confirmEmergencyStop : _guidedModeBar.confirmDisarm) : _guidedModeBar.confirmArm)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       qsTr("RTL")
                    visible:    (_activeVehicle && _activeVehicle.armed) && _activeVehicle.guidedModeSupported && _activeVehicle.flying
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmHome)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       (_activeVehicle && _activeVehicle.flying) ?  qsTr("Land"):  qsTr("Takeoff")
                    visible:    _activeVehicle && _activeVehicle.guidedModeSupported && _activeVehicle.armed
                    onClicked:  _guidedModeBar.confirmAction(_activeVehicle.flying ? _guidedModeBar.confirmLand : _guidedModeBar.confirmTakeoff)
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       qsTr("Pause")
                    visible:    (_activeVehicle && _activeVehicle.armed) && _activeVehicle.pauseVehicleSupported && _activeVehicle.flying
                    onClicked:  {
                        guidedModeHideTimer.restart()
                        _activeVehicle.pauseVehicle()
                    }
                }

                QGCButton {
                    pointSize:  _guidedModeBar._fontPointSize
                    text:       qsTr("Change Altitude")
                    visible:    (_activeVehicle && _activeVehicle.flying) && _activeVehicle.guidedModeSupported && _activeVehicle.armed
                    onClicked:  _guidedModeBar.confirmAction(_guidedModeBar.confirmChangeAlt)
                }
            } // Row
        } // Column
    } // Rectangle - Guided mode buttons

    MouseArea {
        anchors.fill:   parent
        enabled:        guidedModeConfirm.visible
        onClicked:      _guidedModeBar.rejectGuidedModeConfirm()
    }

    // Action confirmation control
    SliderSwitch {
        id:                         guidedModeConfirm
        anchors.bottomMargin:       _margins
        anchors.bottom:             parent.bottom
        anchors.horizontalCenter:   parent.horizontalCenter
        visible:                    false
        z:                          QGroundControl.zOrderWidgets
        fontPointSize:              _guidedModeBar._fontPointSize

        onAccept: {
            guidedModeConfirm.visible = false
            _guidedModeBar.visible = true
            _guidedModeBar.actionConfirmed()
            altitudeSlider.visible = false
            guidedModeHideTimer.restart()
        }

        onReject: _guidedModeBar.rejectGuidedModeConfirm()
    }

    //-- Altitude slider
    Rectangle {
        id:                 altitudeSlider
        anchors.margins:    _margins
        anchors.right:      parent.right
        anchors.top:        parent.top
        anchors.bottom:     parent.bottom
        color:              qgcPal.window
        width:              ScreenTools.defaultFontPixelWidth * 10
        opacity:            0.8
        visible:            false

        function setInitialValueMeters(meters) {
            altSlider.value = QGroundControl.metersToAppSettingsDistanceUnits(meters)
        }

        function setInitialValueAppSettingsDistanceUnits(height) {
            altSlider.value = height
        }

        /// Returns NaN for bad value
        function getValue() {
            var value =  parseFloat(altField.text)
            if (!isNaN(value)) {
                return QGroundControl.appSettingsDistanceUnitsToMeters(value);
            } else {
                return value;
            }
        }

        Column {
            id:                 headerColumn
            anchors.margins:    _margins
            anchors.top:        parent.top
            anchors.left:       parent.left
            anchors.right:      parent.right

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: qsTr("Alt (rel)")
            }

            QGCLabel {
                anchors.horizontalCenter: parent.horizontalCenter
                text: QGroundControl.appSettingsDistanceUnitsString
            }

            QGCTextField {
                id:             altField
                anchors.left:   parent.left
                anchors.right:  parent.right
                text:           altSlider.value.toFixed(1)
            }
        }

        Slider {
            id:                 altSlider
            anchors.margins:    _margins
            anchors.top:        headerColumn.bottom
            anchors.bottom:     parent.bottom
            anchors.left:       parent.left
            anchors.right:      parent.right
            orientation:        Qt.Vertical
            minimumValue:       QGroundControl.metersToAppSettingsDistanceUnits(2)
            maximumValue:       QGroundControl.metersToAppSettingsDistanceUnits((_activeVehicle && _activeVehicle.flying) ? 100 : 10)
        }
    }
}
