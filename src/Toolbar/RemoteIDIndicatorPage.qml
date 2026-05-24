import QtQuick
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

ToolIndicatorPage {
    showExpand: true

    property var    _activeVehicle:     QGroundControl.multiVehicleManager.activeVehicle
    property var    remoteIDManager:    _activeVehicle ? _activeVehicle.remoteIDManager : null

    property bool   gpsFlag:            remoteIDManager ? remoteIDManager.gcsGPSGood         : false
    property bool   basicIDFlag:        remoteIDManager ? remoteIDManager.basicIDGood        : false
    property bool   armFlag:            remoteIDManager ? remoteIDManager.armStatusGood      : false
    property bool   commsFlag:          remoteIDManager ? remoteIDManager.commsGood          : false
    property bool   emergencyDeclared:  remoteIDManager ? remoteIDManager.emergencyDeclared  : false
    property bool   operatorIDFlag:     remoteIDManager ? remoteIDManager.operatorIDGood     : false

    property int    _regionOperation:   QGroundControl.settingsManager.remoteIDSettings.region.value

    // Flags visual properties
    property real   flagsWidth:         ScreenTools.defaultFontPixelWidth * 10
    property real   flagsHeight:        ScreenTools.defaultFontPixelWidth * 5
    property int    radiusFlags:        5

    // Visual properties
    property real   _margins:           ScreenTools.defaultFontPixelWidth

    function goToSettings() {
        if (mainWindow.allowViewSwitch()) {
            mainWindow.closeIndicatorDrawer()
            globals.commingFromRIDIndicator = true
            mainWindow.showSettingsTool()
        }
    }

    contentComponent: Component {
        Rectangle {
            width:          remoteIDCol.width + ScreenTools.defaultFontPixelWidth  * 3
            height:         remoteIDCol.height + ScreenTools.defaultFontPixelHeight * 2 + (emergencyButtonItem.visible ? emergencyButtonItem.height : 0)
            radius:         ScreenTools.defaultFontPixelHeight * 0.5
            color:          qgcPal.window
            border.color:   qgcPal.text

            Column {
                id:                         remoteIDCol
                spacing:                    ScreenTools.defaultFontPixelHeight * 0.5
                width:                      Math.max(remoteIDGrid.width, remoteIDLabel.width)
                anchors.margins:            ScreenTools.defaultFontPixelHeight
                anchors.top:                parent.top
                anchors.horizontalCenter:   parent.horizontalCenter

                QGCLabel {
                    id:                         remoteIDLabel
                    text:                       qsTr("RemoteID Status")
                    font.bold:                  true
                    anchors.horizontalCenter:   parent.horizontalCenter
                }

                GridLayout {
                    id:                         remoteIDGrid
                    anchors.margins:            ScreenTools.defaultFontPixelHeight
                    columnSpacing:              ScreenTools.defaultFontPixelWidth
                    anchors.horizontalCenter:   parent.horizontalCenter
                    columns:                    2

                    Image {
                        id:                 armFlagImage
                        width:              flagsWidth
                        height:             flagsHeight
                        source:             armFlag ? "/qmlimages/RidFlagBackgroundGreen.svg" : "/qmlimages/RidFlagBackgroundRed.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        visible:            commsFlag

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("ARM STATUS")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                            font.pointSize:         ScreenTools.smallFontPointSize
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      goToSettings()
                        }
                    }

                    Image {
                        id:                 commsFlagImage
                        width:              flagsWidth
                        height:             flagsHeight
                        source:             commsFlag ? "/qmlimages/RidFlagBackgroundGreen.svg" : "/qmlimages/RidFlagBackgroundRed.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   commsFlag ? qsTr("RID COMMS") : qsTr("NOT CONNECTED")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                            font.pointSize:         ScreenTools.smallFontPointSize
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      goToSettings()
                        }
                    }

                    Image {
                        id:                 gpsFlagImage
                        width:              flagsWidth
                        height:             flagsHeight
                        source:             gpsFlag ? "/qmlimages/RidFlagBackgroundGreen.svg" : "/qmlimages/RidFlagBackgroundRed.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        visible:            commsFlag

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("GCS GPS")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                            font.pointSize:         ScreenTools.smallFontPointSize
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      goToSettings()
                        }
                    }

                    Image {
                        id:                 basicIDFlagImage
                        width:              flagsWidth
                        height:             flagsHeight
                        source:             basicIDFlag ? "/qmlimages/RidFlagBackgroundGreen.svg" : "/qmlimages/RidFlagBackgroundRed.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        visible:            commsFlag

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("BASIC ID")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                            font.pointSize:         ScreenTools.smallFontPointSize
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      goToSettings()
                        }
                    }

                    Image {
                        id:                 operatorIDFlagImage
                        width:              flagsWidth
                        height:             flagsHeight
                        source:             operatorIDFlag ? "/qmlimages/RidFlagBackgroundGreen.svg" : "/qmlimages/RidFlagBackgroundRed.svg"
                        fillMode:           Image.PreserveAspectFit
                        sourceSize.height:  height
                        visible:            commsFlag ? (QGroundControl.settingsManager.remoteIDSettings.sendOperatorID.value || _regionOperation == RemoteIDSettings.RegionOperation.EU) : false

                        QGCLabel {
                            anchors.fill:           parent
                            text:                   qsTr("OPERATOR ID")
                            wrapMode:               Text.WordWrap
                            horizontalAlignment:    Text.AlignHCenter
                            verticalAlignment:      Text.AlignVCenter
                            font.bold:              true
                            font.pointSize:         ScreenTools.smallFontPointSize
                        }

                        QGCMouseArea {
                            anchors.fill:   parent
                            onClicked:      goToSettings()
                        }
                    }
                }
            }

            Item {
                id:             emergencyButtonItem
                anchors.top:    remoteIDCol.bottom
                anchors.left:   parent.left
                anchors.right:  parent.right
                height:         emergencyDeclareLabel.height + emergencyButton.height + (_margins * 4)
                visible:        commsFlag

                QGCLabel {
                    id:                     emergencyDeclareLabel
                    text:                   emergencyDeclared ? qsTr("EMERGENCY HAS BEEN DECLARED, Press and Hold for 3 seconds to cancel") : qsTr("Press and Hold below button to declare emergency")
                    font.bold:              true
                    anchors.top:            parent.top
                    anchors.left:           parent.left
                    anchors.right:          parent.right
                    anchors.margins:        _margins
                    anchors.topMargin:      _margins * 3
                    wrapMode:               Text.WordWrap
                    horizontalAlignment:    Text.AlignHCenter
                }

                Image {
                    id:                         emergencyButton
                    width:                      flagsWidth * 2
                    height:                     flagsHeight * 1.5
                    source:                     highlighted ? "/qmlimages/RidEmergencyBackgroundHighlight.svg" : "/qmlimages/RidEmergencyBackground.svg"
                    sourceSize.height:          height
                    anchors.horizontalCenter:   parent.horizontalCenter
                    anchors.top:                emergencyDeclareLabel.bottom
                    anchors.margins:            _margins

                    property bool highlighted: emergencyButtonMouseArea.containsMouse || emergencyButtonMouseArea.pressed || emergencyButtonFlashOn
                    property bool emergencyButtonFlashOn: false

                    QGCLabel {
                        anchors.fill:           parent
                        text:                   emergencyDeclared ? qsTr("Clear Emergency") : qsTr("EMERGENCY")
                        wrapMode:               Text.WordWrap
                        horizontalAlignment:    Text.AlignHCenter
                        verticalAlignment:      Text.AlignVCenter
                        font.bold:              true
                        font.pointSize:         ScreenTools.largeFontPointSize
                    }

                    Timer {
                        id:             emergencyButtonTimer
                        interval:       350
                        onTriggered:    emergencyButton.emergencyButtonFlashOn = false
                    }

                    MouseArea {
                        id:                     emergencyButtonMouseArea
                        anchors.fill:           parent
                        hoverEnabled:           true
                        pressAndHoldInterval:   emergencyDeclared ? 3000 : 800
                        onPressAndHold: {
                            emergencyButton.emergencyButtonFlashOn = true
                            emergencyButtonTimer.restart()
                            if (remoteIDManager) {
                                remoteIDManager.setEmergency(!emergencyDeclared)
                            }
                        }
                    }
                }
            }
        }
    }

    expandedComponent: Component {
        RowLayout {
            spacing: ScreenTools.defaultFontPixelWidth

            property var  remoteIDSettings:     QGroundControl.settingsManager.remoteIDSettings
            property Fact regionFact:           remoteIDSettings.region
            property Fact sendOperatorIdFact:   remoteIDSettings.sendOperatorID
            property Fact locationTypeFact:     remoteIDSettings.locationType
            property Fact operatorIDFact:       remoteIDSettings.operatorID
            property bool isEURegion:           regionFact.rawValue == RemoteIDSettings.RegionOperation.EU
            property bool isFAARegion:          regionFact.rawValue == RemoteIDSettings.RegionOperation.FAA
            property real textFieldWidth:       ScreenTools.defaultFontPixelWidth * 24
            property real textLabelWidth:       ScreenTools.defaultFontPixelWidth * 30

            Connections {
                target: regionFact
                function onRawValueChanged() {
                    if (regionFact.rawValue === RemoteIDSettings.RegionOperation.EU) {
                        sendOperatorIdFact.rawValue = true
                    }
                    if (regionFact.rawValue === RemoteIDSettings.RegionOperation.FAA) {
                        locationTypeFact.value = RemoteIDSettings.LocationType.LIVE
                    }
                }
            }

            ColumnLayout {
                spacing:            ScreenTools.defaultFontPixelHeight / 2
                Layout.alignment:   Qt.AlignTop


                SettingsGroupLayout {
                    visible:            armStatusLabel.labelText !== ""
                    LabelledLabel {
                        id :                armStatusLabel
                        label:              qsTr("Arm Status Error")
                        labelText:          remoteIDManager ? remoteIDManager.armStatusError : ""
                        Layout.fillWidth:   true
                    }
                }

                SettingsGroupLayout {
                    heading:            qsTr("Self ID")
                    Layout.fillWidth:       true

                    ColumnLayout {
                        Layout.fillWidth:   true
                        spacing:            ScreenTools.defaultFontPixelHeight / 2

                        FactCheckBoxSlider {
                            id:                 sendSelfIDSlider
                            text:               qsTr("Broadcast")
                            fact:               _fact
                            visible:            _fact.userVisible
                            Layout.fillWidth:   true

                            property Fact _fact: remoteIDSettings.sendSelfID
                        }

                        QGCLabel {
                            text:                   qsTr("If an emergency is declared, Emergency Text will be broadcast even if Broadcast setting is not enabled.")
                            font.pointSize:         ScreenTools.smallFontPointSize
                            wrapMode:               Text.WordWrap
                            Layout.preferredWidth:  sendSelfIDSlider.width
                            visible:                !sendSelfIDSlider._fact.rawValue
                        }
                    }

                    LabelledFactComboBox {
                        id:                 selfIDTypeCombo
                        label:              qsTr("Broadcast Message")
                        fact:               _fact
                        indexModel:         false
                        visible:            _fact.userVisible
                        enabled:            sendSelfIDSlider._fact.rawValue
                        Layout.fillWidth:   true

                        property Fact _fact: remoteIDSettings.selfIDType
                    }

                    LabelledFactTextField {
                        label:                      _fact.shortDescription
                        fact:                       _fact
                        visible:                    _fact.userVisible
                        enabled:                     sendSelfIDSlider._fact.rawValue
                        textField.maximumLength:    23
                        Layout.fillWidth:           true
                        textFieldPreferredWidth:    textFieldWidth

                        property Fact _fact: remoteIDSettings.selfIDFree
                    }

                    LabelledFactTextField {
                        label:                      _fact.shortDescription
                        fact:                       _fact
                        visible:                    _fact.userVisible
                        enabled:                    sendSelfIDSlider._fact.rawValue
                        textField.maximumLength:    23
                        Layout.fillWidth:           true
                        textFieldPreferredWidth:    textFieldWidth

                        property Fact _fact: remoteIDSettings.selfIDExtended
                    }

                    LabelledFactTextField {
                        label:                      _fact.shortDescription
                        fact:                       _fact
                        visible:                    _fact.userVisible
                        textField.maximumLength:    23
                        Layout.fillWidth:           true
                        textFieldPreferredWidth:    textFieldWidth

                        property Fact _fact: remoteIDSettings.selfIDEmergency
                    }
                }

                SettingsGroupLayout {
                    Layout.fillWidth:   true
                    visible:            QGroundControl.corePlugin.showAdvancedUI

                    RowLayout {
                        Layout.fillWidth: true

                        QGCLabel { Layout.fillWidth: true; text: qsTr("Remote ID") }
                        QGCButton {
                            text: qsTr("Configure")
                            onClicked: {
                                goToSettings()
                            }
                        }
                    }
                }
            }

        }
    }
}
