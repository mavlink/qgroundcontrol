/****************************************************************************
 *
 * (c) 2009-2020 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import QtQuick.Dialogs

import QGroundControl
import QGroundControl.Controls
import QGroundControl.Palette
import QGroundControl.MultiVehicleManager
import QGroundControl.ScreenTools
import QGroundControl.Controllers

Rectangle {
    id:     _root
    width:  parent.width
    height: Math.max(ScreenTools.minTouchPixels * 0.72, ScreenTools.defaultFontPixelHeight * 2.02)
    color:  "transparent"
    clip:   true

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    backdropSourceItem: null
    property bool   _communicationLost: _activeVehicle ? _activeVehicle.vehicleLinkManager.communicationLost : false
    property color  _mainStatusBGColor: qgcPal.brandingPurple
    property real   _barMargin:         ScreenTools.defaultFontPixelWidth * 0.72
    property real   _segmentPadding:    ScreenTools.defaultFontPixelWidth * 0.75

    function dropMainStatusIndicatorTool() {
        mainStatusIndicator.dropMainStatusIndicator()
    }

    function openAnalyzeTool() {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.closeIndicatorDrawer()
            mainWindow.showAnalyzeTool()
        }
    }

    function openVehicleConfig() {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.closeIndicatorDrawer()
            mainWindow.showVehicleConfig()
        }
    }

    function openSettingsTool() {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.closeIndicatorDrawer()
            mainWindow.showSettingsTool()
        }
    }

    function factText(fact, fallbackText) {
        if (!fact || fact.valueString === undefined) {
            return fallbackText
        }
        return fact.valueString + (fact.units && fact.units !== "" ? " " + fact.units : "")
    }

    function batteryPercentText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
        }
        return qsTr("N/A")
    }

    function batteryDetailText(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            var battery = vehicle.batteries.get(0)
            var parts = []
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                parts.push(battery.percentRemaining.valueString + battery.percentRemaining.units)
            }
            if (battery && !isNaN(battery.voltage.rawValue)) {
                parts.push(battery.voltage.valueString + " " + battery.voltage.units)
            }
            if (battery && battery.current && !isNaN(battery.current.rawValue)) {
                parts.push(battery.current.valueString + " " + battery.current.units)
            }
            if (parts.length > 0) {
                return parts.join("   ")
            }
        }
        return qsTr("N/A")
    }

    function gpsText(vehicle) {
        if (!vehicle || !vehicle.gps || isNaN(vehicle.gps.count.value)) {
            return qsTr("N/A")
        }
        return vehicle.gps.count.valueString + (isNaN(vehicle.gps.hdop.value) ? "" : "  " + vehicle.gps.hdop.value.toFixed(1))
    }

    function vehicleCountText() {
        return QGroundControl.multiVehicleManager.vehicles.count > 0 ? QGroundControl.multiVehicleManager.vehicles.count.toString() : qsTr("N/A")
    }

    function messageIconColor() {
        if (_activeVehicle) {
            if (_activeVehicle.messageTypeError) {
                return qgcPal.colorRed
            }
            if (_activeVehicle.messageTypeWarning) {
                return qgcPal.colorOrange
            }
        }
        return qgcPal.buttonText
    }

    function statusText() {
        if (!_activeVehicle) {
            _mainStatusBGColor = qgcPal.brandingPurple
            return qsTr("Disconnected")
        }
        if (_communicationLost) {
            _mainStatusBGColor = qgcPal.colorRed
            return qsTr("Comms Lost")
        }
        if (_activeVehicle.armed) {
            _mainStatusBGColor = qgcPal.colorGreen
            if (_activeVehicle.flying) {
                return qsTr("Flying")
            }
            if (_activeVehicle.landing) {
                return qsTr("Landing")
            }
            return qsTr("Armed")
        }
        if (_activeVehicle.readyToFlyAvailable && !_activeVehicle.readyToFly) {
            _mainStatusBGColor = qgcPal.colorYellow
            return qsTr("Not Ready")
        }
        _mainStatusBGColor = qgcPal.colorGreen
        return qsTr("Ready")
    }

    QGCPalette { id: qgcPal }

    GlassBackdrop {
        anchors.fill:           parent
        sourceItem:             _root.backdropSourceItem
        backdropBlurEnabled:    true
        sampleAtItemPosition:   false
        sampleX:                0
        sampleY:                0
        sourceScale:            0.34
        blurAmount:             0.94
        blurMax:                46
        sourceBrightness:       -0.01
        sourceSaturation:       0.62
        tintColor:              Qt.rgba(0.045, 0.048, 0.052, 0.80)
        sheenColor:             "transparent"
    }

    component BarDivider: Rectangle {
        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  1
        Layout.preferredHeight: parent ? parent.height * 0.62 : ScreenTools.defaultFontPixelHeight * 2
        color:                  Qt.rgba(0.82, 0.90, 0.95, 0.18)
    }

    component IconSlot: Rectangle {
        id: iconSlot
        property string iconSource: ""
        property string tooltipText: ""
        property color  iconColor: qgcPal.text
        property real   slotSize:  _root.height * 0.62
        signal clicked()

        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  slotSize
        Layout.preferredHeight: slotSize
        radius:                 Math.round(slotSize * 0.16)
        color:                  mouseArea.pressed ? Qt.rgba(0.135, 0.140, 0.150, 0.48) :
                                (mouseArea.containsMouse ? Qt.rgba(0.175, 0.180, 0.190, 0.32) : "transparent")
        border.color:           (mouseArea.pressed || mouseArea.containsMouse) ? Qt.rgba(0.82, 0.90, 0.95, 0.18) : "transparent"
        border.width:           (mouseArea.pressed || mouseArea.containsMouse) ? 1 : 0

        ToolTip.visible:        mouseArea.containsMouse && tooltipText !== ""
        ToolTip.text:           tooltipText
        ToolTip.delay:          450

        QGCColoredImage {
            anchors.centerIn:   parent
            width:              parent.width * 0.48
            height:             width
            source:             iconSlot.iconSource
            color:              iconSlot.iconColor
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
        }

        QGCMouseArea {
            id:             mouseArea
            anchors.fill:   parent
            hoverEnabled:   !ScreenTools.isMobile
            onClicked:      iconSlot.clicked()
        }
    }

    component InfoSegment: Item {
        id: infoSegment
        property string label: ""
        property string value: ""
        property string iconSource: ""
        property color  iconColor: qgcPal.text
        property real   minimumWidth: ScreenTools.defaultFontPixelWidth * 9

        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  Math.max(minimumWidth, contentRow.implicitWidth + _root._segmentPadding * 1.25)
        Layout.fillHeight:      true

        RowLayout {
            id:                 contentRow
            anchors.fill:       parent
            anchors.leftMargin: _root._segmentPadding * 0.45
            anchors.rightMargin:_root._segmentPadding * 0.45
            spacing:            ScreenTools.defaultFontPixelWidth * 0.45

            QGCColoredImage {
                Layout.alignment:   Qt.AlignVCenter
                width:              ScreenTools.defaultFontPixelHeight * 1.10
                height:             width
                source:             infoSegment.iconSource
                color:              infoSegment.iconColor
                sourceSize.width:   width
                fillMode:           Image.PreserveAspectFit
                visible:            source !== ""
            }

            ColumnLayout {
                Layout.alignment:   Qt.AlignVCenter
                Layout.fillWidth:   true
                spacing:            -ScreenTools.defaultFontPixelHeight * 0.08

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               infoSegment.label
                    color:              qgcPal.buttonText
                    font.pointSize:     ScreenTools.smallFontPointSize
                    elide:              Text.ElideRight
                    visible:            text !== ""
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               infoSegment.value
                    color:              qgcPal.text
                    font.bold:          true
                    font.pointSize:     ScreenTools.defaultFontPointSize
                    elide:              Text.ElideRight
                }
            }
        }
    }

    RowLayout {
        id:                    topBarLayout
        anchors.fill:          parent
        anchors.leftMargin:    _barMargin
        anchors.rightMargin:   _barMargin
        spacing:               ScreenTools.defaultFontPixelWidth * 0.72

        Item {
            id:                     brandBlock
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredWidth:  Math.max(ScreenTools.defaultFontPixelWidth * 24.5, brandRow.implicitWidth)
            Layout.fillHeight:      true

            RowLayout {
                id:                 brandRow
                anchors.fill:       parent
                spacing:            ScreenTools.defaultFontPixelWidth * 0.70

                Rectangle {
                    Layout.alignment:   Qt.AlignVCenter
                    width:              _root.height * 0.62
                    height:             width
                    radius:             Math.round(width * 0.22)
                    color:              "transparent"
                    border.color:       "transparent"
                    border.width:       0

                    Image {
                        anchors.fill:       parent
                        anchors.margins:    parent.width * 0.18
                        source:             "/res/QGCLogoFull.svg"
                        fillMode:           Image.PreserveAspectFit
                        mipmap:             true
                    }
                }

                QGCLabel {
                    Layout.fillWidth:   true
                    text:               qsTr("QGroundControl")
                    color:              qgcPal.text
                    font.pointSize:     ScreenTools.defaultFontPointSize
                    font.bold:          true
                    elide:              Text.ElideRight
                }
            }

            // Tool navigation is exposed as real top/left actions below; the brand is no longer a hidden menu trigger.
        }

        BarDivider { }

        MainStatusIndicator {
            id:                     mainStatusIndicator
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredHeight: topBarLayout.height
        }

        QGCButton {
            id:                     disconnectButton
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredHeight: Math.max(ScreenTools.defaultFontPixelHeight * 1.6, _root.height * 0.52)
            text:                   qsTr("Disconnect")
            onClicked:              _activeVehicle.closeVehicle()
            visible:                _activeVehicle && _communicationLost
        }

        BarDivider { }

        QGCFlickable {
            id:                     nativeIndicatorFlickable
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredWidth:  Math.min(Math.max(ScreenTools.defaultFontPixelWidth * 4.8,
                                                       nativeToolIndicators.childrenRect.width),
                                             ScreenTools.defaultFontPixelWidth * 16.0)
            Layout.fillHeight:      true
            contentWidth:           nativeToolIndicators.childrenRect.width
            contentHeight:          height
            flickableDirection:     Flickable.HorizontalFlick
            boundsBehavior:         Flickable.StopAtBounds
            interactive:            contentWidth > width
            clip:                   true

            FlyViewToolBarIndicators {
                id: nativeToolIndicators
                hiddenIndicatorNames: [
                    "FlightModeIndicator.qml",
                    "APMFlightModeIndicator.qml",
                    "PX4FlightModeIndicator.qml",
                    "MultiVehicleSelector.qml"
                ]
            }
        }

        BarDivider { }

        InfoSegment {
            label:        qsTr("Altitude")
            value:        factText(_activeVehicle ? _activeVehicle.altitudeRelative : null, qsTr("N/A"))
            minimumWidth: ScreenTools.defaultFontPixelWidth * 10.2
        }

        BarDivider { }

        InfoSegment {
            label:        qsTr("Ground Speed")
            value:        factText(_activeVehicle ? _activeVehicle.groundSpeed : null, qsTr("N/A"))
            minimumWidth: ScreenTools.defaultFontPixelWidth * 12.2
        }

        BarDivider { }

        InfoSegment {
            label:        qsTr("Flight Time")
            value:        factText(_activeVehicle ? _activeVehicle.flightTime : null, qsTr("00:00:00"))
            minimumWidth: ScreenTools.defaultFontPixelWidth * 12.2
        }

        BarDivider { }

        Item {
            Layout.fillWidth: true
        }

        RowLayout {
            Layout.alignment:   Qt.AlignVCenter
            Layout.fillHeight:  true
            spacing:            ScreenTools.defaultFontPixelWidth * 0.42

            IconSlot {
                iconSource:     "/qmlimages/Analyze.svg"
                tooltipText:    qsTr("Analyze Tools")
                iconColor:      qgcPal.buttonText
                visible:        QGroundControl.corePlugin.showAdvancedUI
                onClicked:      _root.openAnalyzeTool()
            }

            IconSlot {
                iconSource:     "/qmlimages/Gears.svg"
                tooltipText:    qsTr("Vehicle Configuration")
                iconColor:      qgcPal.buttonText
                onClicked:      _root.openVehicleConfig()
            }

            IconSlot {
                iconSource:     "/res/VehicleMessages.png"
                tooltipText:    qsTr("Vehicle Messages")
                iconColor:      messageIconColor()
                onClicked:      mainStatusIndicator.dropMainStatusIndicator()
            }

            IconSlot {
                iconSource:     "/res/gear-white.svg"
                tooltipText:    qsTr("Application Settings")
                iconColor:      qgcPal.buttonText
                visible:        !QGroundControl.corePlugin.options.combineSettingsAndSetup
                onClicked:      _root.openSettingsTool()
            }
        }

        BarDivider { }

        Item {
            id:                     brandHolder
            Layout.preferredWidth:  _brandSource !== "" ? ScreenTools.defaultFontPixelWidth * 10 : 0
            Layout.fillHeight:      true
            visible:                _brandSource !== ""

            property bool   _outdoorPalette:        qgcPal.globalTheme === QGCPalette.Light
            property bool   _corePluginBranding:    QGroundControl.corePlugin.brandImageIndoor.length != 0
            property string _userBrandImageIndoor:  QGroundControl.settingsManager.brandImageSettings.userBrandImageIndoor.value
            property string _userBrandImageOutdoor: QGroundControl.settingsManager.brandImageSettings.userBrandImageOutdoor.value
            property bool   _userBrandingIndoor:    QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageIndoor.length != 0
            property bool   _userBrandingOutdoor:   QGroundControl.settingsManager.brandImageSettings.visible && _userBrandImageOutdoor.length != 0
            property string _brandImageIndoor:      brandImageIndoor()
            property string _brandImageOutdoor:     brandImageOutdoor()
            property string _brandSource:           _activeVehicle && !_communicationLost ? (_outdoorPalette ? _brandImageOutdoor : _brandImageIndoor) : ""

            Image {
                anchors.fill:       parent
                anchors.margins:    ScreenTools.defaultFontPixelHeight * 0.28
                fillMode:           Image.PreserveAspectFit
                horizontalAlignment: Image.AlignRight
                source:             brandHolder._brandSource
                mipmap:             true
            }

            function brandImageIndoor() {
                if (_userBrandingIndoor) {
                    return _userBrandImageIndoor
                } else if (_userBrandingOutdoor) {
                    return _userBrandImageOutdoor
                } else if (_corePluginBranding) {
                    return QGroundControl.corePlugin.brandImageIndoor
                } else {
                    return _activeVehicle ? _activeVehicle.brandImageIndoor : ""
                }
            }

            function brandImageOutdoor() {
                if (_userBrandingOutdoor) {
                    return _userBrandImageOutdoor
                } else if (_userBrandingIndoor) {
                    return _userBrandImageIndoor
                } else if (_corePluginBranding) {
                    return QGroundControl.corePlugin.brandImageOutdoor
                } else {
                    return _activeVehicle ? _activeVehicle.brandImageOutdoor : ""
                }
            }
        }
    }

    Rectangle {
        id:             largeProgressBar
        anchors.fill:   parent
        color:          Qt.rgba(0.045, 0.048, 0.052, 0.92)
        visible:        _showLargeProgress

        property bool _initialDownloadComplete: _activeVehicle ? _activeVehicle.initialConnectComplete : true
        property bool _userHide:                false
        property bool _showLargeProgress:       !_initialDownloadComplete && !_userHide && qgcPal.globalTheme === QGCPalette.Light

        Connections {
            target: QGroundControl.multiVehicleManager
            function onActiveVehicleChanged(activeVehicle) { largeProgressBar._userHide = false }
        }

        Rectangle {
            anchors.top:    parent.top
            anchors.bottom: parent.bottom
            width:          _activeVehicle ? _activeVehicle.loadProgress * parent.width : 0
            color:          qgcPal.primaryButton
        }

        QGCLabel {
            anchors.centerIn:   parent
            text:               qsTr("Downloading")
            font.pointSize:     ScreenTools.largeFontPointSize
            font.bold:          true
        }

        MouseArea {
            anchors.fill:   parent
            onClicked:      largeProgressBar._userHide = true
        }
    }
}
