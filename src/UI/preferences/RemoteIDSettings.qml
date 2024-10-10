/****************************************************************************
 *
 * (c) 2009-2022 QGROUNDCONTROL PROJECT <http://www.qgroundcontrol.org>
 *
 * QGroundControl is licensed according to the terms in the file
 * COPYING.md in the root of the source code directory.
 *
 ****************************************************************************/

import QtQuick
import QtQuick.Controls
import QtQuick.Dialogs
import QtQuick.Layouts

import QGroundControl
import QGroundControl.FactSystem
import QGroundControl.FactControls
import QGroundControl.Controls
import QGroundControl.ScreenTools
import QGroundControl.MultiVehicleManager
import QGroundControl.Palette

Rectangle {
    id:             remoteIDRoot
    color:          qgcPal.window
    anchors.fill:   parent

    // Visual properties
    property real _margins:             ScreenTools.defaultFontPixelWidth
    property real _labelWidth:          ScreenTools.defaultFontPixelWidth * 28
    property real _valueWidth:          ScreenTools.defaultFontPixelWidth * 24
    property real _columnSpacing:       ScreenTools.defaultFontPixelHeight * 0.25
    property real _comboFieldWidth:     ScreenTools.defaultFontPixelWidth * 30
    property real _valueFieldWidth:     ScreenTools.defaultFontPixelWidth * 10
    property int  _borderWidth:         3
    // Flags visual properties
    property real   flagsWidth:         ScreenTools.defaultFontPixelWidth * 15
    property real   flagsHeight:        ScreenTools.defaultFontPixelWidth * 7
    property int    radiusFlags:        5

    // Flag to get active vehicle and active RID
    property var  _activeRID:           _activeVehicle && _activeVehicle.remoteIDManager ? _activeVehicle.remoteIDManager : null

    // Healthy connection with RID device
    property bool commsGood:            _activeVehicle && _activeVehicle.remoteIDManager ? _activeVehicle.remoteIDManager.commsGood : false

    // General properties
    property var  _activeVehicle:       QGroundControl.multiVehicleManager.activeVehicle
    property var  _offlineVehicle:      QGroundControl.multiVehicleManager.offlineEditingVehicle
    property int  _regionOperation:     QGroundControl.settingsManager.remoteIDSettings.region.value
    property int  _locationType:        QGroundControl.settingsManager.remoteIDSettings.locationType.value
    property int  _classificationType:  QGroundControl.settingsManager.remoteIDSettings.classificationType.value

    enum RegionOperation {
        FAA,
        EU
    }

    enum LocationType {
        TAKEOFF,
        LIVE,
        FIXED
    }

    enum ClassificationType {
        UNDEFINED,
        EU
    }

    // GPS properties
    property var    gcsPosition:        QGroundControl.qgcPositionManger.gcsPosition
    property real   gcsHeading:         QGroundControl.qgcPositionManger.gcsHeading
    property real   gcsHDOP:            QGroundControl.qgcPositionManger.gcsPositionHorizontalAccuracy
    property string gpsDisabled:        "Disabled"
    property string gpsUdpPort:         "UDP Port"

    QGCPalette { id: qgcPal }

    // Function to get the corresponding Self ID label depending on the Self ID Type selected
    function getSelfIdLabelText() {
        switch (selfIDComboBox.currentIndex) {
            case 0:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDFree.shortDescription
                break
            case 1:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDEmergency.shortDescription
                break
            case 2:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDExtended.shortDescription
                break
            default:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDFree.shortDescription
        }
    }

    // Function to get the corresponding Self ID fact depending on the Self ID Type selected
    function getSelfIDFact() {
        switch (selfIDComboBox.currentIndex) {
            case 0:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDFree
                break
            case 1:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDEmergency
                break
            case 2:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDExtended
                break
            default:
                return QGroundControl.settingsManager.remoteIDSettings.selfIDFree
        }
    }

    // Function to move flickable to desire position
    function getFlickableToPosition(y) {
        flicakbleRID.contentY = y
    }

    Item {
        id:                             flagsItem
        anchors.top:                    parent.top
        anchors.horizontalCenter:       parent.horizontalCenter
        anchors.horizontalCenterOffset: ScreenTools.defaultFontPixelWidth // Need this to account for the slight offset in the flickable
        width:                          flicakbleRID.innerWidth
        height:                         flagsColumn.height

        ColumnLayout {
            id:                         flagsColumn
            anchors.horizontalCenter:   parent.horizontalCenter
            spacing:                    _margins

            // ---------------------------------------- STATUS -----------------------------------------
            // Status flags. Visual representation for the state of all necesary information for remoteID
            // to work propely.
            Rectangle {
                id:                     flagsRectangle
                Layout.preferredHeight: statusGrid.height + (_margins * 2)
                Layout.preferredWidth:  statusGrid.width + (_margins * 2)
                color:                  qgcPal.windowShade
                visible:                _activeVehicle
                Layout.fillWidth:       true

                GridLayout {
                    id:                         statusGrid
                    anchors.margins:            _margins
                    anchors.top:                parent.top
                    anchors.horizontalCenter:   parent.horizontalCenter
                    rows:                       1
                    rowSpacing:                 _margins * 3
                    columnSpacing:              _margins * 2

                    Rectangle {
                        id:                     armFlag
                        Layout.preferredHeight: flagsHeight
                        Layout.preferredWidth:  flagsWidth
                        color:                  _activeRID ? (_activeVehicle.remoteIDManager.armStatusGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                        radius:                 radiusFlags
                        visible:                commsGood

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("ARM STATUS")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                        }

                        // On clikced we go to the corresponding settings
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      getFlickableToPosition(flicakbleRID.armstatusY)
                        }

                    }

                    Rectangle {
                        id:                     commsFlag
                        Layout.preferredHeight: flagsHeight
                        Layout.preferredWidth:  flagsWidth
                        color:                  _activeRID ? (_activeVehicle.remoteIDManager.commsGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                        radius:                 radiusFlags

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   _activeRID && _activeVehicle.remoteIDManager.commsGood ? qsTr("RID COMMS") : qsTr("NOT CONNECTED")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                        }
                    }

                    Rectangle {
                        id:                     gpsFlag
                        Layout.preferredHeight: flagsHeight
                        Layout.preferredWidth:  flagsWidth
                        color:                  _activeRID ? (_activeVehicle.remoteIDManager.gcsGPSGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                        radius:                 radiusFlags
                        visible:                commsGood

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("GCS GPS")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                        }

                        // On clikced we go to the corresponding settings
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      getFlickableToPosition(flicakbleRID.gpsY)
                        }
                    }

                    Rectangle {
                        id:                     basicIDFlag
                        Layout.preferredHeight: flagsHeight
                        Layout.preferredWidth:  flagsWidth
                        color:                  _activeRID ? (_activeVehicle.remoteIDManager.basicIDGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                        radius:                 radiusFlags
                        visible:                commsGood

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("BASIC ID")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                        }

                        // On clikced we go to the corresponding settings
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      getFlickableToPosition(flicakbleRID.basicIDY)
                        }
                    }

                    Rectangle {
                        id:                     operatorIDFlag
                        Layout.preferredHeight: flagsHeight
                        Layout.preferredWidth:  flagsWidth
                        color:                  _activeRID ? (_activeVehicle.remoteIDManager.operatorIDGood ? qgcPal.colorGreen : qgcPal.colorRed) : qgcPal.colorGrey
                        radius:                 radiusFlags
                        visible:                commsGood && _activeRID ? (QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value || _regionOperation == RemoteIDIndicatorPage.RegionOperation.EU) : false

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("OPERATOR ID")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                        }

                        // On clicked we go to the corresponding settings
                        MouseArea {
                            anchors.fill:   parent
                            onClicked:      getFlickableToPosition(flicakbleRID.operatorIDY)
                        }
                    }
                }
            }
        }
    }

    QGCFlickable {
        id:                 flicakbleRID
        clip:               true
        anchors.top:        flagsItem.visible ? flagsItem.bottom : parent.top
        anchors.left:       parent.left
        anchors.right:      parent.right
        anchors.bottom:     parent.bottom
        anchors.margins:    ScreenTools.defaultFontPixelWidth
        contentHeight:      outerItem.height
        contentWidth:       outerItem.width
        flickableDirection: Flickable.VerticalFlick

        property var innerWidth:   settingsItem.width

        // Properties to position flickable
        property var armstatusY:    armStatusLabel.y
        property var gpsY:          gpsLabel.y
        property var basicIDY:      basicIDLabel.y
        property var operatorIDY:   operatorIDLabel.y

        Item {
            id:     outerItem
            width:  Math.max(remoteIDRoot.width, settingsItem.width)
            height: settingsItem.height

            ColumnLayout {
                id:                         settingsItem
                anchors.horizontalCenter:   parent.horizontalCenter
                spacing:                    _margins

                // -----------------------------------------------------------------------------------------
                // ---------------------------------------- ARM STATUS -----------------------------------------
                // Arm status error
                QGCLabel {
                    id:                 armStatusLabel
                    text:               qsTr("ARM STATUS")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                    visible:            _activeVehicle && !_activeVehicle.remoteIDManager.armStatusGood
                }

                Rectangle {
                    id:                     armStatusRectangle
                    Layout.preferredHeight: armStatusGrid.height + (_margins * 2)
                    Layout.preferredWidth:  armStatusGrid.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Layout.fillWidth:       true
                    border.width:           _borderWidth
                    border.color:           _activeRID ? (_activeVehicle.remoteIDManager.armStatusGood ? color : qgcPal.colorRed) : color

                    visible:                _activeVehicle && !_activeVehicle.remoteIDManager.armStatusGood

                    GridLayout {
                        id:                         armStatusGrid
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    2
                        rowSpacing:                 _margins * 3
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               qsTr("Arm status error: ")
                            Layout.fillWidth:   true
                        }
                        QGCLabel {
                            text:               _activeVehicle ? _activeVehicle.remoteIDManager.armStatusError : ""
                            Layout.fillWidth:   true
                        }
                    }
                }
                // -----------------------------------------------------------------------------------------

                // ---------------------------------------- REGION -----------------------------------------
                // Region of operation to accomodate for different requirements
                QGCLabel {
                    id:                 regionLabel
                    text:               qsTr("Region")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    id:                     regionRectangle
                    Layout.preferredHeight: regionGrid.height + (_margins * 2)
                    Layout.preferredWidth:  regionGrid.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                true
                    Layout.fillWidth:       true

                    GridLayout {
                        id:                         regionGrid
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    2
                        rowSpacing:                 _margins * 3
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.region.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.region.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.region
                            visible:            QGroundControl.settingsManager.remoteIDSettings.region.visible
                            Layout.fillWidth:   true
                            sizeToContents:     true
                            // In case we change from EU to FAA having the location Type to FIXED, since its not supported in FAA
                            // we need to change it to Live GNSS
                            onActivated: (index) => {
                                if (currentIndex == RemoteIDIndicatorPage.RegionOperation.FAA && QGroundControl.settingsManager.remoteIDSettings.locationType.value != RemoteIDIndicatorPage.LocationType.LIVE)
                                QGroundControl.settingsManager.remoteIDSettings.locationType.value = RemoteIDIndicatorPage.LocationType.LIVE
                            }
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.classificationType.shortDescription
                            visible:            _regionOperation == RemoteIDIndicatorPage.RegionOperation.EU
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.classificationType
                            visible:            _regionOperation == RemoteIDIndicatorPage.RegionOperation.EU
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.categoryEU.shortDescription
                            visible:            (_classificationType == RemoteIDIndicatorPage.ClassificationType.EU) && (_regionOperation == RemoteIDIndicatorPage.RegionOperation.EU)
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.categoryEU
                            visible:            (_classificationType == RemoteIDIndicatorPage.ClassificationType.EU) && (_regionOperation == RemoteIDIndicatorPage.RegionOperation.EU)
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.classEU.shortDescription
                            visible:            (_classificationType == RemoteIDIndicatorPage.ClassificationType.EU) && (_regionOperation == RemoteIDIndicatorPage.RegionOperation.EU)
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.classEU
                            visible:            (_classificationType == RemoteIDIndicatorPage.ClassificationType.EU) && (_regionOperation == RemoteIDIndicatorPage.RegionOperation.EU)
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }
                    }
                }
                // -----------------------------------------------------------------------------------------

                // ----------------------------------------- GPS -------------------------------------------
                // Data representation and connection options for GCS GPS.
                QGCLabel {
                    id:                 gpsLabel
                    text:               qsTr("GPS GCS")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    id:                     gpsRectangle
                    Layout.preferredHeight: gpsGrid.height + gpsGridData.height + (_margins * 3)
                    Layout.preferredWidth:  gpsGrid.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                true
                    Layout.fillWidth:       true

                    border.width:   _borderWidth
                    border.color:   _activeRID ? (_activeVehicle.remoteIDManager.gcsGPSGood ? color : qgcPal.colorRed) : color

                    property var locationTypeValue: QGroundControl.settingsManager.remoteIDSettings.locationType.value

                    // In case we change from FAA to EU region, having selected Location Type FIXED,
                    // We have to change the currentindex to the locationType forced when we change region
                    onLocationTypeValueChanged: {
                        if (locationTypeComboBox.currentIndex != locationTypeValue) {
                            locationTypeComboBox.currentIndex = locationTypeValue
                        }
                    }

                    GridLayout {
                        id:                         gpsGridData
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        rowSpacing:                 _margins
                        columns:                    2
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.locationType.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.locationType.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            id:                 locationTypeComboBox
                            fact:               QGroundControl.settingsManager.remoteIDSettings.locationType
                            visible:            QGroundControl.settingsManager.remoteIDSettings.locationType.visible
                            Layout.fillWidth:   true
                            sizeToContents:     true

                            onActivated: (index) => {
                                // FAA doesnt allow to set a Fixed position. Is either Live GNSS or Takeoff
                                if (_regionOperation == RemoteIDIndicatorPage.RegionOperation.FAA) {
                                    if (currentIndex != 1) {
                                       QGroundControl.settingsManager.remoteIDSettings.locationType.value = 1
                                        currentIndex = 1
                                    }
                                } else {
                                    // TODO: this lines below efectively disable TAKEOFF option. Uncoment when we add support for it
                                    if (currentIndex == 0) {
                                        QGroundControl.settingsManager.remoteIDSettings.locationType.value = 1
                                        currentIndex = 1
                                    } else {
                                        QGroundControl.settingsManager.remoteIDSettings.locationType.value = index
                                        currentIndex = index
                                    }
                                    // --------------------------------------------------------------------------------------------------
                                }
                            }
                        }

                        QGCLabel {
                            text:               qsTr("Latitude Fixed(-90 to 90)")
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                            fact:               QGroundControl.settingsManager.remoteIDSettings.latitudeFixed
                        }

                        QGCLabel {
                            text:               qsTr("Longitude Fixed(-180 to 180)")
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                            fact:               QGroundControl.settingsManager.remoteIDSettings.longitudeFixed
                        }

                        QGCLabel {
                            text:               qsTr("Altitude Fixed")
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            visible:            _locationType == RemoteIDIndicatorPage.LocationType.FIXED
                            Layout.fillWidth:   true
                            fact:               QGroundControl.settingsManager.remoteIDSettings.altitudeFixed
                        }

                        QGCLabel {
                            text:               qsTr("Latitude")
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                        QGCLabel {
                            text:               gcsPosition.isValid ? gcsPosition.latitude : "N/A"
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }

                        QGCLabel {
                            text:               qsTr("Longitude")
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                        QGCLabel {
                            text:               gcsPosition.isValid ? gcsPosition.longitude : "N/A"
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }

                        QGCLabel {
                            text:               _regionOperation == RemoteIDIndicatorPage.RegionOperation.FAA ?
                                                qsTr("Altitude") + qsTr(" (Mandatory)") :
                                                qsTr("Altitude")
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                        QGCLabel {
                            text:               gcsPosition.isValid && !isNaN(gcsPosition.altitude) ? gcsPosition.altitude : "N/A"
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }

                        QGCLabel {
                            text:               qsTr("Heading")
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                        QGCLabel {
                            text:               gcsPosition.isValid && !isNaN(gcsHeading) ? gcsHeading : "N/A"
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }

                        QGCLabel {
                            text:               qsTr("Hor. Accuracy")
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                        QGCLabel {
                            text:               gcsPosition.isValid && gcsHDOP ? ( gcsHDOP + " m" ) : "N/A"
                            Layout.fillWidth:   true
                            visible:            _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        }
                    }

                    GridLayout {
                        id:                         gpsGrid
                        visible:                    !ScreenTools.isMobile
                                                    && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.visible
                                                    && QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.visible
                                                    && _locationType != RemoteIDIndicatorPage.LocationType.TAKEOFF
                        anchors.margins:            _margins
                        anchors.top:                gpsGridData.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        rowSpacing:                 _margins * 3
                        columns:                    2
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text: qsTr("NMEA External GPS Device")
                        }
                        QGCComboBox {
                            id:                     nmeaPortCombo
                            Layout.preferredWidth:  _comboFieldWidth

                            model:  ListModel {
                            }

                            onActivated: (index) => {
                                if (index != -1) {
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.value = textAt(index);
                                }
                            }
                            Component.onCompleted: {
                                model.append({text: gpsDisabled})
                                model.append({text: gpsUdpPort})

                                for (var i in QGroundControl.linkManager.serialPorts) {
                                    nmeaPortCombo.model.append({text:QGroundControl.linkManager.serialPorts[i]})
                                }
                                var index = nmeaPortCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaPort.valueString);
                                nmeaPortCombo.currentIndex = index;
                                if (QGroundControl.linkManager.serialPorts.length === 0) {
                                    nmeaPortCombo.model.append({text: "Serial <none available>"})
                                }
                            }
                        }

                        QGCLabel {
                            visible:          nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
                            text:             qsTr("NMEA GPS Baudrate")
                        }
                        QGCComboBox {
                            visible:                nmeaPortCombo.currentText !== gpsUdpPort && nmeaPortCombo.currentText !== gpsDisabled
                            id:                     nmeaBaudCombo
                            Layout.preferredWidth:  _comboFieldWidth
                            model:                  [1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400, 460800, 921600]

                            onActivated: (index) => {
                                if (index != -1) {
                                    QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.value = textAt(index);
                                }
                            }
                            Component.onCompleted: {
                                var index = nmeaBaudCombo.find(QGroundControl.settingsManager.autoConnectSettings.autoConnectNmeaBaud.valueString);
                                nmeaBaudCombo.currentIndex = index;
                            }
                        }

                        QGCLabel {
                            text:       qsTr("NMEA stream UDP port")
                            visible:    nmeaPortCombo.currentText === gpsUdpPort
                        }
                        FactTextField {
                            visible:                nmeaPortCombo.currentText === gpsUdpPort
                            Layout.preferredWidth:  _valueFieldWidth
                            fact:                   QGroundControl.settingsManager.autoConnectSettings.nmeaUdpPort
                        }
                    }
                }
                // -----------------------------------------------------------------------------------------

                // -------------------------------------- BASIC ID -------------------------------------------
                QGCLabel {
                    id:                 basicIDLabel
                    text:               qsTr("BASIC ID")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    id:                     basicIDRectangle
                    Layout.preferredHeight: basicIDGrid.height + basicIDnote.height + (_margins * 4)
                    Layout.preferredWidth:  basicIDGrid.width  + basicIDnote.width  + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Layout.fillWidth:       true

                    border.width:   _borderWidth
                    border.color:   _activeRID ? (_activeVehicle.remoteIDManager.basicIDGood ? color : qgcPal.colorRed) : color

                    QGCLabel {
                        id:                         basicIDnote
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.bottomMargin:       _margins * 2
                        width:                      basicIDGrid.width
                        text:                       qsTr("Note: This parameter is optional if Basic ID is already set on RID device. " +
                                                         "On that case, this one will be registered as Basic ID 2")
                        wrapMode:                   Text.Wrap
                        visible:                    QGroundControl.settingsManager.remoteIDSettings.basicIDType.visible

                    }

                    GridLayout {
                        id:                         basicIDGrid
                        anchors.margins:            _margins
                        anchors.top:                basicIDnote.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    2
                        rowSpacing:                 _margins * 3
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.basicIDType.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicIDType.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.basicIDType
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicIDType.visible
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.basicIDUaType.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicIDUaType.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.basicIDUaType
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicIDUaType.visible
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }

                        QGCLabel {
                            text:               _activeRID && _activeVehicle.remoteIDManager.basicIDGood ?
                                                QGroundControl.settingsManager.remoteIDSettings.basicID.shortDescription :
                                                QGroundControl.settingsManager.remoteIDSettings.basicID.shortDescription + qsTr(" (Mandatory)")
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicID.visible
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               QGroundControl.settingsManager.remoteIDSettings.basicID
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicID.visible
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.sendBasicID.shortDescription
                            Layout.fillWidth:   true
                            visible:            QGroundControl.settingsManager.remoteIDSettings.basicID.visible
                        }
                        FactCheckBox {
                            fact:       QGroundControl.settingsManager.remoteIDSettings.sendBasicID
                            visible:    QGroundControl.settingsManager.remoteIDSettings.basicID.visible
                        }
                    }
                }
                // ------------------------------------------------------------------------------------------

                // ------------------------------------ OPERATOR ID ----------------------------------------
                QGCLabel {
                    id:                 operatorIDLabel
                    text:               qsTr("Operator ID")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    id:                     operatorIDRectangle
                    Layout.preferredHeight: operatorIDGrid.height + (_margins * 3)
                    Layout.preferredWidth:  operatorIDGrid.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    Layout.fillWidth:       true

                    border.width:   _borderWidth
                    border.color:   (_regionOperation == RemoteIDIndicatorPage.RegionOperation.EU || QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value) ?
                                    (_activeRID && !_activeVehicle.remoteIDManager.operatorIDGood ? qgcPal.colorRed : color) : color

                    GridLayout {
                        id:                         operatorIDGrid
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    2
                        rowSpacing:                 _margins * 3
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.operatorIDType.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.operatorIDType.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            id:                 operatorIDFactComboBox
                            fact:               QGroundControl.settingsManager.remoteIDSettings.operatorIDType
                            visible:            QGroundControl.settingsManager.remoteIDSettings.operatorIDType.visible && (QGroundControl.settingsManager.remoteIDSettings.operatorIDType.enumValues.length > 1)
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }
                        QGCLabel{
                            text:               QGroundControl.settingsManager.remoteIDSettings.operatorIDType.enumStringValue
                            visible:            !operatorIDFactComboBox.visible
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text:               _regionOperation == RemoteIDIndicatorPage.RegionOperation.FAA ?
                                                QGroundControl.settingsManager.remoteIDSettings.operatorID.shortDescription :
                                                QGroundControl.settingsManager.remoteIDSettings.operatorID.shortDescription + qsTr(" (Mandatory)")
                            visible:            QGroundControl.settingsManager.remoteIDSettings.operatorID.visible
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            id:                 operatorIDTextField
                            fact:               QGroundControl.settingsManager.remoteIDSettings.operatorID
                            visible:            QGroundControl.settingsManager.remoteIDSettings.operatorID.visible
                            Layout.fillWidth:   true
                            maximumLength:      20 // Maximum defined by Mavlink definition of OPEN_DRONE_ID_OPERATOR_ID message
	                    onTextChanged: {
                                if (_activeVehicle) {
                                    _activeVehicle.remoteIDManager.checkOperatorID(text)
                                } else {
                                    _offlineVehicle.remoteIDManager.checkOperatorID(text)
                                }
                            }
                            onEditingFinished: {
                                if (_activeVehicle) {
                                    _activeVehicle.remoteIDManager.setOperatorID()
                                } else {
                                    _offlineVehicle.remoteIDManager.setOperatorID()
                                }
                            }
                        }

                        // Spacer
                        QGCLabel {
                            text:               ""
                            visible:            _regionOperation == RemoteIDIndicatorPage.RegionOperation.EU
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.operatorID.shortDescription + qsTr(QGroundControl.settingsManager.remoteIDSettings.operatorIDValid.rawValue == true ? " valid" : " invalid")
                            visible:            _regionOperation == RemoteIDIndicatorPage.RegionOperation.EU
                            Layout.alignment:   Qt.AlignHCenter
                            Layout.fillWidth:   true
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.shortDescription
                            Layout.fillWidth:   true
                            visible:            _regionOperation == RemoteIDIndicatorPage.RegionOperation.FAA
                        }
                        FactCheckBox {
                            fact:       QGroundControl.settingsManager.remoteIDSettings.sendOperatorID
                            visible:    _regionOperation == RemoteIDIndicatorPage.RegionOperation.FAA
                            onClicked: {
                                if (checked) {
                                    if (_activeVehicle) {
                                        _activeVehicle.remoteIDManager.setOperatorID()
                                    }
                                }
                            }
                        }
                    }
                }
                // -----------------------------------------------------------------------------------------

                // -------------------------------------- SELF ID ------------------------------------------
                QGCLabel {
                    id:                 selfIDLabel
                    text:               qsTr("Self ID")
                    Layout.alignment:   Qt.AlignHCenter
                    font.pointSize:     ScreenTools.mediumFontPointSize
                }

                Rectangle {
                    id:                     selfIDRectangle
                    Layout.preferredHeight: selfIDGrid.height + selfIDnote.height + (_margins * 3)
                    Layout.preferredWidth:  selfIDGrid.width + (_margins * 2)
                    color:                  qgcPal.windowShade
                    visible:                true
                    Layout.fillWidth:       true

                    GridLayout {
                        id:                         selfIDGrid
                        anchors.margins:            _margins
                        anchors.top:                parent.top
                        anchors.horizontalCenter:   parent.horizontalCenter
                        columns:                    2
                        rowSpacing:                 _margins * 3
                        columnSpacing:              _margins * 2

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.selfIDType.shortDescription
                            visible:            QGroundControl.settingsManager.remoteIDSettings.selfIDType.visible
                            Layout.fillWidth:   true
                        }
                        FactComboBox {
                            id:                 selfIDComboBox
                            fact:               QGroundControl.settingsManager.remoteIDSettings.selfIDType
                            visible:            QGroundControl.settingsManager.remoteIDSettings.selfIDType.visible
                            Layout.fillWidth:   true
                            sizeToContents:     true
                        }

                        QGCLabel {
                            text:               getSelfIdLabelText()
                            Layout.fillWidth:   true
                        }
                        FactTextField {
                            fact:               getSelfIDFact()
                            Layout.fillWidth:   true
                            maximumLength:      23 // Maximum defined by Mavlink definition of OPEN_DRONE_ID_SELF_ID message
                        }

                        QGCLabel {
                            text:               QGroundControl.settingsManager.remoteIDSettings.sendSelfID.shortDescription
                            Layout.fillWidth:   true
                        }
                        FactCheckBox {
                            fact:       QGroundControl.settingsManager.remoteIDSettings.sendSelfID
                            visible:    QGroundControl.settingsManager.remoteIDSettings.sendSelfID.visible
                        }
                    }

                    QGCLabel {
                        id:                         selfIDnote
                        width:                      selfIDGrid.width
                        anchors.margins:            _margins
                        anchors.top:                selfIDGrid.bottom
                        anchors.horizontalCenter:   parent.horizontalCenter
                        anchors.bottomMargin:       _margins * 2
                        text:                       qsTr("Note: Even if this box is unset, QGroundControl will send self ID message " +
                                                         "if an emergency is set, or after it has been cleared. \
                                                         The message for each kind of selfID is saved and preserves reboots. Select " +
                                                         "each type on the Self ID type dropdown to configure the message to be sent")
                        wrapMode:                   Text.Wrap
                        visible:                    QGroundControl.settingsManager.remoteIDSettings.selfIDType.visible
                    }
                }
                // -----------------------------------------------------------------------------------------
            }
        }
    }
}
