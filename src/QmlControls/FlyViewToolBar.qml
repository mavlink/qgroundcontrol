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
    property real   _topControlHeight:  _root.height * 0.74
    property var    _batterySettings:   QGroundControl.settingsManager.batteryIndicatorSettings
    property int    _batteryDisplayMode: _batterySettings ? _batterySettings.valueDisplay.rawValue : 0
    property bool   _showBatteryTopIndicator: _activeVehicle && _activeVehicle.batteries && _activeVehicle.batteries.count > 0
    property bool   _showGpsTopIndicator: _activeVehicle !== null && _activeVehicle !== undefined
    property bool   _showRightBranding: false

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
        var battery = firstBattery(vehicle)
        if (battery) {
            if (battery && !isNaN(battery.percentRemaining.rawValue)) {
                return battery.percentRemaining.valueString + battery.percentRemaining.units
            }
        }
        return qsTr("N/A")
    }

    function firstBattery(vehicle) {
        if (vehicle && vehicle.batteries && vehicle.batteries.count > 0) {
            return vehicle.batteries.get(0)
        }
        return null
    }

    function batteryVoltageText(vehicle) {
        var battery = firstBattery(vehicle)
        if (battery && !isNaN(battery.voltage.rawValue)) {
            return battery.voltage.valueString + battery.voltage.units
        }
        return qsTr("N/A")
    }

    function batteryCurrentText(vehicle) {
        var battery = firstBattery(vehicle)
        if (battery && battery.current && !isNaN(battery.current.rawValue)) {
            return battery.current.valueString + " " + battery.current.units
        }
        return qsTr("N/A")
    }

    function batteryPrimaryText(vehicle) {
        return _batteryDisplayMode === 1 ? batteryVoltageText(vehicle) : batteryPercentText(vehicle)
    }

    function batterySecondaryText(vehicle) {
        return _batteryDisplayMode === 2 ? batteryVoltageText(vehicle) : ""
    }

    function batteryIconColor(vehicle) {
        var battery = firstBattery(vehicle)
        if (!battery || isNaN(battery.percentRemaining.rawValue)) {
            return qgcPal.buttonText
        }
        if (battery.percentRemaining.rawValue <= _batterySettings.threshold2.rawValue) {
            return qgcPal.colorRed
        }
        if (battery.percentRemaining.rawValue <= _batterySettings.threshold1.rawValue) {
            return qgcPal.colorYellow
        }
        return qgcPal.colorGreen
    }

    function batteryDetailText(vehicle) {
        var battery = firstBattery(vehicle)
        if (battery) {
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

    function gpsAvailable(vehicle) {
        return vehicle && vehicle.gps
    }

    function gpsSatelliteText(vehicle) {
        if (!gpsAvailable(vehicle) || isNaN(vehicle.gps.count.value)) {
            return ""
        }
        return vehicle.gps.count.valueString
    }

    function gpsHdopText(vehicle) {
        if (!gpsAvailable(vehicle) || isNaN(vehicle.gps.hdop.value)) {
            return ""
        }
        return vehicle.gps.hdop.value.toFixed(1)
    }

    function gpsText(vehicle) {
        if (!gpsAvailable(vehicle)) {
            return qsTr("N/A")
        }
        return gpsSatelliteText(vehicle) + (gpsHdopText(vehicle) === "" ? "" : "  " + gpsHdopText(vehicle))
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
        tintColor:              Qt.rgba(0.045, 0.048, 0.052, 0.68)
        sheenColor:             "transparent"
    }

    component BarDivider: Rectangle {
        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  1
        Layout.preferredHeight: parent ? parent.height * 0.62 : ScreenTools.defaultFontPixelHeight * 2
        width:                  1
        height:                 parent ? parent.height * 0.62 : ScreenTools.defaultFontPixelHeight * 2
        color:                  Qt.rgba(0.82, 0.90, 0.95, 0.18)
    }

    component IconSlot: Rectangle {
        id: iconSlot
        property string iconSource: ""
        property string tooltipText: ""
        property color  iconColor: qgcPal.text
        property real   slotSize:  _root._topControlHeight
        signal clicked()

        implicitWidth:          slotSize
        implicitHeight:         slotSize
        width:                  slotSize
        height:                 slotSize
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
            width:              parent.width * 0.56
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
        property real   minimumWidth: ScreenTools.defaultFontPixelWidth * 9
        property real   _textWidth: Math.max(labelText.visible ? labelText.implicitWidth : 0,
                                             valueText.implicitWidth)

        Layout.alignment:       Qt.AlignVCenter
        Layout.preferredWidth:  Math.max(minimumWidth, _textWidth + _root._segmentPadding * 1.60)
        Layout.fillHeight:      true
        implicitWidth:          Layout.preferredWidth

        Column {
            id:                 textColumn
            anchors.centerIn:   parent
            width:              Math.min(infoSegment._textWidth, Math.max(0, parent.width - _root._segmentPadding * 0.90))
            spacing:            -ScreenTools.defaultFontPixelHeight * 0.08

            QGCLabel {
                id:                 labelText
                width:              parent.width
                text:               infoSegment.label
                color:              qgcPal.buttonText
                font.pointSize:     ScreenTools.smallFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:  Text.AlignVCenter
                elide:              Text.ElideRight
                visible:            text !== ""
            }

            QGCLabel {
                id:                 valueText
                width:              parent.width
                text:               infoSegment.value
                color:              qgcPal.text
                font.bold:          true
                font.pointSize:     ScreenTools.defaultFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:  Text.AlignVCenter
                elide:              Text.ElideRight
            }
        }
    }

    component TopBarIndicator: Item {
        id: indicator
        property string iconSource: ""
        property color  iconColor: qgcPal.buttonText
        property string primaryText: ""
        property string secondaryText: ""
        property var    drawerComponent: null
        property real   _iconSize: _root.height * 0.56
        property real   _textWidth: Math.max(primaryLabel.visible ? primaryLabel.implicitWidth : 0,
                                             secondaryLabel.visible ? secondaryLabel.implicitWidth : 0)
        property real   _textGap: _textWidth > 0 ? ScreenTools.defaultFontPixelWidth * 0.42 : 0

        implicitWidth:          _iconSize + _textGap + _textWidth
        width:                  visible ? implicitWidth : 0
        height:                 parent ? parent.height : _root.height

        QGCColoredImage {
            id:                 indicatorIcon
            anchors.left:       parent.left
            anchors.verticalCenter: parent.verticalCenter
            width:              indicator._iconSize
            height:             width
            source:             indicator.iconSource
            color:              indicator.iconColor
            sourceSize.width:   width
            fillMode:           Image.PreserveAspectFit
        }

        Column {
            anchors.left:       indicatorIcon.right
            anchors.leftMargin: indicator._textGap
            anchors.verticalCenter: parent.verticalCenter
            width:              indicator._textWidth
            spacing:            -ScreenTools.defaultFontPixelHeight * 0.08
            visible:            indicator._textWidth > 0

            QGCLabel {
                id:                 primaryLabel
                width:              parent.width
                text:               indicator.primaryText
                color:              qgcPal.text
                font.pointSize:     ScreenTools.defaultFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:  Text.AlignVCenter
                visible:            text !== ""
            }

            QGCLabel {
                id:                 secondaryLabel
                width:              parent.width
                text:               indicator.secondaryText
                color:              qgcPal.text
                font.pointSize:     ScreenTools.defaultFontPointSize
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment:  Text.AlignVCenter
                visible:            text !== ""
            }
        }

        QGCMouseArea {
            anchors.fill:   parent
            enabled:        indicator.drawerComponent !== null
            onClicked:      mainWindow.showIndicatorDrawer(indicator.drawerComponent, indicator)
        }
    }

    Component {
        id: batteryTopIndicatorPage

        ToolIndicatorPage {
            showExpand:         false
            waitForParameters:  false
            contentComponent:   batteryTopIndicatorContent
        }
    }

    Component {
        id: batteryTopIndicatorContent

        SettingsGroupLayout {
            heading:        qsTr("Battery Status")
            contentSpacing: 0
            showDividers:   false

            LabelledLabel {
                label:      qsTr("Remaining")
                labelText:  batteryPercentText(_activeVehicle)
            }

            LabelledLabel {
                label:      qsTr("Voltage")
                labelText:  batteryVoltageText(_activeVehicle)
            }

            LabelledLabel {
                label:      qsTr("Current")
                labelText:  batteryCurrentText(_activeVehicle)
            }
        }
    }

    Component {
        id: gpsTopIndicatorPage

        GPSIndicatorPage { }
    }

    RowLayout {
        id:                    topBarLayout
        anchors.fill:          parent
        anchors.leftMargin:    _barMargin
        anchors.rightMargin:   _barMargin
        spacing:               ScreenTools.defaultFontPixelWidth * 0.72

        Item {
            id:                     brandBlock
            readonly property real  _symbolSize: _root.height * 0.62
            readonly property real  _wordmarkHeight: _root.height * 0.43
            readonly property real  _wordmarkAspect: 727 / 192

            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredWidth:  brandRow.implicitWidth
            Layout.minimumWidth:    brandRow.implicitWidth
            Layout.fillHeight:      true

            RowLayout {
                id:                 brandRow
                anchors.left:       parent.left
                anchors.verticalCenter: parent.verticalCenter
                spacing:            ScreenTools.defaultFontPixelWidth * 0.70

                Image {
                    Layout.alignment:       Qt.AlignVCenter
                    Layout.preferredWidth:  brandBlock._symbolSize
                    Layout.preferredHeight: brandBlock._symbolSize
                    source:                 "/res/brand-symbol.png"
                    sourceSize.width:       width
                    sourceSize.height:      height
                    fillMode:               Image.PreserveAspectFit
                    mipmap:                 true
                }

                Item {
                    Layout.alignment:       Qt.AlignVCenter
                    Layout.preferredWidth:  brandBlock._wordmarkHeight * brandBlock._wordmarkAspect
                    Layout.preferredHeight: brandBlock._symbolSize

                    QGCColoredImage {
                        anchors.horizontalCenter: parent.horizontalCenter
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: ScreenTools.defaultFontPixelHeight * 0.10
                        width:                  parent.width
                        height:                 brandBlock._wordmarkHeight
                        source:                 "/res/brand-wordmark.png"
                        sourceSize.width:       width
                        sourceSize.height:      height
                        color:                  qgcPal.text
                        fillMode:               Image.PreserveAspectFit
                    }
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
            Layout.preferredHeight: _root._topControlHeight
            heightFactor:           0
            text:                   qsTr("Disconnect")
            onClicked:              _activeVehicle.closeVehicle()
            visible:                _activeVehicle && _communicationLost
        }

        BarDivider {
            visible: _showBatteryTopIndicator || _showGpsTopIndicator
        }

        Row {
            id:                     topBarIndicators
            Layout.alignment:       Qt.AlignVCenter
            Layout.preferredWidth:  visible ? implicitWidth : 0
            Layout.fillHeight:      true
            spacing:                _showBatteryTopIndicator && _showGpsTopIndicator ? ScreenTools.defaultFontPixelWidth * 1.05 : 0
            visible:                _showBatteryTopIndicator || _showGpsTopIndicator

            TopBarIndicator {
                id:             batteryTopBarIndicator
                visible:        _showBatteryTopIndicator
                iconSource:     "/qmlimages/Battery.svg"
                iconColor:      batteryIconColor(_activeVehicle)
                primaryText:    batteryPrimaryText(_activeVehicle)
                secondaryText:  batterySecondaryText(_activeVehicle)
                drawerComponent: batteryTopIndicatorPage
            }

            TopBarIndicator {
                id:             gpsTopBarIndicator
                visible:        _showGpsTopIndicator
                iconSource:     "/qmlimages/Gps.svg"
                iconColor:      qgcPal.buttonText
                primaryText:    gpsSatelliteText(_activeVehicle)
                secondaryText:  gpsHdopText(_activeVehicle)
                drawerComponent: gpsTopIndicatorPage
            }
        }

        BarDivider {
            visible: _showBatteryTopIndicator || _showGpsTopIndicator
        }

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

        Item {
            id:                 rightActionRow
            property bool       _showAnalyzeAction: QGroundControl.corePlugin.showAdvancedUI
            property bool       _showAppSettingsAction: !QGroundControl.corePlugin.options.combineSettingsAndSetup
            property int        _visibleActionCount: (_showAnalyzeAction ? 1 : 0) +
                                                     1 +
                                                     1 +
                                                     (_showAppSettingsAction ? 1 : 0)
            property real       _slotSize: _root.height * 0.74
            property real       _rowWidth: _visibleActionCount > 0 ?
                                             (_visibleActionCount * _slotSize +
                                              Math.max(0, _visibleActionCount - 1) * actionRow.spacing) : 0

            Layout.alignment:   Qt.AlignVCenter
            Layout.minimumWidth: _rowWidth
            Layout.preferredWidth: _rowWidth
            Layout.fillHeight:  true

            Row {
                id:                 actionRow
                anchors.right:      parent.right
                anchors.verticalCenter: parent.verticalCenter
                height:             parent.height
                spacing:            ScreenTools.defaultFontPixelWidth * 0.42

                IconSlot {
                    id:             analyzeAction
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource:     "/qmlimages/Analyze.svg"
                    tooltipText:    qsTr("Analyze Tools")
                    iconColor:      qgcPal.buttonText
                    visible:        rightActionRow._showAnalyzeAction
                    onClicked:      _root.openAnalyzeTool()
                }

                IconSlot {
                    id:             vehicleConfigAction
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource:     "/qmlimages/Gears.svg"
                    tooltipText:    qsTr("Vehicle Configuration")
                    iconColor:      qgcPal.buttonText
                    onClicked:      _root.openVehicleConfig()
                }

                IconSlot {
                    id:             vehicleMessagesAction
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource:     "/res/VehicleMessages.png"
                    tooltipText:    qsTr("Vehicle Messages")
                    iconColor:      messageIconColor()
                    onClicked:      mainStatusIndicator.dropMainStatusIndicator()
                }

                IconSlot {
                    id:             appSettingsAction
                    anchors.verticalCenter: parent.verticalCenter
                    iconSource:     "/res/gear-white.svg"
                    tooltipText:    qsTr("Application Settings")
                    iconColor:      qgcPal.buttonText
                    visible:        rightActionRow._showAppSettingsAction
                    onClicked:      _root.openSettingsTool()
                }
            }
        }

        BarDivider {
            visible: _showRightBranding && brandHolder.visible
        }

        Item {
            id:                     brandHolder
            Layout.preferredWidth:  visible ? ScreenTools.defaultFontPixelWidth * 10 : 0
            Layout.fillHeight:      true
            visible:                _showRightBranding && _brandSource !== ""

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
