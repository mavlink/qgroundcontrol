import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

import QGroundControl
import QGroundControl.Controls
import QGroundControl.FactControls

// Editor for Simple mission items
Rectangle {
    id: root
    width: availableWidth
    height: editorColumn.height + (_margin * 2)
    color: qgcPal.windowShadeDark
    radius: _radius

    property bool _specifiesAltitude: missionItem.specifiesAltitude
    property real _margin: ScreenTools.defaultFontPixelHeight / 2
    property real _altRectMargin: ScreenTools.defaultFontPixelWidth / 2
    property var _controllerVehicle: missionItem.masterController.controllerVehicle
    property int _globalAltMode: missionItem.masterController.missionController.globalAltitudeMode
    property bool _globalAltModeIsMixed: _globalAltMode == QGroundControl.AltitudeModeMixed
    property real _radius: ScreenTools.defaultFontPixelWidth / 2
    property real _fieldSpacing: ScreenTools.defaultFontPixelHeight / 2

    QGCPalette { id: qgcPal; colorGroupEnabled: root.enabled }

    Column {
        id: editorColumn
        anchors.margins: _margin
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.top: parent.top
        spacing: _margin

        // Takeoff item
        ColumnLayout {
            anchors.margins: _margin
            anchors.left: parent.left
            anchors.right: parent.right
            spacing: _margin
            visible: missionItem.isTakeoffItem && missionItem.wizardMode // Hack special case for takeoff item

            QGCLabel {
                text: qsTr("Move '%1' %2 to the %3 location. %4")
                    .arg(_controllerVehicle.vtol ? qsTr("T") : qsTr("T"))
                    .arg(_controllerVehicle.vtol ? qsTr("Transition Direction") : qsTr("Takeoff"))
                    .arg(_controllerVehicle.vtol ? qsTr("desired") : qsTr("climbout"))
                    .arg(_controllerVehicle.vtol ? (qsTr("Ensure distance from launch to transition direction is far enough to complete transition.")) : "")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !initialClickLabel.visible
            }

            QGCLabel {
                text: qsTr("Ensure clear of obstacles and into the wind.")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: !initialClickLabel.visible
            }

            QGCButton {
                text: qsTr("Done")
                Layout.fillWidth: true
                visible: !initialClickLabel.visible
                onClicked: {
                    missionItem.wizardMode = false
                }
            }

            QGCLabel {
                id: initialClickLabel
                text: missionItem.launchTakeoffAtSameLocation ?
                                        qsTr("Click in map to set planned Takeoff location.") :
                                        qsTr("Click in map to set planned Launch location.")
                Layout.fillWidth: true
                wrapMode: Text.WordWrap
                visible: missionItem.isTakeoffItem && !missionItem.launchCoordinate.isValid
            }
        }

        ColumnLayout {
            width: parent.width
            spacing: _fieldSpacing
            visible: !missionItem.wizardMode

            QGCTabBar {
                id: tabBar
                Layout.fillWidth: true
                visible: _multipleTabsVisible()

                property bool _basicItemsAvailable: _specifiesAltitude || missionItem.speedSection.available || missionItem.comboboxFacts.count > 0 || missionItem.textFieldFacts.count > 0 || missionItem.nanFacts.count > 0
                property bool _advancedItemsAvailable: missionItem.comboboxFactsAdvanced.count > 0 || missionItem.textFieldFactsAdvanced.count > 0 || missionItem.nanFactsAdvanced.count > 0
                property bool _cameraAvailable: missionItem.cameraSection.available

                function _multipleTabsVisible() {
                    let visibleCount = 0
                    if (_basicItemsAvailable) visibleCount++
                    if (_cameraAvailable) visibleCount++
                    if (_advancedItemsAvailable) visibleCount++
                    return visibleCount > 1
                }

                Component.onCompleted: {
                    if (_basicItemsAvailable) {
                        tabBar.currentIndex = 0
                    } else if (_cameraAvailable) {
                        tabBar.currentIndex = 1
                    } else if (_advancedItemsAvailable) {
                        tabBar.currentIndex = 2
                    } else {
                        tabBar.currentIndex = -1
                    }
                }

                QGCTabButton {
                    id: basicItemsTab
                    icon.source: "/res/PlanSimpleItemBasic.svg"
                    visible: tabBar._basicItemsAvailable
                }

                QGCTabButton {
                    id: cameraTab
                    icon.source: "/res/PlanSimpleItemCamera.svg"
                    visible: tabBar._cameraAvailable
                }

                QGCTabButton {
                    id: advancedItemsTab
                    icon.source: "/res/PlanSimpleItemAdvanced.svg"
                    visible: tabBar._advancedItemsAvailable
                }
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: _fieldSpacing
                visible: tabBar.currentIndex === 0

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing
                    visible: _specifiesAltitude

                    RowLayout {
                        Layout.fillWidth: true
                        visible: _globalAltModeIsMixed

                        QGCLabel {
                            Layout.fillWidth: true
                            text: qsTr("Altitude Mode")
                        }

                        AltModeCombo {
                            altitudeMode: missionItem.altitudeMode
                            vehicle: _controllerVehicle
                            onAltitudeModeChanged: missionItem.altitudeMode = altitudeMode
                        }
                    }

                    FactTextFieldSlider {
                        id: altField
                        Layout.fillWidth: true
                        label: qsTr("Altitude%1").arg(_extraLabelText())
                        fact: missionItem.altitude

                        function _extraLabelText() {
                            if (!_globalAltModeIsMixed && missionItem.altitudeMode !== QGroundControl.AltitudeModeRelative) {
                                return qsTr(" (%1)").arg(QGroundControl.altitudeModeShortDescription(missionItem.altitudeMode))
                            } else {
                                return ""
                            }
                        }
                    }

                    QGCLabel {
                        font.pointSize: ScreenTools.smallFontPointSize
                        text: qsTr("Actual AMSL alt sent: %1 %2").arg(missionItem.amslAltAboveTerrain.valueString).arg(missionItem.amslAltAboveTerrain.units)
                        visible: missionItem.altitudeMode === QGroundControl.AltitudeModeCalcAboveTerrain
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing

                    Repeater {
                        model: missionItem.comboboxFacts

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QGCLabel {
                                font.pointSize: ScreenTools.smallFontPointSize
                                text: object.name
                                visible: object.name !== ""
                            }

                            FactComboBox {
                                Layout.fillWidth: true
                                indexModel: false
                                model: object.enumStrings
                                fact: object
                            }
                        }
                    }
                }

                Repeater {
                    model: missionItem.textFieldFacts

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        enabled: !object.readOnly
                    }
                }

                Repeater {
                    model: missionItem.nanFacts

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        showEnableCheckbox: true
                        enableCheckBoxChecked: !isNaN(object.rawValue)

                        onEnableCheckboxClicked: object.rawValue = enableCheckBoxChecked ? 0 : NaN
                    }
                }

                FactTextFieldSlider {
                    Layout.fillWidth: true
                    label: qsTr("Flight Speed")
                    fact: missionItem.speedSection.flightSpeed
                    showEnableCheckbox: true
                    enableCheckBoxChecked: missionItem.speedSection.specifyFlightSpeed
                    visible: missionItem.speedSection.available

                    onEnableCheckboxClicked: missionItem.speedSection.specifyFlightSpeed = enableCheckBoxChecked
                }
            }

            CameraSection {
                Layout.fillWidth: true
                showSectionHeader: false
                visible: tabBar.currentIndex === 1

                Component.onCompleted: checked = missionItem.cameraSection.settingsSpecified
            }

            ColumnLayout {
                Layout.fillWidth: true
                spacing: _fieldSpacing
                visible: tabBar.currentIndex === 2

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: _fieldSpacing

                    Repeater {
                        model: missionItem.comboboxFactsAdvanced

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 0

                            QGCLabel {
                                font.pointSize: ScreenTools.smallFontPointSize
                                text: object.name
                                visible: object.name !== ""
                            }

                            FactComboBox {
                                Layout.fillWidth: true
                                indexModel: false
                                model: object.enumStrings
                                fact: object
                            }
                        }
                    }
                }

                Repeater {
                    model: missionItem.textFieldFactsAdvanced

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        enabled: !object.readOnly
                    }
                }

                Repeater {
                    model: missionItem.nanFactsAdvanced

                    FactTextFieldSlider {
                        Layout.fillWidth: true
                        label: object.name
                        fact: object
                        showEnableCheckbox: true
                        enableCheckBoxChecked: !isNaN(object.rawValue)

                        onEnableCheckboxClicked: object.rawValue = enableCheckBoxChecked ? 0 : NaN
                    }
                }
            }
        }
    }
}
