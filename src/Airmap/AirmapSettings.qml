/****************************************************************************
 *
 *   (c) 2009-2016 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/


import QtGraphicalEffects       1.0
import QtMultimedia             5.5
import QtQuick                  2.3
import QtQuick.Controls         1.2
import QtQuick.Controls.Styles  1.4
import QtQuick.Dialogs          1.2
import QtQuick.Layouts          1.2
import QtLocation               5.3
import QtPositioning            5.3

import QGroundControl                       1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

QGCView {
    id:                 _qgcView
    viewPanel:          panel
    color:              qgcPal.window
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 30
    property real _buttonWidth:                 ScreenTools.defaultFontPixelWidth * 18
    property real _panelWidth:                  _qgcView.width * _internalWidthRatio
    property Fact _enableAirMapFact:            QGroundControl.settingsManager.airMapSettings.enableAirMap
    property bool _airMapEnabled:               _enableAirMapFact.rawValue

    readonly property real _internalWidthRatio:          0.8

    QGCPalette { id: qgcPal }

    QGCViewPanel {
        id:             panel
        anchors.fill:   parent
        QGCFlickable {
            clip:               true
            anchors.fill:       parent
            contentHeight:      settingsColumn.height
            contentWidth:       settingsColumn.width
            Column {
                id:                 settingsColumn
                width:              _qgcView.width
                spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                anchors.margins:    ScreenTools.defaultFontPixelWidth
                //-----------------------------------------------------------------
                //-- General
                Item {
                    width:                      _panelWidth
                    height:                     generalLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.unitsSettings.visible
                    QGCLabel {
                        id:             generalLabel
                        text:           qsTr("General")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     generalCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    Column {
                        id:             generalCol
                        spacing:        ScreenTools.defaultFontPixelWidth
                        anchors.centerIn: parent
                        FactCheckBox {
                            text:       qsTr("Enable AirMap Services")
                            fact:       _enableAirMapFact
                            visible:    _enableAirMapFact.visible
                        }
                        FactCheckBox {
                            text:       qsTr("Enable Telemetry")
                            fact:       _enableTelemetryFact
                            visible:    _enableTelemetryFact.visible
                            enabled:    _airMapEnabled
                            property Fact _enableTelemetryFact: QGroundControl.settingsManager.airMapSettings.enableTelemetry
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Login / Registration
                Item {
                    width:                      _panelWidth
                    height:                     loginLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    QGCLabel {
                        id:             loginLabel
                        text:           qsTr("Login / Registration")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     loginGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    GridLayout {
                        id:                 loginGrid
                        columns:            2
                        rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                        anchors.centerIn:   parent
                        QGCLabel { text: qsTr("Email:") }
                        FactTextField {
                            fact:           _loginEmailFact
                            enabled:        _airMapEnabled
                            visible:        _loginEmailFact.visible
                            property Fact _loginEmailFact: QGroundControl.settingsManager.airMapSettings.loginEmail
                        }
                        QGCLabel { text: qsTr("Password:") }
                        FactTextField {
                            fact:           _loginPasswordFact
                            enabled:        _airMapEnabled
                            visible:        _loginPasswordFact.visible
                            property Fact _loginPasswordFact: QGroundControl.settingsManager.airMapSettings.loginPassword
                        }
                        Item {
                            width:  1
                            height: 1
                            Layout.columnSpan: 2
                        }
                        QGCLabel {
                            text:   qsTr("Forgot Your AirMap Password?")
                            Layout.alignment:  Qt.AlignHCenter
                            Layout.columnSpan: 2
                        }
                        Item {
                            width:  1
                            height: 1
                            Layout.columnSpan: 2
                        }
                        QGCButton {
                            text:   qsTr("Register for an AirMap Account")
                            Layout.alignment:  Qt.AlignHCenter
                            Layout.columnSpan: 2
                            enabled:           _airMapEnabled
                            onClicked: {
                                Qt.openUrlExternally("https://www.airmap.com");
                            }
                        }
                    }
                }
                //-----------------------------------------------------------------
                //-- Pilot Profile
                Item {
                    width:                      _panelWidth
                    height:                     profileLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:             profileLabel
                        text:           qsTr("Pilot Profile")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     profileGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    GridLayout {
                        id:                 profileGrid
                        columns:            2
                        columnSpacing:      ScreenTools.defaultFontPixelHeight * 2
                        rowSpacing:         ScreenTools.defaultFontPixelWidth  * 0.25
                        anchors.centerIn:   parent
                        QGCLabel { text: qsTr("Name:") }
                        QGCLabel { text: qsTr("John Doe") }
                        QGCLabel { text: qsTr("User Name:") }
                        QGCLabel { text: qsTr("joe36") }
                        QGCLabel { text: qsTr("Email:") }
                        QGCLabel { text: qsTr("jonh@doe.com") }
                        QGCLabel { text: qsTr("Phone:") }
                        QGCLabel { text: qsTr("+1 212 555 1212") }
                    }
                }
                //-----------------------------------------------------------------
                //-- License (Will this stay here?)
                Item {
                    width:                      _panelWidth
                    height:                     licenseLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:             licenseLabel
                        text:           qsTr("License")
                        font.family:    ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     licenseGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    GridLayout {
                        id:                 licenseGrid
                        columns:            2
                        columnSpacing:      ScreenTools.defaultFontPixelHeight * 2
                        rowSpacing:         ScreenTools.defaultFontPixelWidth  * 0.25
                        anchors.centerIn:   parent
                        QGCLabel        { text: qsTr("API Key:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.apiKey; }
                        QGCLabel        { text: qsTr("Client ID:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.clientID; }
                        QGCLabel        { text: qsTr("User Name:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.userName; }
                        QGCLabel        { text: qsTr("Password:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.password; echoMode: TextInput.Password }
                    }
                }
                //-----------------------------------------------------------------
                //-- Flight List
                Item {
                    width:                      _panelWidth
                    height:                     flightListLabel.height
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    visible:                    QGroundControl.settingsManager.appSettings.visible
                    QGCLabel {
                        id:                     flightListLabel
                        text:                   qsTr("Flight List Management")
                        font.family:            ScreenTools.demiboldFontFamily
                    }
                }
                Rectangle {
                    height:                     flightListButton.height + (ScreenTools.defaultFontPixelHeight * 2)
                    width:                      _panelWidth
                    color:                      qgcPal.windowShade
                    anchors.margins:            ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    QGCButton {
                        id:             flightListButton
                        text:           qsTr("Show Flight List")
                        backRadius:     4
                        heightFactor:   0.3333
                        showBorder:     true
                        width:          ScreenTools.defaultFontPixelWidth * 16
                        anchors.centerIn: parent
                        onClicked: {
                            panelLoader.sourceComponent = flightList
                        }
                    }
                }
            }
        }
        Loader {
            id: panelLoader
            anchors.centerIn: parent
        }
    }
    //---------------------------------------------------------------
    //-- Flight List
    Component {
        id:             flightList
        Rectangle {
            id:         flightListRoot
            width:      _qgcView.width
            height:     _qgcView.height
            color:      qgcPal.window
            property var _flightList: QGroundControl.airspaceManager.flightPlan.flightList
            Component.onCompleted: {
                QGroundControl.airspaceManager.flightPlan.loadFlightList()
            }
            Connections {
                target: _flightList
                onCountChanged: {
                    tableView.resizeColumnsToContents()
                }
            }
            MouseArea {
                anchors.fill:   parent
                hoverEnabled:   true
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            //---------------------------------------------------------
            //-- Flight List
            RowLayout {
                anchors.fill: parent
                TableView {
                    id:                 tableView
                    anchors.top:        parent.top
                    anchors.bottom:     parent.bottom
                    model:              _flightList
                    selectionMode:      SelectionMode.MultiSelection
                    Layout.fillWidth:   true
                    TableViewColumn {
                        title: qsTr("Created")
                        width: ScreenTools.defaultFontPixelWidth * 20
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.createdTime : ""
                            }
                        }
                    }
                    TableViewColumn {
                        title: qsTr("Flight Start")
                        width: ScreenTools.defaultFontPixelWidth * 20
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.startTime : ""
                            }
                        }
                    }
                    TableViewColumn {
                        title: qsTr("Take Off")
                        width: ScreenTools.defaultFontPixelWidth * 22
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.takeOff.latitude.toFixed(6) + ', ' + o.takeOff.longitude.toFixed(6) : ""
                            }
                        }
                    }
                }
                Item {
                    width:              map.width
                    height:             parent.height
                    Layout.alignment:   Qt.AlignTop | Qt.AlignLeft
                    Column {
                        spacing:            ScreenTools.defaultFontPixelHeight
                        anchors.top:        parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:           qsTr("Flight List")
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCButton {
                            text:           qsTr("Refresh")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            enabled:        true
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                QGroundControl.airspaceManager.flightPlan.loadFlightList()
                            }
                        }
                        QGCButton {
                            text:           qsTr("Select All")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            enabled:        _flightList.count > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                tableView.selection.selectAll()
                            }
                        }
                        QGCButton {
                            text:           qsTr("Select None")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            enabled:        _flightList.count > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                tableView.selection.clear()
                            }
                        }
                        QGCButton {
                            text:           qsTr("Delete Selected")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            enabled:        false
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                //-- Clear selection
                                for(var i = 0; i < _flightList.count; i++) {
                                    var o = _flightList.get(i)
                                    if (o) o.selected = false
                                }
                                //-- Flag selected flights
                                tableView.selection.forEach(function(rowIndex){
                                    var o = _flightList.get(rowIndex)
                                    if (o) o.selected = true
                                })
                                //TODO:
                            }
                        }
                        QGCButton {
                            text:           qsTr("Close")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                panelLoader.sourceComponent = null
                            }
                        }
                    }
                    QGCLabel {
                        text:           qsTr("Flight Area")
                        anchors.bottom: map.top
                        anchors.bottomMargin: ScreenTools.defaultFontPixelHeight * 0.25
                        anchors.horizontalCenter: parent.horizontalCenter
                    }
                    Map {
                        id:             map
                        width:          ScreenTools.defaultFontPixelWidth * 40
                        height:         width * 0.6666
                        anchors.bottom: parent.bottom
                        anchors.horizontalCenter: parent.horizontalCenter
                        zoomLevel:      QGroundControl.flightMapZoom
                        center:         QGroundControl.flightMapPosition
                        gesture.acceptedGestures: MapGestureArea.PinchGesture
                        plugin:         Plugin { name: "QGroundControl" }
                        function updateActiveMapType() {
                            var settings =  QGroundControl.settingsManager.flightMapSettings
                            var fullMapName = settings.mapProvider.enumStringValue + " " + settings.mapType.enumStringValue
                            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                                if (fullMapName === map.supportedMapTypes[i].name) {
                                    map.activeMapType = map.supportedMapTypes[i]
                                    return
                                }
                            }
                        }
                        Component.onCompleted: {
                            updateActiveMapType()
                        }
                        Connections {
                            target:             QGroundControl.settingsManager.flightMapSettings.mapType
                            onRawValueChanged:  updateActiveMapType()
                        }

                        Connections {
                            target:             QGroundControl.settingsManager.flightMapSettings.mapProvider
                            onRawValueChanged:  updateActiveMapType()
                        }

                    }
                }
            }
        }
    }
}
