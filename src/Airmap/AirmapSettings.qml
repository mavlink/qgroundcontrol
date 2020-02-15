/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
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
import QGroundControl.Airspace              1.0
import QGroundControl.Controllers           1.0
import QGroundControl.Controls              1.0
import QGroundControl.FactControls          1.0
import QGroundControl.FactSystem            1.0
import QGroundControl.FlightMap             1.0
import QGroundControl.MultiVehicleManager   1.0
import QGroundControl.Palette               1.0
import QGroundControl.ScreenTools           1.0
import QGroundControl.SettingsManager       1.0

Item {
    id:                 _root
    anchors.fill:       parent
    anchors.margins:    ScreenTools.defaultFontPixelWidth

    property real _labelWidth:                  ScreenTools.defaultFontPixelWidth * 20
    property real _editFieldWidth:              ScreenTools.defaultFontPixelWidth * 20
    property real _buttonWidth:                 ScreenTools.defaultFontPixelWidth * 18
    property real _panelWidth:                  _root.width * _internalWidthRatio
    property Fact _enableAirMapFact:            QGroundControl.settingsManager.airMapSettings.enableAirMap
    property bool _airMapEnabled:               _enableAirMapFact.rawValue
    property var  _authStatus:                  QGroundControl.airspaceManager.authStatus

    readonly property real _internalWidthRatio:          0.8

    QGCFlickable {
        clip:               true
        anchors.fill:       parent
        contentHeight:      settingsColumn.height
        contentWidth:       settingsColumn.width
        Column {
            id:                 settingsColumn
            width:              _root.width
            spacing:            ScreenTools.defaultFontPixelHeight * 0.5
            anchors.margins:    ScreenTools.defaultFontPixelWidth
            //-----------------------------------------------------------------
            //-- General
            Item {
                width:                      _panelWidth
                height:                     generalLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                QGCLabel {
                    id:             generalLabel
                    text:           qsTr("General")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:                     generalRow.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:                      _panelWidth
                color:                      qgcPal.windowShade
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                Row {
                    id:                 generalRow
                    spacing:            ScreenTools.defaultFontPixelWidth * 4
                    anchors.centerIn:   parent
                    Column {
                        spacing:        ScreenTools.defaultFontPixelWidth
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
                        FactCheckBox {
                            text:       qsTr("Show Airspace on Map (Experimental)")
                            fact:       _enableAirspaceFact
                            visible:    _enableAirspaceFact.visible
                            enabled:    _airMapEnabled
                            property Fact _enableAirspaceFact: QGroundControl.settingsManager.airMapSettings.enableAirspace
                        }
                    }
                    QGCButton {
                        text:           qsTr("Clear Saved Answers")
                        enabled:        _enableAirMapFact.rawValue
                        onClicked:      clearDialog.open()
                        anchors.verticalCenter: parent.verticalCenter
                        MessageDialog {
                            id:                 clearDialog
                            visible:            false
                            icon:               StandardIcon.Warning
                            standardButtons:    StandardButton.Yes | StandardButton.No
                            title:              qsTr("Clear Saved Answers")
                            text:               qsTr("All saved ruleset answers will be cleared. Is this really what you want?")
                            onYes: {
                                QGroundControl.airspaceManager.ruleSets.clearAllFeatures()
                                clearDialog.close()
                            }
                            onNo: {
                                clearDialog.close()
                            }
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Connection Status
            Item {
                width:                      _panelWidth
                height:                     statusLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    QGroundControl.settingsManager.appSettings.visible && _airMapEnabled
                QGCLabel {
                    id:                     statusLabel
                    text:                   qsTr("Connection Status")
                    font.family:            ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:                     statusCol.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:                      _panelWidth
                color:                      qgcPal.windowShade
                visible:                    QGroundControl.settingsManager.appSettings.visible && _airMapEnabled
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                Column {
                    id:                     statusCol
                    spacing:                ScreenTools.defaultFontPixelHeight * 0.5
                    width:                  parent.width
                    anchors.centerIn:       parent
                    QGCLabel {
                        text:                       QGroundControl.airspaceManager.connected ? qsTr("Connected") : qsTr("Not Connected")
                        color:                      QGroundControl.airspaceManager.connected ? qgcPal.colorGreen : qgcPal.colorRed
                        anchors.horizontalCenter:   parent.horizontalCenter
                    }
                    QGCLabel {
                        text:                       QGroundControl.airspaceManager.connectStatus
                        visible:                    QGroundControl.airspaceManager.connectStatus != ""
                        wrapMode:                   Text.WordWrap
                        horizontalAlignment:        Text.AlignHCenter
                        width:                      parent.width * 0.8
                        anchors.horizontalCenter:   parent.horizontalCenter
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
                    columns:            3
                    columnSpacing:      ScreenTools.defaultFontPixelWidth
                    rowSpacing:         ScreenTools.defaultFontPixelHeight * 0.25
                    anchors.centerIn:   parent
                    QGCLabel        { text: qsTr("User Name:") }
                    FactTextField {
                        fact:           _usernameFact
                        width:          _editFieldWidth
                        enabled:        _airMapEnabled
                        visible:        _usernameFact.visible
                        Layout.fillWidth:    true
                        Layout.minimumWidth: _editFieldWidth
                        property Fact _usernameFact: QGroundControl.settingsManager.airMapSettings.userName
                    }
                    QGCLabel {
                        text: {
                            if(!QGroundControl.airspaceManager.connected)
                                return qsTr("Not Connected")
                            switch(_authStatus) {
                            case AirspaceManager.Unknown:
                                return ""
                            case AirspaceManager.Anonymous:
                                return qsTr("Anonymous")
                            case AirspaceManager.Authenticated:
                                return qsTr("Authenticated")
                            default:
                                return qsTr("Authentication Error")
                            }
                        }
                        Layout.rowSpan:     2
                        Layout.alignment:   Qt.AlignVCenter
                    }
                    QGCLabel { text: qsTr("Password:") }
                    FactTextField {
                        fact:           _passwordFact
                        width:          _editFieldWidth
                        enabled:        _airMapEnabled
                        visible:        _passwordFact.visible
                        echoMode:       TextInput.Password
                        Layout.fillWidth:    true
                        Layout.minimumWidth: _editFieldWidth
                        property Fact _passwordFact: QGroundControl.settingsManager.airMapSettings.password
                    }
                    Item {
                        width:  1
                        height: 1
                    }
                    Item {
                        width:  1
                        height: 1
                        Layout.columnSpan: 3
                    }
                    QGCLabel {
                        text:               qsTr("Forgot Your AirMap Password?")
                        Layout.alignment:   Qt.AlignHCenter
                        Layout.columnSpan:  3
                    }
                    Item {
                        width:  1
                        height: 1
                        Layout.columnSpan:  3
                    }
                    QGCButton {
                        text:               qsTr("Register for an AirMap Account")
                        Layout.alignment:   Qt.AlignHCenter
                        Layout.columnSpan:  3
                        enabled:            _airMapEnabled
                        onClicked: {
                            Qt.openUrlExternally("https://www.airmap.com");
                        }
                    }
                }
            }
            //-----------------------------------------------------------------
            //-- Pilot Profile
            Item {
                //-- Disabled for now
                width:                      _panelWidth
                height:                     profileLabel.height
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    false // QGroundControl.settingsManager.appSettings.visible
                QGCLabel {
                    id:             profileLabel
                    text:           qsTr("Pilot Profile (WIP)")
                    font.family:    ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                //-- Disabled for now
                height:                     profileGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:                      _panelWidth
                color:                      qgcPal.windowShade
                visible:                    false // QGroundControl.settingsManager.appSettings.visible
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
                visible:                    QGroundControl.settingsManager.airMapSettings.usePersonalApiKey.visible
                QGCLabel {
                    id:                     licenseLabel
                    text:                   qsTr("License")
                    font.family:            ScreenTools.demiboldFontFamily
                }
            }
            Rectangle {
                height:                     licenseGrid.height + (ScreenTools.defaultFontPixelHeight * 2)
                width:                      _panelWidth
                color:                      qgcPal.windowShade
                anchors.margins:            ScreenTools.defaultFontPixelWidth
                anchors.horizontalCenter:   parent.horizontalCenter
                visible:                    QGroundControl.settingsManager.airMapSettings.usePersonalApiKey.visible
                GridLayout {
                    id:                     licenseGrid
                    columns:                2
                    columnSpacing:          ScreenTools.defaultFontPixelHeight * 2
                    rowSpacing:             ScreenTools.defaultFontPixelWidth  * 0.25
                    anchors.centerIn:       parent
                    FactCheckBox {
                        id:                 hasPrivateKey
                        text:               qsTr("Personal API Key")
                        fact:               QGroundControl.settingsManager.airMapSettings.usePersonalApiKey
                        Layout.columnSpan:  2
                    }
                    Item {
                        width:      1
                        height:     1
                        visible:    hasPrivateKey.checked
                        Layout.columnSpan:  2
                    }
                    QGCLabel        { text: qsTr("API Key:"); visible: hasPrivateKey.checked; }
                    FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.apiKey; width: _editFieldWidth * 2; visible: hasPrivateKey.checked; Layout.fillWidth: true; Layout.minimumWidth: _editFieldWidth * 2; }
                    QGCLabel        { text: qsTr("Client ID:"); visible: hasPrivateKey.checked; }
                    FactTextField   { fact: QGroundControl.settingsManager.airMapSettings.clientID; width: _editFieldWidth * 2; visible: hasPrivateKey.checked; Layout.fillWidth: true; Layout.minimumWidth: _editFieldWidth * 2;  }
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
    //---------------------------------------------------------------
    //-- Flight List
    Component {
        id:             flightList
        Rectangle {
            id:         flightListRoot
            width:      _root.width
            height:     _root.height
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
            //---------------------------------------------------------
            //-- Flight List
            RowLayout {
                anchors.fill: parent
                TableView {
                    id:                 tableView
                    model:              _flightList
                    selectionMode:      SelectionMode.SingleSelection
                    Layout.alignment:   Qt.AlignVCenter
                    Layout.fillWidth:   true
                    Layout.fillHeight:  true
                    onCurrentRowChanged: {
                        var o = _flightList.get(tableView.currentRow)
                        if(o) {
                            flightArea.path = o.boundingBox
                            map.fitViewportToMapItems()
                        }
                    }
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
                        width:  ScreenTools.defaultFontPixelWidth * 18
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
                        width:  ScreenTools.defaultFontPixelWidth * 18
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
                        title:  qsTr("Flight End")
                        width:  ScreenTools.defaultFontPixelWidth * 18
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? o.endTime : ""
                            }
                            color: tableView.currentRow === styleData.row ? qgcPal.colorBlue : "black"
                            font.family: ScreenTools.fixedFontFamily
                            font.pixelSize: ScreenTools.smallFontPointSize
                        }
                    }
                    TableViewColumn {
                        title:  qsTr("State")
                        width:  ScreenTools.defaultFontPixelWidth * 8
                        horizontalAlignment: Text.AlignHCenter
                        delegate : Text  {
                            horizontalAlignment: Text.AlignHCenter
                            text: {
                                var o = _flightList.get(styleData.row)
                                return o ? (o.active ? qsTr("Active") : qsTr("Completed")) : qsTr("Unknown")
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
                            text:           qsTr("End Selected")
                            backRadius:     4
                            heightFactor:   0.3333
                            showBorder:     true
                            width:          _buttonWidth
                            enabled: {
                                var o = _flightList.get(tableView.currentRow)
                                return o && o.active
                            }
                            anchors.horizontalCenter: parent.horizontalCenter
                            onClicked: {
                                endFlightDialog.visible = true
                            }
                            MessageDialog {
                                id:         endFlightDialog
                                visible:    false
                                icon:       StandardIcon.Warning
                                standardButtons: StandardButton.Yes | StandardButton.No
                                title:      qsTr("End Flight")
                                text:       qsTr("Confirm ending active flight?")
                                onYes: {
                                    var o = _flightList.get(tableView.currentRow)
                                    if(o) {
                                        QGroundControl.airspaceManager.flightPlan.endFlight(o.flightID)
                                    }
                                    endFlightDialog.visible = false
                                }
                                onNo: {
                                    endFlightDialog.visible = false
                                }
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
                            text:           _flightList.count > 0 ? _flightList.count + qsTr(" Flights Loaded") : qsTr("No Flights Loaded")
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
                            var fullMapName = settings.mapProvider.value + " " + settings.mapType.value
                            for (var i = 0; i < map.supportedMapTypes.length; i++) {
                                if (fullMapName === map.supportedMapTypes[i].name) {
                                    map.activeMapType = map.supportedMapTypes[i]
                                    return
                                }
                            }
                        }
                        MapPolygon {
                            id:             flightArea
                            color:          Qt.rgba(1,0,0,0.2)
                            border.color:   Qt.rgba(1,1,1,0.65)
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
