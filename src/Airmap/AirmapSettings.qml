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
                            width:          _editFieldWidth
                            enabled:        _airMapEnabled
                            visible:        _loginEmailFact.visible
                            property Fact _loginEmailFact: QGroundControl.settingsManager.airMapSettings.loginEmail
                        }
                        QGCLabel { text: qsTr("Password:") }
                        FactTextField {
                            fact:           _loginPasswordFact
                            width:          _editFieldWidth
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
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.apiKey;   width: _editFieldWidth; }
                        QGCLabel        { text: qsTr("Client ID:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.clientID; width: _editFieldWidth; }
                        QGCLabel        { text: qsTr("User Name:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.userName; width: _editFieldWidth; }
                        QGCLabel        { text: qsTr("Password:") }
                        FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.password; width: _editFieldWidth; echoMode: TextInput.Password }
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
            property var    _flightList: QGroundControl.airspaceManager.flightPlan.flightList
            property real   _mapWidth:   ScreenTools.defaultFontPixelWidth * 40
            MouseArea {
                anchors.fill:   parent
                hoverEnabled:   true
                onWheel:        { wheel.accepted = true; }
                onPressed:      { mouse.accepted = true; }
                onReleased:     { mouse.accepted = true; }
            }
            function updateSelection() {
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
                        title:  qsTr("No")
                        width:  ScreenTools.defaultFontPixelWidth * 3
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text {
                            horizontalAlignment: Text.AlignHCenter
                            text:  (styleData.row + 1)
                            color: tableView.currentRow === styleData.row ? qgcPal.colorBlue : "black"
                            font.family: ScreenTools.fixedFontFamily
                            font.pixelSize: ScreenTools.smallFontPointSize
                        }
                    }
                    TableViewColumn {
                        title:  qsTr("Created")
                        width:  ScreenTools.defaultFontPixelWidth * 20
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.createdTime : ""
                            }
                            color: tableView.currentRow === styleData.row ? qgcPal.colorBlue : "black"
                            font.family: ScreenTools.fixedFontFamily
                            font.pixelSize: ScreenTools.smallFontPointSize
                        }
                    }
                    TableViewColumn {
                        title:  qsTr("Flight Start")
                        width:  ScreenTools.defaultFontPixelWidth * 20
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.startTime : ""
                            }
                            color: tableView.currentRow === styleData.row ? qgcPal.colorBlue : "black"
                            font.family: ScreenTools.fixedFontFamily
                            font.pixelSize: ScreenTools.smallFontPointSize
                        }
                    }
                    TableViewColumn {
                        title:  qsTr("State")
                        width:  ScreenTools.defaultFontPixelWidth * 22
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? (o.beingDeleted ? qsTr("Deleting") : qsTr("Valid")) : qsTr("Unknown")
                            }
                            color: tableView.currentRow === styleData.row ? qgcPal.colorBlue : "black"
                            font.family: ScreenTools.fixedFontFamily
                            font.pixelSize: ScreenTools.smallFontPointSize
                        }
                    }
                }
                Item {
                    width:              flightListRoot._mapWidth
                    height:             parent.height
                    Layout.alignment:   Qt.AlignTop | Qt.AlignLeft
                    QGCLabel {
                        id:             loadingLabel
                        text:           qsTr("Loading Flight List")
                        visible:        QGroundControl.airspaceManager.flightPlan.loadingFlightList
                        anchors.centerIn: parent
                    }
                    QGCColoredImage {
                        id:                 busyIndicator
                        height:             ScreenTools.defaultFontPixelHeight * 2.5
                        width:              height
                        source:             "/qmlimages/MapSync.svg"
                        sourceSize.height:  height
                        fillMode:           Image.PreserveAspectFit
                        mipmap:             true
                        smooth:             true
                        color:              qgcPal.colorGreen
                        visible:            loadingLabel.visible
                        anchors.top:        loadingLabel.bottom
                        anchors.topMargin:  ScreenTools.defaultFontPixelHeight
                        anchors.horizontalCenter: parent.horizontalCenter
                        RotationAnimation on rotation {
                            loops:          Animation.Infinite
                            from:           360
                            to:             0
                            duration:       740
                            running:        busyIndicator.visible
                        }
                    }
                    Column {
                        spacing:            ScreenTools.defaultFontPixelHeight * 0.75
                        visible:            !QGroundControl.airspaceManager.flightPlan.loadingFlightList
                        anchors.top:        parent.top
                        anchors.horizontalCenter: parent.horizontalCenter
                        QGCLabel {
                            text:           qsTr("Flight List")
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        Rectangle {
                            color:          qgcPal.window
                            border.color:   qgcPal.globalTheme === QGCPalette.Dark ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.25)
                            border.width:   1
                            radius:         4
                            width:          _mapWidth - (ScreenTools.defaultFontPixelWidth * 2)
                            height:         rangeCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                            Column {
                                id:         rangeCol
                                anchors.centerIn: parent
                                spacing:    ScreenTools.defaultFontPixelHeight * 0.5
                                QGCLabel {
                                    text:   qsTr("Range")
                                    anchors.horizontalCenter: parent.horizontalCenter
                                }
                                Row {
                                    spacing:            ScreenTools.defaultFontPixelWidth * 2
                                    anchors.horizontalCenter: parent.horizontalCenter
                                    Column {
                                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                        QGCButton {
                                            text:           qsTr("From")
                                            backRadius:     4
                                            heightFactor:   0.3333
                                            showBorder:     true
                                            width:          _buttonWidth * 0.5
                                            onClicked:      fromPicker.visible = true
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }
                                        QGCLabel {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: fromPicker.selectedDate.toLocaleDateString(Qt.locale())
                                        }
                                    }
                                    Rectangle {
                                        width:  1
                                        height: parent.height
                                        color:  qgcPal.globalTheme === QGCPalette.Dark ? Qt.rgba(1,1,1,0.25) : Qt.rgba(0,0,0,0.25)
                                    }
                                    Column {
                                        spacing:            ScreenTools.defaultFontPixelHeight * 0.5
                                        QGCButton {
                                            text:           qsTr("To")
                                            backRadius:     4
                                            heightFactor:   0.3333
                                            showBorder:     true
                                            width:          _buttonWidth * 0.5
                                            onClicked:      toPicker.visible = true
                                            anchors.horizontalCenter: parent.horizontalCenter
                                        }
                                        QGCLabel {
                                            anchors.horizontalCenter: parent.horizontalCenter
                                            text: toPicker.selectedDate.toLocaleDateString(Qt.locale())
                                        }
                                    }
                                }
                            }
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
                                var start   = fromPicker.selectedDate
                                var end     = toPicker.selectedDate
                                start.setHours(0,0,0,0)
                                end.setHours(23,59,59,0)
                                QGroundControl.airspaceManager.flightPlan.loadFlightList(start, end)
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
                            enabled:        tableView.selection.count > 0
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                flightListRoot.updateSelection();
                                QGroundControl.airspaceManager.flightPlan.deleteSelectedFlights()
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
                        QGCLabel {
                            text:           _flightList.count > 0 ? tableView.selection.count + '/' + _flightList.count + qsTr(" Flights Selected") : qsTr("No Flights Loaded")
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                        QGCLabel {
                            text:           qsTr("A maximum of 250 flights were loaded")
                            color:          qgcPal.colorOrange
                            font.pixelSize: ScreenTools.smallFontPointSize
                            visible:        _flightList.count >= 250
                            anchors.horizontalCenter: parent.horizontalCenter
                        }
                    }
                    QGCLabel {
                        text:           qsTr("Flight Area ") + (tableView.currentRow + 1)
                        visible:        !QGroundControl.airspaceManager.flightPlan.loadingFlightList && _flightList.count > 0 && tableView.currentRow >= 0
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
                        visible:        !QGroundControl.airspaceManager.flightPlan.loadingFlightList && _flightList.count > 0 && tableView.currentRow >= 0
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
                    Calendar {
                        id: fromPicker
                        anchors.centerIn: parent
                        visible: false;
                        onClicked: {
                            visible = false;
                        }
                    }
                    Calendar {
                        id: toPicker
                        anchors.centerIn: parent
                        visible: false;
                        minimumDate: fromPicker.selectedDate
                        onClicked: {
                            visible = false;
                        }
                    }
                }
            }
        }
    }
}
